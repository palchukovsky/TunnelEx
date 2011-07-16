/**************************************************************************
 *   Created: 2007/03/01 4:06
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "Tunnel.hpp"
#include "UniquePtr.hpp"
#include "TunnelConnectionSignal.hpp"
#include "TunnelBuffer.hpp"
#include "ServerWorker.hpp"
#include "Connection.hpp"
#include "Acceptor.hpp"
#include "Log.hpp"
#include "ModulesFactory.hpp"
#include "Listener.hpp"
#include "EndpointAddress.hpp"
#include "String.hpp"
#include "Licensing.hpp"
#include "Locking.hpp"

using namespace TunnelEx;

//////////////////////////////////////////////////////////////////////////

namespace {
	
	void ReportOpenError(
				const Tunnel &tunnel,
				const EndpointAddress &remoteAddr,
				const LocalException &error) {
		if (!Log::GetInstance().IsDebugRegistrationOn()) {
			return;
		}
		Format message(
			"Opening new outcoming connection for tunnel %1% to the %2% "
				"is failed with the error \"%3%\", will try next endpoint...");
		message % tunnel.GetInstanceId();
		String buffer;
		TunnelEx::ConvertString(remoteAddr.GetResourceIdentifier(), buffer);
		message % buffer.GetCStr();
		TunnelEx::ConvertString(error.GetWhat(), buffer);
		message % buffer.GetCStr();
		Log::GetInstance().AppendDebug(message.str());
	}

}

//////////////////////////////////////////////////////////////////////////

Tunnel::ReadWriteConnections::ReadWriteConnections() {
	//...//
}

Tunnel::ReadWriteConnections::ReadWriteConnections(
			SharedPtr<Connection> read,
			SharedPtr<Connection> write)
		: read(read),
		write(write) {
	//...//
}

void Tunnel::ReadWriteConnections::Swap(Tunnel::ReadWriteConnections &rhs) throw() {
	rhs.read.Swap(read);
	rhs.write.Swap(write);
}

//////////////////////////////////////////////////////////////////////////

struct Tunnel::Licenses : private boost::noncopyable {

	typedef ACE_Thread_Mutex Mutex;
	typedef ACE_Guard<Mutex> Lock;

	Licenses() 
			: rwSplitLicense(&rwSplitLicenseState) {
		//...//
	}

	Licensing::FsLocalStorageState rwSplitLicenseState;
	Licensing::EndpointIoSeparationLicense rwSplitLicense;
	Mutex rwMutex;

};

//////////////////////////////////////////////////////////////////////////

class Tunnel::ListenerBinder : private boost::noncopyable {
public:
	class ListenerBinder(
					std::list<SharedPtr<const Listener> >  &collection,
					TunnelConnectionSignal &signal)
				: m_collection(collection),
			m_signal(&signal) {
		//...//
	}
	void Bind(SharedPtr<Listener> listener) {
		// I don't want to copy vector here before connect signal as it
		// can be to slow for each tunnel and each connection.
		m_collection.push_back(listener);
		try {
			m_signal->ConnectToOnNewMessageBlockSignal(
				boost::bind(&Listener::OnNewMessageBlock, listener.Get(), _1));
		} catch (...) {
			m_collection.pop_back();
			throw;
		}
	}
	void SetSignal(TunnelConnectionSignal &signal) {
		m_signal = &signal;
	}
private:
	std::list<SharedPtr<const Listener> >  &m_collection;
	TunnelConnectionSignal *m_signal;
};

//////////////////////////////////////////////////////////////////////////

template<class Base>
class Tunnel::ConnectionOpeningExceptionImpl : public Base {
public:
	explicit ConnectionOpeningExceptionImpl(
				const Tunnel &tunnel,
				SharedPtr<const EndpointAddress> address,
				const LocalException &error,
				const wchar_t *const connectionType)
			: Base(L""),
			m_tunnelInstanceId(tunnel.GetInstanceId()),
			m_address(address),
			m_error(error.Clone()),
			m_connectionType(connectionType) {
		//...//
	}
	ConnectionOpeningExceptionImpl(
				const ConnectionOpeningExceptionImpl &rhs)
			: Base(rhs),
			m_tunnelInstanceId(rhs.m_tunnelInstanceId),
			m_address(rhs.m_address),
			m_error(rhs.m_error->Clone()),
			m_connectionType(rhs.m_connectionType) {
		//...//
	}
	ConnectionOpeningExceptionImpl & operator =(
				const ConnectionOpeningExceptionImpl &rhs) {
		DestinationConnectionOpeningException::operator =(rhs);
		m_tunnelInstanceId = rhs.m_tunnelInstanceId;
		m_address = rhs.m_address;
		m_error = rhs.m_error->Clone();
		m_connectionType = rhs.m_connectionType;
		return *this;
	}
	virtual const wchar_t * GetWhat() const throw() {
		try {
			return GetWhatImpl().c_str();
		} catch (...) {
			LogTracking("ConnectionOpeningExceptionImpl", "GetWhat", __FILE__, __LINE__);
			return L"Unknown error";
		}
	}
	UniquePtr<LocalException> Clone() const {
		return UniquePtr<LocalException>(
			new ConnectionOpeningExceptionImpl<Base>(*this));
	}
private:
	const std::wstring & GetWhatImpl() const {
		if (m_what.empty()) {
			WFormat what(
				L"Opening new %4% connection for tunnel %1% to the %2%"
				L" is failed with the error \"%3%\"");
			what % m_tunnelInstanceId;
			what % m_address->GetResourceIdentifier().GetCStr();
			what % m_error->GetWhat();
			what % m_connectionType;
			m_what = what.str();
		}
		return m_what;
	}
private:
	Instance::Id m_tunnelInstanceId;
	SharedPtr<const EndpointAddress> m_address;
	UniquePtr<LocalException> m_error;
	mutable std::wstring m_what;
	const wchar_t *m_connectionType;
};

//////////////////////////////////////////////////////////////////////////

Tunnel::Tunnel(
			const bool isStatic,
			ServerWorker &server,
			SharedPtr<const TunnelRule> rule,
			SharedPtr<Connection> sourceRead,
			SharedPtr<Connection> sourceWrite)
		: m_isStatic(isStatic),
		m_server(server),
		m_rule(rule),
		m_connectionsToClose(0),
		m_setupComplitedConnections(0),
		m_buffer(new TunnelBuffer),
		m_source(sourceRead, sourceWrite),
		m_destinationIndex(0),
		m_closedConnections(0),
		m_isDead(false) {
	m_destination = CreateDestinationConnections(m_destinationIndex);
	Log::GetInstance().AppendDebug("Outcoming connection created for %1%.", GetInstanceId());
	Init();
}

void Tunnel::Init() {

	using boost::bind;
	using boost::function;

	SharedPtr<TunnelConnectionSignal> sourceDataTransferSignal(
		new TunnelConnectionSignal(*const_cast<Tunnel *>(this)));
	SharedPtr<TunnelConnectionSignal> destinationDataTransferSignal(
		new TunnelConnectionSignal(*const_cast<Tunnel *>(this)));

	std::list<Connection *> connectionsToSetup;
	std::list<SharedPtr<const Listener> > listeners;

	try {

		{
			const TunnelConnectionSignal::OnAllConnectionsSetupCompletedSlot slot
				= boost::bind(&Tunnel::OnConnectionSetup, const_cast<Tunnel *>(this), _1);
			sourceDataTransferSignal->ConnectToOnConnectionSetupCompleted(slot);
			destinationDataTransferSignal->ConnectToOnConnectionSetupCompleted(slot);
		}

		{
			const TunnelConnectionSignal::OnConnectionCloseSlot slot
				= boost::bind(&Tunnel::OnConnectionClose, this, _1);
			sourceDataTransferSignal->ConnectToOnConnectionCloseSignal(slot);
			destinationDataTransferSignal->ConnectToOnConnectionCloseSignal(slot);
		}

		{
			const TunnelConnectionSignal::OnConnectionClosedSlot slot
				= boost::bind(&Tunnel::OnConnectionClosed, this, _1);
			sourceDataTransferSignal->ConnectToOnConnectionClosedSignal(slot);
			destinationDataTransferSignal->ConnectToOnConnectionClosedSignal(slot);
		}

		sourceDataTransferSignal->ConnectToOnMessageBlockSentSignal(
			boost::bind(&Connection::OnMessageBlockSent, &GetOutcomingReadConnection(), _1));
		destinationDataTransferSignal->ConnectToOnMessageBlockSentSignal(
			boost::bind(&Connection::OnMessageBlockSent, &GetIncomingReadConnection(), _1));

		ModulesFactory::Ref factory = ModulesFactory::GetInstance();

		ListenerBinder listenerBinderServer(listeners, *sourceDataTransferSignal);
		boost::function<void(SharedPtr<Listener>)> listenerBinder(
			boost::bind(&ListenerBinder::Bind, &listenerBinderServer, _1));
		factory.CreatePostListeners(
			m_server.GetServer(),
			*m_rule,
			GetIncomingReadConnection(),
			GetOutcomingWriteConnection(),
			listenerBinder);
		factory.CreatePreListeners(
			m_server.GetServer(),
			*m_rule,
			GetOutcomingReadConnection(),
			GetIncomingWriteConnection(),
			listenerBinder);
		sourceDataTransferSignal->ConnectToOnNewMessageBlockSignal(
			boost::bind(&Connection::SendToRemote, &GetOutcomingWriteConnection(), _1));

		listenerBinderServer.SetSignal(*destinationDataTransferSignal);
		factory.CreatePostListeners(
			m_server.GetServer(),
			*m_rule,
			GetOutcomingReadConnection(),
			GetIncomingWriteConnection(),
			listenerBinder);
		factory.CreatePreListeners(
			m_server.GetServer(),
			*m_rule,
			GetIncomingReadConnection(),
			GetOutcomingWriteConnection(),
			listenerBinder);
		destinationDataTransferSignal->ConnectToOnNewMessageBlockSignal(
			boost::bind(&Connection::SendToRemote, &GetIncomingWriteConnection(), _1));

		if (&GetIncomingReadConnection() == &GetIncomingWriteConnection()) {
			GetIncomingReadConnection().Open(
				sourceDataTransferSignal,
				Connection::MODE_READ_WRITE);
			if (!GetIncomingReadConnection().IsSetupCompleted()) {
				connectionsToSetup.push_back(&GetIncomingReadConnection());
			}
			Log::GetInstance().AppendDebug(
				"Incoming connection read/write stream opened.");
		} else {
			GetIncomingReadConnection().Open(
				sourceDataTransferSignal,
				Connection::MODE_READ);
			if (!GetIncomingReadConnection().IsSetupCompleted()) {
				connectionsToSetup.push_back(&GetIncomingReadConnection());
			}
			Log::GetInstance().AppendDebug(
				"Incoming connection read stream opened.");
			GetIncomingWriteConnection().Open(
				sourceDataTransferSignal,
				Connection::MODE_WRITE);
			if (!GetIncomingWriteConnection().IsSetupCompleted()) {
				connectionsToSetup.push_back(&GetIncomingWriteConnection());
			}
			Log::GetInstance().AppendDebug(
				"Incoming connection write stream opened.");
		}
		if (&GetOutcomingReadConnection() == &GetOutcomingWriteConnection()) {
			GetOutcomingReadConnection().Open(
				destinationDataTransferSignal,
				Connection::MODE_READ_WRITE);
			if (!GetOutcomingReadConnection().IsSetupCompleted()) {
				connectionsToSetup.push_back(&GetOutcomingReadConnection());
			}
			Log::GetInstance().AppendDebug(
				"Outcoming connection read/write stream opened.");
		} else {
			GetOutcomingReadConnection().Open(
				destinationDataTransferSignal,
				Connection::MODE_READ);
			if (!GetOutcomingReadConnection().IsSetupCompleted()) {
				connectionsToSetup.push_back(&GetOutcomingReadConnection());
			}
			Log::GetInstance().AppendDebug(
				"Outcoming connection read stream opened.");
			GetOutcomingWriteConnection().Open(
				destinationDataTransferSignal,
				Connection::MODE_WRITE);
			if (!GetOutcomingWriteConnection().IsSetupCompleted()) {
				connectionsToSetup.push_back(&GetOutcomingWriteConnection());
			}
			Log::GetInstance().AppendDebug(
				"Outcoming connection write stream opened.");
		}

	} catch (...) {
		try {
			//! @todo: not exception-safe code! should be reimplemented!!!
			//! @todo: can throw!
			destinationDataTransferSignal->DisconnectAll();
			//! @todo: can throw!
			sourceDataTransferSignal->DisconnectAll();
		} catch (...) {
			assert(false);
		}
		throw;
	}

	m_sourceDataTransferSignal.Swap(sourceDataTransferSignal);
	m_destinationDataTransferSignal.Swap(destinationDataTransferSignal);
	m_connectionsToSetup = connectionsToSetup;
	m_connectionsToClose += m_connectionsToSetup.size();
	m_listeners.swap(listeners);
	m_setupComplitedConnections = 0;

	ReportOpened();

}

Tunnel::~Tunnel() throw() {
	//! @todo: WARNING! this is is not exception-safe code and it should be reimplemented!
	try {
		m_sourceDataTransferSignal->DisconnectDataTransfer();
		m_destinationDataTransferSignal->DisconnectDataTransfer();
	} catch (...) {
		assert(false);
	}
	ReadWriteConnections().Swap(m_source);
	ReadWriteConnections().Swap(m_destination);
	assert(m_closedConnections <= m_connectionsToClose);
	while (m_closedConnections < m_connectionsToClose) {
		// FIXME - reimplement with condition
		Log::GetInstance().AppendDebug(
			"Not all connections closed in tunnel %1%, wait for scheduled...",
			GetInstanceId());
		ACE_OS::sleep(ACE_Time_Value(0, 500 * 1000));
	}
	ReportClosed();
}

Tunnel::ReadWriteConnections Tunnel::CreateDestinationConnections(
			size_t &destinationIndex)
		const {

	const RuleEndpointCollection &destinations = m_rule->GetDestinations();
	const size_t destinationsNumber = destinations.GetSize();
	assert(destinationsNumber > 0);
	assert(destinationIndex < destinationsNumber);

	for (size_t i = destinationIndex; i < destinationsNumber; ++i) {
		const RuleEndpoint &endpoint = destinations[i];
		if (endpoint.IsCombined()) {
			SharedPtr<const EndpointAddress> address = endpoint.GetCombinedAddress();
			assert(address != 0);
			try {
				SharedPtr<Connection> connection(
					address->CreateRemoteConnection(endpoint, address).Release());
				destinationIndex = i;
				return ReadWriteConnections(connection, connection);
			} catch (const TunnelEx::ConnectionOpeningException &ex) {
				if (!((i + 1) >= destinationsNumber)) {
					ReportOpenError(*this, *address, ex);
				} else {
					typedef ConnectionOpeningExceptionImpl<
							DestinationConnectionOpeningException>
						ExceptionImpl;
					throw ExceptionImpl(*this, address, ex, L"outcoming ");
				}
			}
		} else {
			{
				Licenses &licenses = const_cast<Tunnel *>(this)->GetLicenses();
				Licenses::Lock licensesLock(licenses.rwMutex);
				if (!licenses.rwSplitLicense.IsFeatureAvailable(true)) {
					Log::GetInstance().AppendWarn(
						"Could not open connection with I/O channels separation."
							" The functionality you have requested requires"
							" a License Upgrade. Please purchase a License that"
							" will enable this feature at http://" TUNNELEX_DOMAIN "/order"
							" or get free trial at http://" TUNNELEX_DOMAIN "/order/trial.");
					throw LocalException(
						L"Could not open connection, License Upgrade required");
				}
			}
			SharedPtr<const EndpointAddress> readAddress = endpoint.GetReadAddress();
			assert(readAddress != 0);
			SharedPtr<const EndpointAddress> writeAddress = endpoint.GetWriteAddress();
			assert(writeAddress != 0);
			SharedPtr<Connection> readConnection;
			SharedPtr<Connection> writeConnection;
			try {
				readConnection.Reset(
						readAddress->CreateRemoteConnection(
							endpoint,
							readAddress)
						.Release());
				writeConnection.Reset(
					writeAddress->CreateRemoteConnection(
							endpoint,
							writeAddress)
						.Release());
				destinationIndex = i;
				return ReadWriteConnections(readConnection, writeConnection);
			} catch (const TunnelEx::ConnectionOpeningException &ex) {
				const SharedPtr<const EndpointAddress> errorAddress = readConnection
					?	readAddress
					:	writeAddress;
				if (!((i + 1) >= destinationsNumber)) {
					ReportOpenError(*this, *errorAddress, ex);
				} else {
					typedef ConnectionOpeningExceptionImpl<
						DestinationConnectionOpeningException>
						ExceptionImpl;
					throw ExceptionImpl(*this, errorAddress, ex, L"outcoming ");
				}
			}
		}
	}

	WFormat message(L"Could not open new outcoming connection for %1% - destination list is empty");
	message % GetInstanceId();
	throw TunnelEx::ConnectionOpeningException(message.str().c_str());
	
}

void Tunnel::ReportOpened() const {

	if (!Log::GetInstance().IsInfoRegistrationOn()) {
		return;
	}

	struct Appender : private boost::noncopyable {

		explicit Appender(std::ostringstream &streamIn)
				: stream(streamIn) {
			//...//
		}

		void AppendRemote(const Connection &connection, bool isRead, bool *isRemote = 0) {
			const UniquePtr<const EndpointAddress> remoteAddress
				= connection.GetRemoteAddress();
			if (isRemote) {
				*isRemote = remoteAddress;
			}
			if (remoteAddress) {
				stream
					<< ConvertString(remoteAddress->GetResourceIdentifier(), buffer).GetCStr();
			} else {
				AppendLocal(connection, isRead);
			}
		}

		void AppendLocal(const Connection &connection, bool isRead) {
			const Endpoint &ruleEndpoint = connection.GetRuleEndpoint();
			const WString &identifier = !ruleEndpoint.IsCombined()
				? isRead
					? ruleEndpoint.GetReadResourceIdentifier()
					: ruleEndpoint.GetWriteResourceIdentifier()
				: ruleEndpoint.GetCombinedResourceIdentifier();
			stream << ConvertString(identifier, buffer).GetCStr();
		}

		std::ostringstream &stream;
		String buffer;

	};

	std::ostringstream message;
	Appender appender(message);

	message << "Created tunnel";
	if (!m_rule->GetName().IsEmpty()) {
		message
			<< " \""
			<< ConvertString(m_rule->GetName(), appender.buffer).GetCStr()
			<< '\"';
	}
	message << ": ";

	bool isReadRemote = false;
	bool isWriteRemote = false;

	appender.AppendRemote(GetIncomingReadConnection(), true, &isReadRemote);
	if (&GetIncomingReadConnection() != &GetIncomingWriteConnection()) {
		message << "(write: ";
		appender.AppendRemote(GetIncomingWriteConnection(), false, &isWriteRemote);
		message << ")";
	}
	if (isReadRemote || isWriteRemote) {
		message << " > ";
		appender.AppendLocal(GetIncomingReadConnection(), true);
		if (&GetIncomingReadConnection() != &GetIncomingWriteConnection()) {
			message << "(write: ";
			appender.AppendLocal(GetIncomingWriteConnection(), false);
			message << ")";
		}
	}
	message << " < " << m_server.GetServer().GetName().GetCStr() << " > ";
	appender.AppendRemote(GetOutcomingReadConnection(), true);
	if (&GetOutcomingReadConnection() != &GetOutcomingWriteConnection()) {
		message << "(write: ";
		appender.AppendRemote(GetOutcomingWriteConnection(), false);
		message << ")";
	}
	
	message << " (" << GetInstanceId() << ':';
	if (&GetIncomingReadConnection() == &GetIncomingWriteConnection()) {
		message << GetIncomingReadConnection().GetInstanceId();
	} else {
		message
			<< GetIncomingReadConnection().GetInstanceId()
			<< '>' << GetIncomingWriteConnection().GetInstanceId() ;
	}
	message << "<>";
	if (&GetOutcomingReadConnection() == &GetOutcomingWriteConnection()) {
		message << GetOutcomingReadConnection().GetInstanceId();
	} else {
		message
			<< GetOutcomingReadConnection().GetInstanceId()
			<< '>' << GetOutcomingWriteConnection().GetInstanceId();
	}
	message << ')';

	Log::GetInstance().AppendInfo(message.str());

}

void Tunnel::ReportClosed() const throw() {
	if (!Log::GetInstance().IsInfoRegistrationOn()) {
		return;
	}
	try {
		Format message("Closed tunnel %1%.");
		message % GetInstanceId();
		Log::GetInstance().AppendInfo(message.str());
	} catch (...) {
		assert(false);
	}
}

ACE_Proactor & Tunnel::GetProactor() {
	return m_server.GetProactor();
}

void Tunnel::StartSetup() {
	Log::GetInstance().AppendDebug(
		"Starting setup %1% connections in tunnel %2%...",
		m_connectionsToSetup.size(),
		GetInstanceId());
	foreach (Connection *const connection, m_connectionsToSetup) {
		connection->StartSetup();
	}
}

void Tunnel::StartRead() {
	if (&GetIncomingReadConnection() == &GetIncomingWriteConnection()) {
		GetIncomingReadConnection().StartReadRemote();
	} else {
		GetIncomingReadConnection().StartReadRemote();
		GetIncomingWriteConnection().StartReadRemote();
	}
	if (&GetOutcomingReadConnection() == &GetOutcomingWriteConnection()) {
		GetOutcomingReadConnection().StartReadRemote();
	} else {
		GetOutcomingReadConnection().StartReadRemote();
		GetOutcomingWriteConnection().StartReadRemote();
	}
	Log::GetInstance().AppendDebug(
		"Started reading data for tunnel %1%.",
		GetInstanceId());
}

void Tunnel::OnConnectionSetup(Instance::Id instanceId) {
	assert(m_setupComplitedConnections < m_connectionsToSetup.size());
	++m_setupComplitedConnections;
	Log::GetInstance().AppendDebug(
		"Connection %4% setup completed in tunnel %1% (connections: %2%, already completed: %3%).",
		GetInstanceId(),
		m_connectionsToSetup.size(),
		m_setupComplitedConnections,
		instanceId);
	if (m_setupComplitedConnections >= m_connectionsToSetup.size()) {
		StartRead();
	}
}

void Tunnel::OnConnectionClose(Instance::Id instanceId) {
	assert(m_closedConnections < m_connectionsToClose);
	Log::GetInstance().AppendDebug(
		"Closing connection %1% in tunnel %2% (connections: %3%, already closed: %4%).",
		instanceId,
		GetInstanceId(),
		m_connectionsToClose,
		m_closedConnections);
	m_server.CloseTunnel(GetInstanceId());
}

void Tunnel::OnConnectionClosed(Instance::Id instanceId) {
	assert(m_closedConnections < m_connectionsToClose);
	Log::GetInstance().AppendDebug(
		"Connection %1% closed in tunnel %2% (connections: %3%, already closed: %4%).",
		instanceId,
		GetInstanceId(),
		m_connectionsToClose,
		m_closedConnections);
	Interlocked::Increment(&m_closedConnections);
	// FIXME: shot here d-or condition or not?
	// object can be deleted here!
}

Tunnel::Licenses & Tunnel::GetLicenses() {
	static Licenses licenses;
	return licenses;
}

bool Tunnel::Switch(
			const boost::optional<SharedPtr<Connection> > &sourceRead,
			const boost::optional<SharedPtr<Connection> > &sourceWrite) {

	Log::GetInstance().AppendDebug("Switching tunnel %1%...", GetInstanceId());

	assert(sourceRead || sourceWrite || IsDestinationSetupFailed());

	const bool isDestinationSetupFailed = IsDestinationSetupFailed();

	try {
		//! @todo: can throw!
		m_destinationDataTransferSignal->DisconnectDataTransfer();
		//! @todo: can throw!
		m_sourceDataTransferSignal->DisconnectDataTransfer();
	} catch (...) {
		assert(false);
	}

	ReadWriteConnections source;
	source.Swap(m_source);

	if (sourceRead) {
		source.read = *sourceRead;
	}
	if (sourceWrite) {
		source.write = *sourceWrite;
	}

	ReadWriteConnections destination;
	destination.Swap(m_destination);
	unsigned int destinationIndex = m_destinationIndex;

	if (isDestinationSetupFailed) {
		bool isReopened = false;
		bool isReopenedFailed = false;
		if (destination.read->IsSetupFailed()) {
			const SharedPtr<const EndpointAddress> address
				= destination.read->GetRuleEndpointAddress();
			if (address->IsReadyToRecreateRemoteConnection()) {
				Log::GetInstance().AppendDebug(
					"Reopening outcoming first connection for tunnel %1%...",
					GetInstanceId());
				try {
					const RuleEndpoint &endpoint
						= destination.read->GetRuleEndpoint();
					destination.read.Reset(
							address->CreateRemoteConnection(endpoint, address)
						.Release());
					if (endpoint.IsCombined()) {
						destination.write = destination.read;
					}
					isReopened = true;
				} catch (const TunnelEx::ConnectionOpeningException &ex) {
					if (m_rule->GetDestinations().GetSize() <= m_destinationIndex + 1) {
						typedef ConnectionOpeningExceptionImpl<
								DestinationConnectionOpeningException>
							ExceptionImpl;
						throw ExceptionImpl(*this, address, ex, L"outcoming ");
					} else {
						ReportOpenError(*this, *address, ex);
						isReopenedFailed = true;
					}
				}
			}
		}
		if (	!isReopenedFailed
				&& !destination.write->GetRuleEndpoint().IsCombined()
				&& destination.write->IsSetupFailed()) {
			const SharedPtr<const EndpointAddress> address
				= destination.write->GetRuleEndpointAddress();
			if (address->IsReadyToRecreateRemoteConnection()) {
				Log::GetInstance().AppendDebug(
					"Reopening outcoming second connection for tunnel %1%...",
					GetInstanceId());
				try {
					destination.write.Reset(
							address->CreateRemoteConnection(
								destination.write->GetRuleEndpoint(),
								address)
						.Release());
					isReopened = true;
				} catch (const TunnelEx::ConnectionOpeningException &ex) {
					if (m_rule->GetDestinations().GetSize() <= m_destinationIndex + 1) {
						typedef ConnectionOpeningExceptionImpl<
								DestinationConnectionOpeningException>
							ExceptionImpl;
						throw ExceptionImpl(*this, address, ex, L"outcoming ");
					} else {
						ReportOpenError(*this, *address, ex);
						isReopenedFailed = true;
					}
				}
			}
		}
		if (!isReopened || isReopenedFailed) {
			ReadWriteConnections().Swap(destination);
			Log::GetInstance().AppendDebug(
				"Trying next destination endpoint for tunnel %1%...",
				GetInstanceId());
			if (m_rule->GetDestinations().GetSize() <= m_destinationIndex + 1) {
				Log::GetInstance().AppendDebug(
					"No more destination endpoints for tunnel %1%.",
					GetInstanceId());
				return false;
			}
			unsigned int destinationIndexTmp = destinationIndex + 1;
			destination = CreateDestinationConnections(destinationIndexTmp);
			destinationIndex = destinationIndexTmp;
		}
	} else {
		assert(sourceRead || sourceRead);
	}
	source.Swap(m_source);
	destination.Swap(m_destination);
	m_destinationIndex = destinationIndex;

	Log::GetInstance().AppendDebug("Outcoming connection created for %1%.", GetInstanceId());

	Init();

	return true;

}

bool Tunnel::IsSetupFailed() const {
	return
		GetOutcomingReadConnection().IsSetupFailed()
		|| GetIncomingReadConnection().IsSetupFailed()
		|| GetOutcomingWriteConnection().IsSetupFailed()
		|| GetIncomingWriteConnection().IsSetupFailed();
}

bool Tunnel::IsSourceSetupFailed() const {
	return GetIncomingReadConnection().IsSetupFailed()
		|| GetIncomingWriteConnection().IsSetupFailed();
}

bool Tunnel::IsDestinationSetupFailed() const {
	return GetOutcomingReadConnection().IsSetupFailed()
		|| GetOutcomingWriteConnection().IsSetupFailed();
}
