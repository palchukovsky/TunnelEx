/**************************************************************************
 *   Created: 2010/03/21 23:54
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: PathfinderEndpointAddress.cpp 1109 2010-12-26 06:33:37Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "PathfinderEndpointAddress.hpp"
#include "Service.hpp"

#include "Modules/Inet/ProxyExceptions.hpp"

#ifdef TEX_PATHFINDER_TEST
#	include "EndpointResourceIdentifierParsers.hpp"
#endif

#include <TunnelEx/Connection.hpp> // to avoid warning C4150 in std::auto_ptr or TunnelEx::UniquePtr
#include <TunnelEx/Exceptions.hpp>
#include <TunnelEx/Log.hpp>
#include <TunnelEx/Server.hpp>

using namespace std;
using namespace boost;
using namespace TunnelEx;
using Mods::Inet::NetworkPort;
using Mods::Inet::Proxy;
using Mods::Inet::ProxyList;
using namespace TunnelEx::Mods::Pathfinder;


#ifdef TEX_PATHFINDER_TEST
	
	namespace {

		template<class Proxy>
		void LogPathfinderHostTestResult(const Proxy &proxy, const wchar_t *const result) {
			filesystem::path logPath = Helpers::GetModuleFilePathA().branch_path();
			logPath /= "PathfinderTest.log";
			wofstream log(logPath.string().c_str(), ios::app);
			log
				<< posix_time::ptime(posix_time::microsec_clock::local_time())
				<< L'\t'
				<< proxy.host << L':' << proxy.port
				<< '\t'
				<< result
				<< endl;
		}

	}

#endif

PathfinderEndpointAddress::PathfinderEndpointAddress()
		: m_isPathfinderNode(false),
#		ifdef TEX_PATHFINDER_TEST
			m_isTest(false),
			m_testIndex(0),
#		endif
		m_isSetupCompleted(false) {
	//...//
}

PathfinderEndpointAddress::PathfinderEndpointAddress(
			const PathfinderEndpointAddress &rhs)
		: TcpEndpointAddress(rhs),
		m_resourceIdentifier(rhs.m_resourceIdentifier),
		m_isPathfinderNode(rhs.m_isPathfinderNode),
#		ifdef TEX_PATHFINDER_TEST
			m_isTest(rhs.m_isTest),
			m_testIndex(rhs.m_testIndex),
#		endif
		m_isSetupCompleted(rhs.m_isSetupCompleted),
		m_proxy(rhs.m_proxy) {
	//...//
}
		
PathfinderEndpointAddress::PathfinderEndpointAddress(
			const WString &resourceIdentifier,
			Server::ConstPtr server /*= 0*/)
		: TcpEndpointAddress(resourceIdentifier, server),
		m_isPathfinderNode(false),
#		ifdef TEX_PATHFINDER_TEST
			m_isTest(false),
			m_testIndex(0),
#		endif
		m_isSetupCompleted(false) {
#	ifdef TEX_PATHFINDER_TEST
	{
		const wstring path(resourceIdentifier.GetCStr());
		Helpers::EndpointResourceIdentifierParsers::UrlSplitConstIterator pathIt
			= make_split_iterator(path, token_finder(is_any_of(L"?&")));
		if (!pathIt.eof() && !(++pathIt).eof()) {
			Helpers::EndpointResourceIdentifierParsers::ParseUrlParam(
				pathIt,
				L"is_test",
				bind(
					&Helpers::EndpointResourceIdentifierParsers::ParseUrlParamValue<bool>,
					_1,
					ref(m_isTest)),
				false);
			if (m_isTest) {
				WFormat message(L"PATHFINDER ENDPOINT PREPARED FOR TESTING (%1%).");
				message % resourceIdentifier.GetCStr();
				Log::GetInstance().AppendWarn(
					ConvertString<String>(message.str().c_str()).GetCStr());
			}
		}
	}
#	endif
}

PathfinderEndpointAddress::~PathfinderEndpointAddress() throw() {
	//...//
}

const PathfinderEndpointAddress & PathfinderEndpointAddress::operator =(
			const PathfinderEndpointAddress &rhs) {
	PathfinderEndpointAddress(rhs).Swap(*this);
	return *this;
}

void PathfinderEndpointAddress::Swap(PathfinderEndpointAddress &rhs) throw() {
	TcpEndpointAddress::Swap(rhs);
	m_resourceIdentifier.Swap(rhs.m_resourceIdentifier);
}

UniquePtr<Acceptor> PathfinderEndpointAddress::OpenForIncomingConnections(
			const RuleEndpoint &,
			SharedPtr<const EndpointAddress>)
		const {
	throw EndpointHasNotMultiClientsTypeException(
		L"Could not create acceptor for pathfinder");
}

const WString & PathfinderEndpointAddress::GetResourceIdentifier() const {
	if (m_resourceIdentifier.IsEmpty()) {
		WString resourceIdentifier = TcpEndpointAddress::GetResourceIdentifier();
		ConvertTcpToPathfinder(resourceIdentifier);
		const_cast<PathfinderEndpointAddress *>(this)
			->m_resourceIdentifier.Swap(resourceIdentifier);
	}
	BOOST_ASSERT(!m_resourceIdentifier.IsEmpty());
	return m_resourceIdentifier;
}

bool PathfinderEndpointAddress::IsHasMultiClientsType(void) const {
	return false;
}

UniquePtr<EndpointAddress> PathfinderEndpointAddress::Clone() const {
	UniquePtr<PathfinderEndpointAddress> result(new PathfinderEndpointAddress(*this));
	if (result->m_isPathfinderNode) {
		BOOST_ASSERT(GetProxyList().size() > 0);
		if (result->m_isSetupCompleted) {
			result->m_isSetupCompleted = false;
		}
	}
	return result;
}

UniquePtr<Connection> PathfinderEndpointAddress::CreateConnection(
			const RuleEndpoint &endpoint,
			SharedPtr<const EndpointAddress> originalAddress) 
		const {

	if (m_isPathfinderNode) {
		BOOST_ASSERT(GetProxyList().size() > 0);
		BOOST_ASSERT(this == originalAddress.Get());
		if (!m_isSetupCompleted) {
			return TcpEndpointAddress::CreateConnection(endpoint, originalAddress);
		} else {
			m_isSetupCompleted = false;
			ProxyList proxyList(GetProxyList());
			BOOST_ASSERT(m_proxy.size() > 0);
#			ifdef TEX_PATHFINDER_TEST
				if (m_isTest) {
					for (size_t i = 0; i < m_testIndex && !m_proxy.empty(); ++i) {
						m_proxy.pop_front();
						if (m_proxy.empty()) {
							throw ConnectionOpeningException(L"Pathfinder tests: no  proxy found");
						}
					}
				}
#			endif
			while (m_proxy.size() > 0) {
				*proxyList.rbegin() = *m_proxy.begin();
				m_proxy.pop_front();
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
#					ifndef TEX_PATHFINDER_TEST
						if (m_proxy.size() > 0) {
							Service::instance()->ReportConnectError(
								*this,
								*proxyList.rbegin());
							Log::GetInstance().AppendDebug(ex.GetWhat());
						} else {
							throw;
						}
#					else
						if (!m_isTest) {
							if (m_proxy.size() > 0) {
								Service::instance()->ReportConnectError(
									*this,
									*proxyList.rbegin());
								Log::GetInstance().AppendDebug(ex.GetWhat());
							} else {
								throw;
							}
						} else {
							LogPathfinderHostTestResult(
								*proxyList.rbegin(),
								L"FAIL to connect");
							if (m_proxy.size() == 0) {
								Log::GetInstance().AppendWarn("Pathfinder tests finished.");
								throw;
							}
						}
#					endif

				}
			}
			BOOST_ASSERT(false);
		}
	}
	
	m_isSetupCompleted = false;

	ProxyList proxyList(GetProxyList());
	proxyList.push_back(Proxy());

	SharedPtr<PathfinderEndpointAddress> address;
	{
		UniquePtr<EndpointAddress> originalAddressClone
			= originalAddress->Clone();
		address.Reset(
			polymorphic_downcast<PathfinderEndpointAddress *>(
				originalAddressClone.Get()));
		originalAddressClone.Release();
	}
	address->m_isPathfinderNode = true;

	for (bool isOnlineRequest = false; !isOnlineRequest; ) {

		try {
#			ifdef TEX_PATHFINDER_TEST
				static Inet::ProxyList testProxyListCache;
				if (!m_isTest || testProxyListCache.empty()) {
					isOnlineRequest
						= Service::instance()->GetProxy(*this, address->m_proxy);
					if (m_isTest) {
						testProxyListCache = address->m_proxy;
						isOnlineRequest = true;
					}
				} else {
					address->m_proxy = testProxyListCache;
					isOnlineRequest = true;
				}
#			else
				isOnlineRequest
					= Service::instance()->GetProxy(*this, address->m_proxy);
#			endif
			BOOST_ASSERT(address->m_proxy.size() > 0);
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

#		ifdef TEX_PATHFINDER_TEST
			if (m_isTest) {
				for (size_t i = 0; i < m_testIndex && !address->m_proxy.empty(); ++i) {
					address->m_proxy.pop_front();
					if (address->m_proxy.empty()) {
						Log::GetInstance().AppendWarn("Pathfinder tests finished.");
					}
				}
			}
#		endif
		while (address->m_proxy.size() > 0) {
			*proxyList.rbegin() = *address->m_proxy.begin();
			address->m_proxy.pop_front();
#			ifdef TEX_PATHFINDER_TEST
				if (m_isTest) {
					++const_cast<PathfinderEndpointAddress *>(this)->m_testIndex;
				}
#			endif
			address->SetProxyList(proxyList);
			Log::GetInstance().AppendDebug(
				"Pathfinder tries to make connection through %1%:%2%...",
				ConvertString<String>(proxyList.rbegin()->host.c_str()).GetCStr(),
				proxyList.rbegin()->port);
			try {
				return address->CreateConnection(endpoint, address);
			} catch (const TunnelEx::ConnectionOpeningException &ex) {
#				ifndef TEX_PATHFINDER_TEST
					Service::instance()->ReportConnectError(
						*address,
						*proxyList.rbegin());
					Log::GetInstance().AppendDebug(ex.GetWhat());
#				else
					if (!m_isTest) {
						Service::instance()->ReportConnectError(
							*address,
							*proxyList.rbegin());
						Log::GetInstance().AppendDebug(ex.GetWhat());
					} else {
						LogPathfinderHostTestResult(
							*proxyList.rbegin(),
							L"FAIL to connect");
					}
#				endif
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
	if (m_isSetupCompleted && m_proxy.size() > 0) {
		BOOST_ASSERT(m_isPathfinderNode);
		return true;
	} else  {
		return false;
	}
}

void PathfinderEndpointAddress::ClearResourceIdentifierCache() throw() {
	TcpEndpointAddress::ClearResourceIdentifierCache();
	WString().Swap(m_resourceIdentifier);
}

void PathfinderEndpointAddress::ConvertTcpToPathfinder(WString &resourceIdentifier) {
	BOOST_ASSERT(starts_with(wstring(resourceIdentifier.GetCStr()), L"tcp://"));
	WString result = L"pathfinder";
	result += resourceIdentifier.SubStr(3);
	BOOST_ASSERT(starts_with(wstring(result.GetCStr()), L"pathfinder://"));
	BOOST_ASSERT(!result.IsEmpty());
	result.Swap(resourceIdentifier);
}

WString PathfinderEndpointAddress::CreateResourceIdentifier(
			const wstring &host,
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
			const wstring &host,
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
	if (!m_isPathfinderNode) {
		BOOST_ASSERT(m_proxy.size() == 0);
		return;
	}
	BOOST_ASSERT(m_isSetupCompleted == false);
	m_isSetupCompleted = true;
	BOOST_ASSERT(GetProxyList().size() > 0);
#	ifndef TEX_PATHFINDER_TEST
		Service::instance()->ReportSuccess(*this, *GetProxyList().rbegin());
#	else
		if (!m_isTest) {
			Service::instance()->ReportSuccess(*this, *GetProxyList().rbegin());
			Inet::ProxyList().swap(m_proxy);
		} else {
			LogPathfinderHostTestResult(*GetProxyList().rbegin(), L"SUCCESS");
		}
#	endif
}

void PathfinderEndpointAddress::StatConnectionSetupCanceling() const throw() {
	TcpEndpointAddress::StatConnectionSetupCanceling();
	if (!m_isPathfinderNode) {
		BOOST_ASSERT(m_proxy.size() == 0);
		return;
	}
	BOOST_ASSERT(m_isSetupCompleted == false);
	m_isSetupCompleted = true;
	BOOST_ASSERT(GetProxyList().size() > 0);
#	ifndef TEX_PATHFINDER_TEST
		Service::instance()->ReportWorkingError(*this, *GetProxyList().rbegin());
#	else
	if (!m_isTest) {
		Service::instance()->ReportWorkingError(*this, *GetProxyList().rbegin());
	} else {
		LogPathfinderHostTestResult(*GetProxyList().rbegin(), L"FAIL to work");
	}
#	endif

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
