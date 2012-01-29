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
#include "SmartPtr.hpp"
#include "TunnelConnectionSignal.hpp"
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
		Log::GetInstance().AppendDebugEx(
			[&tunnel, &remoteAddr, &error]() -> Format {
				Format message(
					"Failed to open new outcoming connection to %1% in tunnel %2%:"
						" \"%3%\"."
						" Will try next endpoint...");
				message
					% remoteAddr.GetResourceIdentifier()
					% tunnel.GetInstanceId()
					% TunnelEx::ConvertString<String>(error.GetWhat());
				return message;
			});
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
			Format message(
				"Unknown system error occurred: %1%:%2%."
					" Please restart the service"
					" and contact product support to resolve this issue."
					" %3% %4%");
			message
				% __FILE__ % __LINE__
				% TUNNELEX_NAME % TUNNELEX_BUILD_IDENTITY;
			Log::GetInstance().AppendFatalError(message.str());
			assert(false);
			return L"Unknown error";
		}
	}
	AutoPtr<LocalException> Clone() const {
		return AutoPtr<LocalException>(
			new ConnectionOpeningExceptionImpl<Base>(*this));
	}
private:
	const std::wstring & GetWhatImpl() const {
		if (m_what.empty()) {
			WFormat what(L"Failed to open %1%connection to \"%2%\" for tunnel %3%: \"%4%\"");
			what
				% m_connectionType
				% m_address->GetResourceIdentifier()
				% m_tunnelInstanceId
				% m_error->GetWhat();
			m_what = what.str();
		}
		return m_what;
	}
private:
	Instance::Id m_tunnelInstanceId;
	SharedPtr<const EndpointAddress> m_address;
	AutoPtr<LocalException> m_error;
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
		m_source(sourceRead, sourceWrite),
		m_destinationIndex(0),
		m_closedConnections(0),
		m_allConnectionsClosedCondition(m_allConnectionsClosedMutex),
		m_isDead(false) {
	m_destination = CreateDestinationConnections(m_destinationIndex);
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
		} else {
			GetIncomingReadConnection().Open(
				sourceDataTransferSignal,
				Connection::MODE_READ);
			if (!GetIncomingReadConnection().IsSetupCompleted()) {
				connectionsToSetup.push_back(&GetIncomingReadConnection());
			}
			GetIncomingWriteConnection().Open(
				sourceDataTransferSignal,
				Connection::MODE_WRITE);
			if (!GetIncomingWriteConnection().IsSetupCompleted()) {
				connectionsToSetup.push_back(&GetIncomingWriteConnection());
			}
		}
		if (&GetOutcomingReadConnection() == &GetOutcomingWriteConnection()) {
			GetOutcomingReadConnection().Open(
				destinationDataTransferSignal,
				Connection::MODE_READ_WRITE);
			if (!GetOutcomingReadConnection().IsSetupCompleted()) {
				connectionsToSetup.push_back(&GetOutcomingReadConnection());
			}
		} else {
			GetOutcomingReadConnection().Open(
				destinationDataTransferSignal,
				Connection::MODE_READ);
			if (!GetOutcomingReadConnection().IsSetupCompleted()) {
				connectionsToSetup.push_back(&GetOutcomingReadConnection());
			}
			GetOutcomingWriteConnection().Open(
				destinationDataTransferSignal,
				Connection::MODE_WRITE);
			if (!GetOutcomingWriteConnection().IsSetupCompleted()) {
				connectionsToSetup.push_back(&GetOutcomingWriteConnection());
			}
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
	if (!m_isDead) {
		MarkAsDead();
	}
	{
		GetIncomingReadConnection().Close();
		if (&GetIncomingReadConnection() != &GetIncomingWriteConnection()) {
			GetIncomingWriteConnection().Close();
		}
		GetOutcomingReadConnection().Close();
		if (&GetOutcomingReadConnection() != &GetOutcomingWriteConnection()) {
			GetOutcomingWriteConnection().Close();
		}
	}
	{
		AllConnectionsClosedLock lock(m_allConnectionsClosedMutex);
		assert(m_closedConnections <= m_connectionsToClose);
		for (auto i = 1; m_closedConnections < m_connectionsToClose; ++i) {
			if (i > 1) {
	#			ifdef DEV_VER
				{
					Format message(
						"Not all connections closed in tunnel %1%, waiting for the %2% time for scheduled...");
					message % GetInstanceId() % i;
					Log::GetInstance().AppendWarn(message.str());
				}
	#			else
					Log::GetInstance().AppendDebug(
						"Not all connections closed in tunnel %1%, waiting for the %2% time for scheduled...",
						GetInstanceId(),
						i);
	#			endif
			}
			const ACE_Time_Value waitUntil(ACE_OS::gettimeofday() + ACE_Time_Value(60));
			verify(m_allConnectionsClosedCondition.wait(&waitUntil) != -1 || errno == ETIME);
		}
		assert(m_closedConnections == m_connectionsToClose);
	}
	ReportClosed();
}

void Tunnel::DisconnectDataTransferSignals() throw() {
	try {
		m_sourceDataTransferSignal->DisconnectDataTransfer();
		m_destinationDataTransferSignal->DisconnectDataTransfer();
	} catch (...) {
		Format message(
			"Unknown system error occurred for tunnel %5%:"
				" %1%:%2%."
				" Please restart the service"
				" and contact product support to resolve this issue."
				" %3% %4%");
		message
			% __FILE__ % __LINE__
			% TUNNELEX_NAME % TUNNELEX_BUILD_IDENTITY
			% GetInstanceId();
		Log::GetInstance().AppendFatalError(message.str());
		assert(false);
	}
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

	const bool isSilent = m_rule->IsSilent();
	if (
			(!isSilent && !Log::GetInstance().IsInfoRegistrationOn())
			|| (isSilent && !Log::GetInstance().IsDebugRegistrationOn())) {
		return;
	}

	struct Appender : private boost::noncopyable {

		explicit Appender(std::ostringstream &streamIn)
				: stream(streamIn) {
			//...//
		}

		void AppendRemote(const Connection &connection, bool isRead, bool *isRemote = 0) {
			const AutoPtr<const EndpointAddress> remoteAddress
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

	if (!m_rule->IsSilent()) {
		Log::GetInstance().AppendInfo(message.str());
	} else {
		Log::GetInstance().AppendDebug(message.str());
	}

}

void Tunnel::ReportClosed() const throw() {
	const bool isSilent = m_rule->IsSilent();
	if (
			(!isSilent && !Log::GetInstance().IsInfoRegistrationOn())
			|| (isSilent && !Log::GetInstance().IsDebugRegistrationOn())) {
		return;
	}
	Format message("Closed tunnel %1% with code %2%/%3%.");
	message
		% GetInstanceId()
		% GetIncomingReadConnection().GetCloseCode()
		% GetOutcomingReadConnection().GetCloseCode();
	if (!m_rule->IsSilent()) {
		Log::GetInstance().AppendInfo(message.str());
	} else {
		Log::GetInstance().AppendDebug(message.str());
	}
}

ACE_Proactor & Tunnel::GetProactor() {
	return m_server.GetProactor();
}

void Tunnel::StartSetup() {
	foreach (Connection *const connection, m_connectionsToSetup) {
		connection->StartSetup();
	}
}

void Tunnel::StartRead() {
	if (&GetIncomingReadConnection() == &GetIncomingWriteConnection()) {
		GetIncomingReadConnection().StartReadingRemote();
	} else {
		GetIncomingReadConnection().StartReadingRemote();
		GetIncomingWriteConnection().StartReadingRemote();
	}
	if (&GetOutcomingReadConnection() == &GetOutcomingWriteConnection()) {
		GetOutcomingReadConnection().StartReadingRemote();
	} else {
		GetOutcomingReadConnection().StartReadingRemote();
		GetOutcomingWriteConnection().StartReadingRemote();
	}
}

void Tunnel::OnConnectionSetup(Instance::Id /*instanceId*/) {
	assert(m_setupComplitedConnections < m_connectionsToSetup.size());
	++m_setupComplitedConnections;
	if (m_setupComplitedConnections >= m_connectionsToSetup.size()) {
		StartRead();
	}
}

void Tunnel::OnConnectionClose(Instance::Id /*instanceId*/) {
	m_server.CloseTunnel(GetInstanceId());
}

void Tunnel::OnConnectionClosed(Instance::Id /*instanceId*/) {
	AllConnectionsClosedLock lock(m_allConnectionsClosedMutex);
	assert(m_closedConnections < m_connectionsToClose);
	if (++m_closedConnections >= m_connectionsToClose) {
		m_allConnectionsClosedCondition.broadcast();
	}
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
