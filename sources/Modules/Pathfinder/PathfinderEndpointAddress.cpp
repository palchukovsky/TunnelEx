/**************************************************************************
 *   Created: 2010/03/21 23:54
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "PathfinderEndpointAddress.hpp"
#include "Service.hpp"

#include "Modules/Inet/ProxyExceptions.hpp"

#include "Core/Connection.hpp" // to avoid warning C4150 in std::auto_ptr or TunnelEx::UniquePtr
#include "Core/Exceptions.hpp"
#include "Core/Log.hpp"
#include "Core/Server.hpp"

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;
using namespace TunnelEx;
using Mods::Inet::NetworkPort;
using Mods::Inet::Proxy;
using Mods::Inet::ProxyList;
using namespace TunnelEx::Mods::Pathfinder;

//////////////////////////////////////////////////////////////////////////

class PathfinderEndpointAddress::Implementation {

public:

	Implementation()
			: m_isPathfinderNode(false),
			m_isSetupCompleted(false) {
		//...//					
	}

private:

	Implementation & operator =(const Implementation &);

public:

	WString m_resourceIdentifier;
	bool m_isPathfinderNode;
	bool m_isSetupCompleted;
	Inet::ProxyList m_proxy;

};

//////////////////////////////////////////////////////////////////////////

PathfinderEndpointAddress::PathfinderEndpointAddress()
		: m_pimpl(new Implementation) {
	//...//
}

PathfinderEndpointAddress::PathfinderEndpointAddress(
			const PathfinderEndpointAddress &rhs)
		: TcpEndpointAddress(rhs),
		m_pimpl(new Implementation(*rhs.m_pimpl)) {
	//...//
}
		
PathfinderEndpointAddress::PathfinderEndpointAddress(
			const WString &resourceIdentifier,
			Server::ConstPtr server /*= 0*/)
		: TcpEndpointAddress(resourceIdentifier, server),
		m_pimpl(new Implementation) {
	//...//
}

PathfinderEndpointAddress::~PathfinderEndpointAddress() throw() {
	delete m_pimpl;
}

const PathfinderEndpointAddress & PathfinderEndpointAddress::operator =(
			const PathfinderEndpointAddress &rhs) {
	PathfinderEndpointAddress(rhs).Swap(*this);
	return *this;
}

void PathfinderEndpointAddress::Swap(PathfinderEndpointAddress &rhs) throw() {
	TcpEndpointAddress::Swap(rhs);
	m_pimpl->m_resourceIdentifier.Swap(rhs.m_pimpl->m_resourceIdentifier);
}

UniquePtr<Acceptor> PathfinderEndpointAddress::OpenForIncomingConnections(
			const RuleEndpoint &,
			SharedPtr<const EndpointAddress>)
		const {
	throw EndpointHasNotMultiClientsTypeException(
		L"Could not create acceptor for pathfinder");
}

const WString & PathfinderEndpointAddress::GetResourceIdentifier() const {
	if (m_pimpl->m_resourceIdentifier.IsEmpty()) {
		WString resourceIdentifier = TcpEndpointAddress::GetResourceIdentifier();
		ConvertTcpToPathfinder(resourceIdentifier);
		m_pimpl->m_resourceIdentifier.Swap(resourceIdentifier);
	}
	assert(!m_pimpl->m_resourceIdentifier.IsEmpty());
	return m_pimpl->m_resourceIdentifier;
}

bool PathfinderEndpointAddress::IsHasMultiClientsType(void) const {
	return false;
}

UniquePtr<EndpointAddress> PathfinderEndpointAddress::Clone() const {
	UniquePtr<PathfinderEndpointAddress> result(new PathfinderEndpointAddress(*this));
	if (result->m_pimpl->m_isPathfinderNode) {
		assert(GetProxyList().size() > 0);
		if (result->m_pimpl->m_isSetupCompleted) {
			result->m_pimpl->m_isSetupCompleted = false;
		}
	}
	return result;
}

UniquePtr<Connection> PathfinderEndpointAddress::CreateConnection(
			const RuleEndpoint &endpoint,
			SharedPtr<const EndpointAddress> originalAddress) 
		const {

	if (m_pimpl->m_isPathfinderNode) {
		assert(GetProxyList().size() > 0);
		assert(this == originalAddress.Get());
		if (!m_pimpl->m_isSetupCompleted) {
			return TcpEndpointAddress::CreateConnection(endpoint, originalAddress);
		} else {
			m_pimpl->m_isSetupCompleted = false;
			ProxyList proxyList(GetProxyList());
			assert(m_pimpl->m_proxy.size() > 0);
			while (m_pimpl->m_proxy.size() > 0) {
				*proxyList.rbegin() = *m_pimpl->m_proxy.begin();
				m_pimpl->m_proxy.pop_front();
				const_cast<PathfinderEndpointAddress *>(this)->SetProxyList(proxyList);
				Log::GetInstance().AppendDebug(
					"Pathfinder tries to make connection through %1%:%2%...",
					ConvertString<String>(proxyList.rbegin()->host.c_str()).GetCStr(),
					proxyList.rbegin()->port);
				try {
					return TcpEndpointAddress::CreateConnection(
						endpoint,
						originalAddress);
				} catch (const TunnelEx::ConnectionOpeningException &ex) {
					if (m_pimpl->m_proxy.size() > 0) {
						Service::instance()->ReportConnectError(
							*this,
							*proxyList.rbegin());
						Log::GetInstance().AppendDebug(ex.GetWhat());
					} else {
						throw;
					}
				}
			}
			assert(false);
		}
	}
	
	m_pimpl->m_isSetupCompleted = false;

	ProxyList proxyList(GetProxyList());
	proxyList.push_back(Proxy());

	SharedPtr<PathfinderEndpointAddress> address;
	{
		UniquePtr<EndpointAddress> originalAddressClone
			= originalAddress->Clone();
		address.Reset(
			boost::polymorphic_downcast<PathfinderEndpointAddress *>(
				originalAddressClone.Get()));
		originalAddressClone.Release();
	}
	address->m_pimpl->m_isPathfinderNode = true;

	for (bool isOnlineRequest = false; !isOnlineRequest; ) {

		try {
			isOnlineRequest
				= Service::instance()->GetProxy(*this, address->m_pimpl->m_proxy);
			assert(address->m_pimpl->m_proxy.size() > 0);
		} catch (const TunnelEx::Mods::Pathfinder::LicensingException &) {
			throw;
		} catch (const TunnelEx::Mods::Pathfinder::ServiceException &ex) {
			Log::GetInstance().AppendDebug(ex.GetWhat());
			Log::GetInstance().AppendDebug("Pathfinder tries direct connection...");
			try {
				return TcpEndpointAddress::CreateConnection(endpoint, originalAddress);
			} catch (const TunnelEx::ConnectionOpeningException &ex) {
				Log::GetInstance().AppendDebug(ex.GetWhat());
			}
			throw;
		}

		while (address->m_pimpl->m_proxy.size() > 0) {
			*proxyList.rbegin() = *address->m_pimpl->m_proxy.begin();
			address->m_pimpl->m_proxy.pop_front();
			address->SetProxyList(proxyList);
			Log::GetInstance().AppendDebug(
				"Pathfinder tries to make connection through %1%:%2%...",
				ConvertString<String>(proxyList.rbegin()->host.c_str()).GetCStr(),
				proxyList.rbegin()->port);
			try {
				return address->CreateConnection(endpoint, address);
			} catch (const TunnelEx::ConnectionOpeningException &ex) {
				Service::instance()->ReportConnectError(
					*address,
					*proxyList.rbegin());
				Log::GetInstance().AppendDebug(ex.GetWhat());
			}
		}

	}

	Log::GetInstance().AppendDebug("Pathfinder tries direct connection...");
	try {
		return TcpEndpointAddress::CreateConnection(endpoint, originalAddress);
	} catch (const TunnelEx::ConnectionOpeningException &ex) {
		Log::GetInstance().AppendDebug(ex.GetWhat());
		throw TunnelEx::ConnectionOpeningException(
			L"Pathfinder service could not find network path, endpoint is unavailable");
	}
	
}

const ACE_INET_Addr * PathfinderEndpointAddress::GetFirstProxyAceInetAddr(
			bool isHttpTunnelingAvailable,
			bool /*isCascadeTunnelingAvailable*/)
		const {
	return Base::GetFirstProxyAceInetAddr(isHttpTunnelingAvailable, true);
}

bool PathfinderEndpointAddress::IsReadyToRecreateConnection() const {
	if (m_pimpl->m_isSetupCompleted && m_pimpl->m_proxy.size() > 0) {
		assert(m_pimpl->m_isPathfinderNode);
		return true;
	} else  {
		return false;
	}
}

void PathfinderEndpointAddress::ClearResourceIdentifierCache() throw() {
	TcpEndpointAddress::ClearResourceIdentifierCache();
	WString().Swap(m_pimpl->m_resourceIdentifier);
}

void PathfinderEndpointAddress::ConvertTcpToPathfinder(WString &resourceIdentifier) {
	assert(boost::starts_with(std::wstring(resourceIdentifier.GetCStr()), L"tcp://"));
	WString result = L"pathfinder";
	result += resourceIdentifier.SubStr(3);
	assert(boost::starts_with(std::wstring(result.GetCStr()), L"pathfinder://"));
	assert(!result.IsEmpty());
	result.Swap(resourceIdentifier);
}

WString PathfinderEndpointAddress::CreateResourceIdentifier(
			const std::wstring &host,
			NetworkPort port,
			const SslCertificateId &certificate,
			const SslCertificateIdCollection &remoteCertificates) {
	WString result = TcpEndpointAddress::CreateResourceIdentifier(
		host,
		port,
		certificate,
		remoteCertificates);
	ConvertTcpToPathfinder(result);
	return result;
}

WString PathfinderEndpointAddress::CreateResourceIdentifier(
			const std::wstring &host,
			NetworkPort port,
			const SslCertificateId &certificate,
			const SslCertificateIdCollection &remoteCertificates,
			const ProxyList &proxyList) {
	WString result = TcpEndpointAddress::CreateResourceIdentifier(
		host,
		port,
		certificate,
		remoteCertificates,
		proxyList);
	ConvertTcpToPathfinder(result);
	return result;
}

void PathfinderEndpointAddress::StatConnectionSetupCompleting() const throw() {
	TcpEndpointAddress::StatConnectionSetupCompleting();
	if (!m_pimpl->m_isPathfinderNode) {
		assert(m_pimpl->m_proxy.size() == 0);
		return;
	}
	assert(m_pimpl->m_isSetupCompleted == false);
	m_pimpl->m_isSetupCompleted = true;
	assert(GetProxyList().size() > 0);
	Service::instance()->ReportSuccess(*this, *GetProxyList().rbegin());
}

void PathfinderEndpointAddress::StatConnectionSetupCanceling() const throw() {
	TcpEndpointAddress::StatConnectionSetupCanceling();
	if (!m_pimpl->m_isPathfinderNode) {
		assert(m_pimpl->m_proxy.size() == 0);
		return;
	}
	assert(m_pimpl->m_isSetupCompleted == false);
	m_pimpl->m_isSetupCompleted = true;
	assert(GetProxyList().size() > 0);
	Service::instance()->ReportWorkingError(*this, *GetProxyList().rbegin());
}

void PathfinderEndpointAddress::StatConnectionSetupCanceling(
			const WString &reason)
		const 
		throw() {
	// this reason can be generated only after error at proxy setup.
	Log::GetInstance().AppendDebug(ConvertString<String>(reason).GetCStr());
	StatConnectionSetupCanceling();
}

//////////////////////////////////////////////////////////////////////////

namespace TunnelEx { namespace Mods { namespace Pathfinder {

	UniquePtr<EndpointAddress> CreateEndpointAddress(
				Server::ConstRef server,
				const WString &resourceIdentifier) {
		return UniquePtr<EndpointAddress>(
			new PathfinderEndpointAddress(
				resourceIdentifier,
				&server));
	}

} } }
