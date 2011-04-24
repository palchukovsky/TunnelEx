/**************************************************************************
 *   Created: 2008/05/22 21:23
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "InetEndpointAddress.hpp"
#include "TcpConnectionAcceptor.hpp"
#include "UdpConnectionAcceptor.hpp"
#include "OutcomingTcpConnection.hpp"
#include "OutcomingUdpConnection.hpp"
#include "HttpProxyConnection.hpp"
#include "ConnectionsTraits.hpp"
#include "EndpointResourceIdentifierParsers.hpp"
#include "Licensing.hpp"

#include "Core/Exceptions.hpp"
#include "Core/String.hpp"

using namespace TunnelEx;
using namespace TunnelEx::Helpers;
using namespace TunnelEx::Helpers::Crypto;
using namespace TunnelEx::Mods::Inet;

//////////////////////////////////////////////////////////////////////////

namespace {
	Licensing::FsLocalStorageState & GetLicensingState() {
		static Licensing::FsLocalStorageState licenseState;
		return licenseState;
	}
}

//////////////////////////////////////////////////////////////////////////

namespace TunnelEx { namespace Mods { namespace Inet {

	void ParseEndpointProxyUserPass(
				EndpointResourceIdentifierParsers::UrlSplitConstIterator source,
				std::wstring &user,
				std::wstring &password) {
		EndpointResourceIdentifierParsers::UrlSplitConstIterator it
			= boost::make_split_iterator(
				*source,
				boost::first_finder(L":", boost::is_equal()));
		std::wstring userTmp = StringUtil::DecodeUrl(begin(*it), end(*it));
		std::wstring passTmp; 
		if (it != source) {
			passTmp = StringUtil::DecodeUrl(begin(*++it), end(*source));
		}
		user.swap(userTmp);
		password.swap(passTmp);
	}

	void ParseEndpointProxy(
				EndpointResourceIdentifierParsers::UrlSplitConstIterator source,
				ProxyList &proxyList) {
		EndpointResourceIdentifierParsers::UrlSplitConstIterator paramIt
			= boost::make_split_iterator(
				*source,
				boost::first_finder(L"//", boost::is_equal()));
		if (paramIt == source || !equals(*paramIt, L"http:") || (++paramIt).eof()) {
			throw InvalidLinkException(L"Unknown proxy type");
		}
		Proxy proxy;
		EndpointResourceIdentifierParsers::UrlSplitConstIterator accessParamIt
			= boost::make_split_iterator(
				*paramIt,
				boost::first_finder(L"@", boost::is_equal()));
		if (accessParamIt != paramIt) {
			ParseEndpointProxyUserPass(accessParamIt, proxy.user, proxy.password);
			++accessParamIt;
		}
		EndpointResourceIdentifierParsers::ParseEndpointHostPort(
			accessParamIt,
			proxy.host,
			proxy.port,
			false,
			false);
		proxyList.push_back(proxy);
	}

	WString CreateEndpointResourceIdentifier(
				const wchar_t *const proto,
				const std::wstring &host,
				NetworkPort port,
				const SslCertificateId &certificate = SslCertificateId(),
				const SslCertificateIdCollection &remoteCertificates = SslCertificateIdCollection()) {
		assert(std::wstring(proto).size() > 0);
		assert(host.size() > 0);
		WString result;
		{
			WFormat str(L"%1%://%2%:%3%");
			str % proto % host;
			if (port) {
				str % port;
			} else {
				str % '*';
			}
			result = str.str().c_str();
		}
		assert(!certificate.IsEmpty() || remoteCertificates.GetSize() == 0);
		if (!certificate.IsEmpty()) {
			result += L"?certificate=";
			result += certificate.EncodeUrlClone();
			const size_t remoteCertificatesNumb = remoteCertificates.GetSize();
			for (size_t i = 0; i < remoteCertificatesNumb; ++i) {
				const SslCertificateId &certificate = remoteCertificates[i];
				assert(!certificate.IsEmpty());
				if (certificate.IsEmpty()) {
					continue;
				}
				result += L"&remote_certificate=";
				result += certificate.EncodeUrlClone();
			}
		}
		return result;
	}

	WString CreateEndpointResourceIdentifier(
				const wchar_t *const proto,
				const std::wstring &adapter,
				NetworkPort port,
				const std::wstring &allowedHost,
				const SslCertificateId &certificate = SslCertificateId(),
				const SslCertificateIdCollection &remoteCertificates = SslCertificateIdCollection()) {
		assert(adapter.size() > 0);
		WString result = CreateEndpointResourceIdentifier(
			proto,
			allowedHost,
			port,
			certificate,
			remoteCertificates);
		if (result.Find(L'?') == result.GetNPos()) {
			result += '?';
		} else {
			result += '&';
		}
		result += L"adapter=";
		result += StringUtil::EncodeUrl(adapter).c_str();
		return result;
	}

	WString CreateEndpointResourceIdentifier(
				const wchar_t *proto,
				const std::wstring &host,
				NetworkPort port,
				const SslCertificateId &certificate,
				const SslCertificateIdCollection &remoteCertificates,
				const ProxyList &proxyList) {
		
		assert(host != L"*");
		assert(port != 0);

		WString result = CreateEndpointResourceIdentifier(
			proto,
			host,
			port,
			certificate,
			remoteCertificates);

		if (proxyList.size() > 0) {
			const wchar_t *const proxyType = L"http";
			std::vector<std::wstring> proxyListStr;
			proxyListStr.reserve(proxyList.size());
			foreach (const Proxy &proxy, proxyList) {
				if (!proxy.user.empty()) {
					if (proxy.password.empty()) {
						proxyListStr.push_back(
								(WFormat(L"proxy=%1%://%2%@%3%:%4%")
									% proxyType
									% StringUtil::EncodeUrl(proxy.user)
									% proxy.host
									% proxy.port)
								.str());
					} else {
						proxyListStr.push_back(
								(WFormat(L"proxy=%1%://%2%:%3%@%4%:%5%")
									% proxyType
									% StringUtil::EncodeUrl(proxy.user)
									% StringUtil::EncodeUrl(proxy.password)
									% proxy.host
									% proxy.port)
								.str());
					}
				} else {
					proxyListStr.push_back(
						(WFormat(L"proxy=%1%://%2%:%3%")
								% proxyType
								% proxy.host
								% proxy.port)
							.str());
				}
			}
			assert(proxyListStr.size() == proxyList.size());
			if (result.Find(L'?') == result.GetNPos()) {
				result += L'?';
			} else {
				result += L'&';
			}
			result += boost::join(proxyListStr, L"&").c_str();
		} 
		
		return result;

	}

} } } 

//////////////////////////////////////////////////////////////////////////

class InetEndpointAddress::Implementation {

public:

	Implementation()
			: m_port(0),
			m_server(0) {
		//...//
	}
	
	explicit Implementation(
				const WString &resourceIdentifier,
				Server::ConstPtr server = 0)
			: m_server(server) {
		const std::wstring path = resourceIdentifier.GetCStr();
		EndpointResourceIdentifierParsers::UrlSplitConstIterator pathIt
			= boost::make_split_iterator(
				path,
				boost::token_finder(boost::is_any_of(L"?&")));
		EndpointResourceIdentifierParsers::ParseEndpointHostPort(
			pathIt,
			m_host,
			m_port,
			true,
			true);
		++pathIt;
		EndpointResourceIdentifierParsers::ParseUrlParam(
			pathIt,
			L"adapter",
			boost::bind(
				&EndpointResourceIdentifierParsers::ParseUrlParamValue<std::wstring>,
				_1,
				boost::ref(m_adapter)),
			false);
	}
		
	explicit Implementation(const ACE_INET_Addr &aceAddr)
			: m_addr(new ACE_INET_Addr(aceAddr)),
			m_port(0),
			m_server(0) {
		//...//
	}

	explicit Implementation(
				const wchar_t *host,
				NetworkPort port,
				Server::ConstPtr server = 0)
			: m_host(host),
			m_port(port),
			m_server(server) {
		//...//
	}

	Implementation(const Implementation &rhs)
			: m_host(rhs.m_host),
			m_port(rhs.m_port),
			m_adapter(rhs.m_adapter),
			m_server(rhs.m_server) {
		if (rhs.m_addr.get()) {
			m_addr.reset(new ACE_INET_Addr(*rhs.m_addr));
		}
	}

	~Implementation() {
		//...//
	}

private:

	const Implementation & operator =(const Implementation &);

public:

	mutable std::auto_ptr<ACE_INET_Addr> m_addr;
	std::wstring m_host;
	NetworkPort m_port;
	std::wstring m_adapter;

	Server::ConstPtr const m_server;

public:

	const ACE_INET_Addr & GetAceInetAddr() const {
		
		if (m_addr.get()) {
			return *m_addr;
		}

		if (!m_adapter.empty()) {
			if (boost::iequals(m_adapter, L"all")) {
				if (m_host != L"*") {
					m_addr.reset(new ACE_INET_Addr(m_port, m_host.c_str(), AF_INET));
				} else {
					m_addr.reset(new ACE_INET_Addr(m_port, static_cast<ACE_UINT32>(INADDR_ANY)));
				}
			} else if (boost::iequals(m_adapter, L"loopback")) {
				if (	m_host != L"*"
						&& !boost::iequals(m_host, L"127.0.0.1")
						&& !boost::iequals(m_host, L"localhost")) {
					// see this text below too
					throw InvalidLinkException(
						L"Network adapter has another IP address");
				}
				m_addr.reset(new ACE_INET_Addr(m_port, L"127.0.0.1", AF_INET));
			} else {
				std::vector<unsigned char> adaptersInfo(sizeof(IP_ADAPTER_INFO));
				ULONG adaptersInfoBufferLen = ULONG(adaptersInfo.size());
				if (	GetAdaptersInfo(
							reinterpret_cast<PIP_ADAPTER_INFO>(&adaptersInfo[0]),
							&adaptersInfoBufferLen)
						== ERROR_BUFFER_OVERFLOW) {
					adaptersInfo.resize(adaptersInfoBufferLen);
				}
				String adapterId = ConvertString<String>(m_adapter.c_str());
				if (	GetAdaptersInfo(
							reinterpret_cast<PIP_ADAPTER_INFO>(&adaptersInfo[0]),
							&adaptersInfoBufferLen)
						== NO_ERROR) {
					PIP_ADAPTER_INFO adapter
						= reinterpret_cast<PIP_ADAPTER_INFO>(&adaptersInfo[0]);
					while (adapter) {
						if (adapterId != adapter->AdapterName) {
							adapter = adapter->Next;
							continue;
						}
						const char *const ipAddr = adapter->IpAddressList.IpAddress.String;
						if (!strlen(ipAddr) || !strcmp(ipAddr, "0.0.0.0")) {
							throw InvalidLinkException(
								L"Network adapter is inactive");
						} else if (	m_host != L"*"
									&& !boost::iequals(m_host, ConvertString<WString>(ipAddr).GetCStr())) {
							// see this text on "localhost" too
							throw InvalidLinkException(
								L"Network adapter has another IP address");
						}
						m_addr.reset(
							new ACE_INET_Addr(m_port, ipAddr, AF_INET));
						break;
					}
					if (!m_addr.get()) {
						throw InvalidLinkException(
							L"Failed to find network adapter and get IP address");
					}
				} else {
					Error error(GetLastError());
					WFormat message(L"Could get network adapters list: \"%1%\" (error code: %2%)");
					message % error.GetString().GetCStr();
					message % error.GetErrorNo();
					throw SystemException(message.str().c_str());
				}
			}
		} else if (m_host.empty()) {
			m_addr.reset(new ACE_INET_Addr);
		} else if (m_host == L"*") {
			m_addr.reset(new ACE_INET_Addr(m_port, static_cast<ACE_UINT32>(INADDR_ANY)));
		} else {
			m_addr.reset(new ACE_INET_Addr(m_port, m_host.c_str(), AF_INET));
		}

		return *m_addr;

	}

};

//////////////////////////////////////////////////////////////////////////

InetEndpointAddress::InetEndpointAddress()
		: m_pimpl(new Implementation) {
	//...//
}

InetEndpointAddress::InetEndpointAddress(
			const WString &resourceIdentifier,
			Server::ConstPtr server /*= 0*/)
		: m_pimpl(new Implementation(resourceIdentifier, server)) {
	//...//
}

InetEndpointAddress::InetEndpointAddress(
			const wchar_t *host,
			NetworkPort port,
			Server::ConstPtr server /*= 0*/)
		: m_pimpl(new Implementation(host, port, server)) {
	//...//
}


InetEndpointAddress::InetEndpointAddress(const ACE_INET_Addr &aceAddr)
		: m_pimpl(new Implementation(aceAddr)) {
	//...//
}

InetEndpointAddress::InetEndpointAddress(const InetEndpointAddress &rhs)
		: EndpointAddress(rhs),
		m_pimpl(new Implementation(*rhs.m_pimpl)) {
	//...//
}

InetEndpointAddress::~InetEndpointAddress() {
	delete m_pimpl;
}

const InetEndpointAddress & InetEndpointAddress::operator =(
			const InetEndpointAddress &rhs) {
	std::auto_ptr<Implementation> newImpl(new Implementation(*rhs.m_pimpl));
	EndpointAddress::operator =(rhs);
	delete m_pimpl;
	m_pimpl = newImpl.release();
	return *this;
}

void InetEndpointAddress::Swap(InetEndpointAddress &rhs) throw() {
	EndpointAddress::Swap(rhs);
	Implementation *const oldImpl = m_pimpl;
	m_pimpl = rhs.m_pimpl;
	rhs.m_pimpl = oldImpl;
}

Server::ConstPtr InetEndpointAddress::GetServer() const {
	return m_pimpl->m_server;
}

const std::wstring & InetEndpointAddress::GetAdapter() const {
	return m_pimpl->m_adapter;
}

const std::wstring & InetEndpointAddress::GetHostName() const {
	if (m_pimpl->m_host.empty() && m_pimpl->m_addr.get()) {
		const_cast<InetEndpointAddress *>(this)->m_pimpl->m_host
			= ConvertString<WString>(GetHostAddress()).GetCStr();
	}
	return m_pimpl->m_host;
}

void InetEndpointAddress::GetResovedHostName(std::wstring &result) const {
	std::vector<wchar_t> buffer(MAXHOSTNAMELEN + 1);
	const int getHostNameResult
		= GetAceInetAddr().get_host_name(&buffer[0], buffer.size());
	result = getHostNameResult == 0
		?	&buffer[0]
		:	ConvertString<WString>(GetHostAddress()).GetCStr();
}

const char * InetEndpointAddress::GetHostAddress() const {
	return GetAceInetAddr().get_host_addr();
}

NetworkPort InetEndpointAddress::GetPort() const {
	return m_pimpl->m_port || !m_pimpl->m_addr.get()
		?	m_pimpl->m_port
		:	GetAceInetAddr().get_port_number();
}

void InetEndpointAddress::SetHost(const wchar_t *host) {
	m_pimpl->m_host = host;
	ClearResourceIdentifierCache();
}

void InetEndpointAddress::SetPort(NetworkPort port) {
	m_pimpl->m_port = port;
	ClearResourceIdentifierCache();
}

void InetEndpointAddress::ClearResourceIdentifierCache() throw() {
	m_pimpl->m_addr.reset();
}

const ACE_INET_Addr & InetEndpointAddress::GetAceInetAddr() const {
	return m_pimpl->GetAceInetAddr();
}

//////////////////////////////////////////////////////////////////////////

class TcpEndpointAddress::Implementation {

public:

	Implementation()
			: m_forceSslStatus(FSS_NONE) {
		//...//
	}

	explicit Implementation(const WString &resourceIdentifier)
			: m_forceSslStatus(FSS_NONE) {
		const std::wstring path = resourceIdentifier.GetCStr();
		EndpointResourceIdentifierParsers::UrlSplitConstIterator pathIt
			= boost::make_split_iterator(
				path,
				boost::token_finder(boost::is_any_of(L"?&")));
		++pathIt;
		EndpointResourceIdentifierParsers::ParseUrlParam(
			pathIt,
			L"proxy",
			boost::bind(&ParseEndpointProxy, _1, boost::ref(m_proxyList)),
			true);
		EndpointResourceIdentifierParsers::ParseEndpointCertificates(
			pathIt,
			m_certificate,
			m_remoteCertificates);
	}

	~Implementation() {
		if (m_sslClientContext.get()) {
			SSL_CTX_flush_sessions(m_sslClientContext->context(), 0);
		}
		if (m_sslServerContext.get()) {
			SSL_CTX_flush_sessions(m_sslServerContext->context(), 0);
		}
	}

	Implementation(const Implementation &rhs)
			: m_proxyList(rhs.m_proxyList),
			m_resourceIdentifier(rhs.m_resourceIdentifier),
			m_certificate(rhs.m_certificate),
			m_remoteCertificates(rhs.m_remoteCertificates),
			m_privateKey(rhs.m_privateKey),
			m_remotePublicCertificate(rhs.m_remotePublicCertificate),
			m_sslServerContext(rhs.m_sslServerContext),
			m_sslClientContext(rhs.m_sslClientContext),
			m_forceSslStatus(rhs.m_forceSslStatus) {
		if (rhs.m_proxyAddr.get()) {
			m_proxyAddr.reset(new ACE_INET_Addr(*rhs.m_proxyAddr));
		}
	}

private:

	const Implementation & operator =(const Implementation &);

public:

	bool IsSslServer() const {
		assert(
			(!m_sslClientContext && m_forceSslStatus != FSS_CLIENT)
			|| (!m_sslServerContext && m_forceSslStatus != FSS_SERVER));
		return m_sslServerContext || m_forceSslStatus == FSS_SERVER;
	}

	bool IsSslClient() const {
		assert(
			(!m_sslServerContext && m_forceSslStatus != FSS_SERVER)
			|| (!m_sslClientContext && m_forceSslStatus != FSS_CLIENT));
		return m_sslClientContext || m_forceSslStatus == FSS_CLIENT;
	}

	std::auto_ptr<ACE_SSL_Context> CreateSslContext(
				bool isServer,
				Server::ConstRef server)
			const {

		std::auto_ptr<ACE_SSL_Context> result(new ACE_SSL_Context);

		{
			int endpointMode;
			int sessionCacheMode;
			if (isServer) {
				endpointMode = ACE_SSL_Context::SSLv23_server;
				sessionCacheMode = SSL_SESS_CACHE_SERVER;
			} else {
				endpointMode = ACE_SSL_Context::SSLv23_client;
				sessionCacheMode = SSL_SESS_CACHE_CLIENT;
			}
			if (result->set_mode(endpointMode) != 0) {
				WFormat message(L"Failed to load endpoint certificate: %1%");
				message % ConvertString<WString>(
						OpenSslError::GetLast(true).GetAsString())
					.GetCStr();
				Log::GetInstance().AppendDebug("Failed to setup SSL/TLS context.");
				throw SystemException(message.str().c_str());
			}
			SSL_CTX_set_session_cache_mode(result->context(), sessionCacheMode);
		}

		static Licensing::SslLicense sslLicense(&GetLicensingState());
		if (!sslLicense.IsFeatureAvailable(true)) {
			Log::GetInstance().AppendWarn(
				"Could not use SSL/TLS."
					" The functionality you have requested requires"
					" a License Upgrade. Please purchase a License that"
					" will enable this feature at http://" TUNNELEX_DOMAIN "/order"
					" or get free trial at http://" TUNNELEX_DOMAIN "/order/trial.");
			throw LocalException(L"Could not use SSL/TLS, License Upgrade required");
		}

		{
			typedef boost::uint32_t SessionId;
			static SessionId id = 0;
			++id;
			BOOST_STATIC_ASSERT(sizeof(id) <= SSL_MAX_SSL_SESSION_ID_LENGTH);
			SSL_CTX_set_session_id_context(
				result->context(),
				reinterpret_cast<const unsigned char *>(&id),
				sizeof(SessionId));
		}

		const bool isAnonymous
			=	!m_privateKey
				&& m_certificate == TcpEndpointAddress::GetAnonymousSslCertificateMagicName();

		// Certificate set
		if (isServer || !isAnonymous) {
			if (!m_privateKey) {
				UniquePtr<X509Private> certificate = !isAnonymous
					?	server
							.GetCertificatesStorage()
							.GetPrivateCertificate(m_certificate)
					:	GenerateAnonymousPrivateCertificate();
				const_cast<Implementation *>(this)
					->m_privateKey
					.reset(certificate.Get());
				certificate.Release();
			}
			if (result->certificate(&m_privateKey->Get()) != 0) {
				WFormat message(L"Failed to load endpoint certificate: %1%");
				message
					% ConvertString<WString>(
							OpenSslError::GetLast(true).GetAsString())
						.GetCStr();
				throw SystemException(message.str().c_str());
			}
			// Private key set
			//! @todo: Check implementation for this code in ACE after version 5.8.0 [2010/12/05 23:09]
			if (	SSL_CTX_use_PrivateKey(result->context(), &m_privateKey->GetPrivateKey().Get()) <= 0
					|| result->verify_private_key() != 0) {
				WFormat message(L"Failed to load endpoint certificate: %1%");
				message
					% ConvertString<WString>(
							OpenSslError::GetLast(true).GetAsString())
						.GetCStr();
				throw SystemException(message.str().c_str());
			}
		}

		bool remoteKey = false;
		// Remote side verification certificates set
		//! @todo: Check implementation for this code in ACE after version 5.8.0 [2010/12/05 23:09]
		if (m_remotePublicCertificate) {
			if (	!X509_STORE_add_cert(
						result->context()->cert_store,
						&m_remotePublicCertificate->Get())
					|| (isServer
						&& !SSL_CTX_add_client_CA(
							result->context(),
							&m_remotePublicCertificate->Get()))) {
				WFormat message(L"Failed to load verification certificate: %1%");
				message
					% ConvertString<WString>(
							OpenSslError::GetLast(true).GetAsString())
						.GetCStr();
				throw SystemException(message.str().c_str());
			}
			remoteKey = true;
		} else if (m_remoteCertificates.GetSize() > 0) {
			bool isAnyLoaded = false;
			for (size_t i = 0; i < m_remoteCertificates.GetSize(); ++i) {
				UniquePtr<X509Shared> certificate;
				try {
					certificate = server
						.GetCertificatesStorage()
						.GetCertificate(m_remoteCertificates[i]);
				} catch (const TunnelEx::NotFoundException &) {
					Log::GetInstance().AppendWarn(
						"Failed to find one of verification certificates.");
					continue;
				}
				isAnyLoaded = true;
				if (	!X509_STORE_add_cert(
							result->context()->cert_store,
							&certificate->Get())
						|| (isServer
							&& !SSL_CTX_add_client_CA(
								result->context(),
								&certificate->Get()))) {
					WFormat message(L"Failed to load verification certificate: %1%");
					message
						% ConvertString<WString>(
							OpenSslError::GetLast(true).GetAsString())
						.GetCStr();
					throw SystemException(message.str().c_str());
				}
			}
			if (!isAnyLoaded) {
				throw LocalException(
					L"Failed to found all verification certificates for this endpoint");
			}
			remoteKey = true;
		}
		if (remoteKey) {
			result->set_verify_peer(true, true);
		}

		return result;

	}

private:

	UniquePtr<X509Private> GenerateAnonymousPrivateCertificate() const {
		const std::auto_ptr<const Rsa> rsa(Rsa::Generate(Key::SIZE_2048));
		UniquePtr<X509Private> cert(
			X509Private::GenerateVersion3(
				rsa->GetPrivateKey(),
				rsa->GetPublicKey(),
				rsa->GetPrivateKey(),
				TUNNELEX_NAME,
				TUNNELEX_VENDOR,
				"http://" TUNNELEX_DOMAIN,
				"",
				"",
				"",
				"Anonymous",
				"",
				"",
				"",
				"",
				"")
			.release());
		return cert;
	}

public:

	ProxyList m_proxyList;
	mutable std::auto_ptr<ACE_INET_Addr> m_proxyAddr;

	WString m_resourceIdentifier;

	SslCertificateId m_certificate;
	SslCertificateIdCollection m_remoteCertificates;
	boost::shared_ptr<X509Private> m_privateKey;
	boost::shared_ptr<X509Shared> m_remotePublicCertificate;
	boost::shared_ptr<ACE_SSL_Context> m_sslServerContext;
	boost::shared_ptr<ACE_SSL_Context> m_sslClientContext;

	enum ForceSslStatus {
		FSS_NONE,
		FSS_CLIENT,
		FSS_SERVER
	};
	ForceSslStatus m_forceSslStatus;

};

//////////////////////////////////////////////////////////////////////////

TcpEndpointAddress::TcpEndpointAddress()
		: m_pimpl(new Implementation) {
	//...//
}

TcpEndpointAddress::TcpEndpointAddress(
			const TunnelEx::WString &resourceIdentifier,
			Server::ConstPtr server /*= 0*/)
		: InetEndpointAddress(resourceIdentifier, server),
		m_pimpl(new Implementation(resourceIdentifier)) {
	//...//
}

TcpEndpointAddress::TcpEndpointAddress(const ACE_INET_Addr &addr)
		: InetEndpointAddress(addr),
		m_pimpl(new Implementation) {
	//...//
}

TcpEndpointAddress::TcpEndpointAddress(
			const ACE_INET_Addr &addr,
			std::auto_ptr<TunnelEx::Helpers::Crypto::X509Shared> remoteCertificate)
		: InetEndpointAddress(addr),
		m_pimpl(new Implementation) {
	m_pimpl->m_remotePublicCertificate = remoteCertificate;
}

TcpEndpointAddress::TcpEndpointAddress(
			const wchar_t *host,
			NetworkPort port,
			Server::ConstPtr server /*= 0*/)
		: InetEndpointAddress(host, port, server),
		m_pimpl(new Implementation) {
	//...//
}

TcpEndpointAddress::~TcpEndpointAddress() throw() {
	delete m_pimpl;
}

TcpEndpointAddress::TcpEndpointAddress(const TcpEndpointAddress &rhs) 
		: InetEndpointAddress(rhs),
		m_pimpl(new Implementation(*rhs.m_pimpl)) {
	//...//
}

const TcpEndpointAddress & TcpEndpointAddress::operator =(
			const TcpEndpointAddress &rhs) {
	std::auto_ptr<Implementation> newImpl(new Implementation(*rhs.m_pimpl));
	InetEndpointAddress::operator =(rhs);
	delete m_pimpl;
	m_pimpl = newImpl.release();
	return *this;
}

UniquePtr<EndpointAddress> TcpEndpointAddress::Clone() const {
	return UniquePtr<EndpointAddress>(new TcpEndpointAddress(*this));
}

bool TcpEndpointAddress::IsHasMultiClientsType() const {
	return true;
}

UniquePtr<Acceptor> TcpEndpointAddress::OpenForIncomingConnections(
			const RuleEndpoint &ruleEndpoint,
			SharedPtr<const EndpointAddress> ruleEndpointAddress)
		const {
	UniquePtr<Acceptor> result;
	if (GetCertificate().IsEmpty()) {
		assert(!m_pimpl->IsSslServer());
		assert(!m_pimpl->IsSslClient());
		assert(!m_pimpl->m_sslClientContext);
		assert(!m_pimpl->m_sslServerContext);
		result.Reset(
			new TcpConnectionAcceptor<false, true>(
				*this,
				ruleEndpoint,
				ruleEndpointAddress));
	} else if (m_pimpl->IsSslClient()) {
		assert(!m_pimpl->IsSslServer());
		result.Reset(
			new TcpConnectionAcceptor<true, false>(
				*this,
				ruleEndpoint,
				ruleEndpointAddress));
	} else {
		result.Reset(
			new TcpConnectionAcceptor<true, true>(
				*this,
				ruleEndpoint,
				ruleEndpointAddress));
	}
	return result;
}

UniquePtr<Connection> TcpEndpointAddress::CreateRemoteConnection(
			const RuleEndpoint &ruleEndpoint,
			SharedPtr<const EndpointAddress> ruleEndpointAddress)
		const {
	return CreateConnection(ruleEndpoint, ruleEndpointAddress);
}

UniquePtr<Connection> TcpEndpointAddress::CreateLocalConnection(
			const RuleEndpoint &ruleEndpoint,
			SharedPtr<const EndpointAddress> ruleEndpointAddress) 
		const {
	return CreateConnection(ruleEndpoint, ruleEndpointAddress);
}

UniquePtr<Connection> TcpEndpointAddress::CreateConnection(
			const RuleEndpoint &ruleEndpoint,
			SharedPtr<const EndpointAddress> ruleEndpointAddress) 
		const {
	using namespace SubProtoDirectionTraits;
	UniquePtr<Connection> result;
	const ACE_INET_Addr *const proxyAddress = GetProxyAceInetAddr();
	if (proxyAddress == 0) {
		if (GetCertificate().IsEmpty()) {
			assert(!m_pimpl->IsSslServer());
			assert(!m_pimpl->IsSslClient());
			assert(!m_pimpl->m_sslClientContext);
			assert(!m_pimpl->m_sslServerContext);
			typedef TcpOut<false, false>::Connection Connection;
			result.Reset(
				new Connection(GetAceInetAddr(), ruleEndpoint, ruleEndpointAddress));
		} else if (m_pimpl->IsSslServer()) {
			assert(!m_pimpl->IsSslClient());
			typedef TcpOut<true, true>::Connection Connection;
			result.Reset(
				new Connection(GetAceInetAddr(), ruleEndpoint, ruleEndpointAddress));
		} else {
			typedef TcpOut<true, false>::Connection Connection;
			result.Reset(
				new Connection(GetAceInetAddr(), ruleEndpoint, ruleEndpointAddress));
		}
	} else if (GetCertificate().IsEmpty()) {
		assert(!m_pimpl->IsSslServer());
		assert(!m_pimpl->IsSslClient());
		assert(!m_pimpl->m_sslClientContext);
		assert(!m_pimpl->m_sslServerContext);
		typedef TcpOut<false, false>::Connection SubConnection;
		typedef HttpProxyConnection<SubConnection> Connection;
		result.Reset(new Connection(*proxyAddress, ruleEndpoint, ruleEndpointAddress));
	} else if (m_pimpl->IsSslServer()) {
		assert(!m_pimpl->IsSslClient());
		typedef TcpOut<true, true>::Connection SubConnection;
		typedef HttpProxyConnection<SubConnection> Connection;
		result.Reset(
			new Connection(*proxyAddress, ruleEndpoint, ruleEndpointAddress));
	} else {
		typedef TcpOut<true, false>::Connection SubConnection;
		typedef HttpProxyConnection<SubConnection> Connection;
		result.Reset(
			new Connection(*proxyAddress, ruleEndpoint, ruleEndpointAddress));
	}
	return result;
}

const ACE_INET_Addr * TcpEndpointAddress::GetProxyAceInetAddr() const {
	if (m_pimpl->m_proxyList.size() == 0) {
		return 0;
	}
	static Licensing::ProxyLicense proxyLicense(&GetLicensingState());
	static Licensing::ProxyCascadeLicense proxyCascadeLicense(&GetLicensingState());
	const ACE_INET_Addr *const result = GetFirstProxyAceInetAddr(
		proxyLicense.IsFeatureAvailable(true),
		proxyCascadeLicense.IsFeatureAvailable(true));
	if (!proxyLicense.IsFeatureValueAvailable(true)) {
		Log::GetInstance().AppendWarn(
			"Could not activate HTTP tunneling."
				" The functionality you have requested requires"
				" a License Upgrade. Please purchase a License that"
				" will enable this feature at http://" TUNNELEX_DOMAIN "/order"
				" or get free trial at http://" TUNNELEX_DOMAIN "/order/trial.");
		throw LocalException(L"Could not activate HTTP tunneling, License Upgrade required");
	} else if (
			m_pimpl->m_proxyList.size() > 1
			&& !proxyCascadeLicense.IsFeatureValueAvailable(true)) {
		Log::GetInstance().AppendWarn(
			"Could not activate cascade tunnel."
				" The functionality you have requested requires"
				" a License Upgrade. Please purchase a License that"
				" will enable this feature at http://" TUNNELEX_DOMAIN "/order"
				" or get free trial at http://" TUNNELEX_DOMAIN "/order/trial.");
		throw LocalException(L"Could not cascade tunnel, License Upgrade required");
	}
	return result;
}

const ACE_INET_Addr * TcpEndpointAddress::GetFirstProxyAceInetAddr(
			bool isHttpTunnelingAvailable,
			bool isCascadeTunnelingAvailable)
		const {
	if (!m_pimpl->m_proxyAddr.get()) {
		if (m_pimpl->m_proxyList.size() == 0) {
			return 0;
		} else if (
				isHttpTunnelingAvailable
				&& (m_pimpl->m_proxyList.size() == 1 || isCascadeTunnelingAvailable)) {
			m_pimpl->m_proxyAddr.reset(
				new ACE_INET_Addr(
					m_pimpl->m_proxyList.begin()->port,
					m_pimpl->m_proxyList.begin()->host.c_str(),
					AF_INET));
		}
	}
	return m_pimpl->m_proxyAddr.get();
}

std::wstring TcpEndpointAddress::GetHumanReadable(
			boost::function<std::wstring(const std::wstring &)> adapterSearcher)
		const {
	std::wstring result;
	std::list<std::wstring> props;
	if (!GetCertificate().IsEmpty()) {
		props.push_back(L"secured");
	}
	if (!GetAdapter().empty()) {
		assert(GetProxyList().size() == 0);
		result += L"> ";
		result += CreateEndpointResourceIdentifier(
				GetProto(),
				adapterSearcher(GetAdapter().c_str()),
				GetPort())
			.GetCStr();
	} else {
		result += CreateEndpointResourceIdentifier(
				GetProto(),
				GetHostName(),
				GetPort())
			.GetCStr();
		if (GetProxyList().size() == 1) {
			WFormat proxy(L"proxy: %1%:%2%");
			proxy % GetProxyList().begin()->host % GetProxyList().begin()->port;
			props.push_back(proxy.str());
		} else if (GetProxyList().size() > 1) {
			props.push_back(L"proxy cascade");
		}
	}
	if (props.size() > 0) {
		result += L" (";
		result += boost::join(props, L", ");
		result += L")";
	}
	return result;
}

const WString & TcpEndpointAddress::GetResourceIdentifier() const {
	if (m_pimpl->m_resourceIdentifier.IsEmpty()) {
		if (!GetAdapter().empty()) {
			m_pimpl->m_resourceIdentifier = CreateEndpointResourceIdentifier(
				GetProto(),
				GetAdapter(),
				GetPort(),
				GetHostName(),
				GetCertificate(),
				GetRemoteCertificates());
		} else if (GetProxyList().size() > 0) {
			m_pimpl->m_resourceIdentifier = CreateEndpointResourceIdentifier(
				GetProto(),
				GetHostName(),
				GetPort(),
				GetCertificate(),
				GetRemoteCertificates(),
				GetProxyList());
		} else {
			m_pimpl->m_resourceIdentifier = CreateEndpointResourceIdentifier(
				GetProto(),
				GetHostName(),
				GetPort(),
				GetCertificate(),
				GetRemoteCertificates());
		}
	}
	return m_pimpl->m_resourceIdentifier;
}

WString TcpEndpointAddress::CreateResourceIdentifier(
			const std::wstring &host,
			NetworkPort port,
			const SslCertificateId &certificate,
			const SslCertificateIdCollection &remoteCertificates) {
	return CreateEndpointResourceIdentifier(
		GetProto(),
		host,
		port,
		certificate,
		remoteCertificates);
}

WString TcpEndpointAddress::CreateResourceIdentifier(
			const std::wstring &host,
			NetworkPort port,
			const SslCertificateId &certificate,
			const SslCertificateIdCollection &remoteCertificates,
			const ProxyList &proxyList) {
	return CreateEndpointResourceIdentifier(
		GetProto(),
		host,
		port,
		certificate,
		remoteCertificates,
		proxyList);
}

WString TcpEndpointAddress::CreateResourceIdentifier(
			const std::wstring &adapter,
			NetworkPort port,
			const std::wstring &allowedHost,
			const SslCertificateId &certificate,
			const SslCertificateIdCollection &remoteCertificates) {
	return CreateEndpointResourceIdentifier(
		GetProto(),
		adapter,
		port,
		allowedHost,
		certificate,
		remoteCertificates);
}

const ProxyList & TcpEndpointAddress::GetProxyList() const {
	return m_pimpl->m_proxyList;
}

void TcpEndpointAddress::SetProxyList(const ProxyList &newList) {
#	if defined(_DEBUG) || defined(TEST)
		BOOST_FOREACH(const Proxy &proxy, newList) {
			assert(!proxy.host.empty());
			assert(proxy.port > 0);
			assert(
				(proxy.password.empty() || proxy.user.empty())
				|| (!proxy.password.empty() && !proxy.user.empty()));
		}
#	endif
	ProxyList newListTmp(newList);
	ClearResourceIdentifierCache();
	newListTmp.swap(m_pimpl->m_proxyList);
	m_pimpl->m_proxyAddr.reset();
}

void TcpEndpointAddress::ClearResourceIdentifierCache() throw() {
	InetEndpointAddress::ClearResourceIdentifierCache();
	try {
		m_pimpl->m_resourceIdentifier.Clear();
		m_pimpl->m_sslServerContext.reset();
		m_pimpl->m_sslClientContext.reset();
	} catch (...) {
		Log::GetInstance().AppendFatalError(
			"Exception at TcpEndpointAddress::ClearResourceIdentifierCache");
		assert(false);
	}
}

const wchar_t * TcpEndpointAddress::GetProto() {
	return L"tcp";
}

const SslCertificateId & TcpEndpointAddress::GetCertificate() const {
	return m_pimpl->m_certificate;
}

void TcpEndpointAddress::SetCertificate(const SslCertificateId &certificate) {
	m_pimpl->m_certificate = certificate;
	ClearResourceIdentifierCache();
}

const SslCertificateIdCollection & TcpEndpointAddress::GetRemoteCertificates() const {
	return m_pimpl->m_remoteCertificates;
}

void TcpEndpointAddress::SetRemoteCertificates(const SslCertificateIdCollection &ids) {
	m_pimpl->m_remoteCertificates = ids;
	ClearResourceIdentifierCache();
}

const ACE_SSL_Context & TcpEndpointAddress::GetSslServerContext() const {
	assert(!m_pimpl->m_sslClientContext.get());
	if (m_pimpl->m_sslServerContext.get()) {
		assert(m_pimpl->m_forceSslStatus != Implementation::FSS_CLIENT);
		return *m_pimpl->m_sslServerContext;
	} else if (!GetServer()) {
		throw LogicalException(L"Internal error: failed to get server context");
	} else if (m_pimpl->m_forceSslStatus == Implementation::FSS_CLIENT) {
		throw LogicalException(L"Internal error: SSL/TLS context for server endpoint does not provided");
	}
	m_pimpl->m_sslServerContext = m_pimpl->CreateSslContext(true, *GetServer());
	return *m_pimpl->m_sslServerContext;
}

const ACE_SSL_Context & TcpEndpointAddress::GetSslClientContext() const {
	assert(!m_pimpl->m_sslServerContext.get());
	if (m_pimpl->m_sslClientContext.get()) {
		assert(m_pimpl->m_forceSslStatus != Implementation::FSS_SERVER);
		return *m_pimpl->m_sslClientContext;
	} else if (!GetServer()) {
		throw LogicalException(L"Internal error: failed to get server context");
	} else if (m_pimpl->m_forceSslStatus == Implementation::FSS_SERVER) {
		throw LogicalException(L"Internal error: SSL/TLS context for client endpoint does not provided");
	}
	m_pimpl->m_sslClientContext = m_pimpl->CreateSslContext(false, *GetServer());
	return *m_pimpl->m_sslClientContext;
}

void TcpEndpointAddress::CopyCertificate(
			const TcpEndpointAddress &localEndpoint,
			const TcpEndpointAddress &remoteEndpoint) {
	SslCertificateId certificate(localEndpoint.m_pimpl->m_certificate);
	SslCertificateIdCollection remoteCertificates;
	boost::shared_ptr<X509Private> privateKey(localEndpoint.m_pimpl->m_privateKey);
	boost::shared_ptr<X509Shared> remotePublicCertificate(
		remoteEndpoint.m_pimpl->m_remotePublicCertificate);
	boost::shared_ptr<ACE_SSL_Context> sslServerContext;
	boost::shared_ptr<ACE_SSL_Context> sslClientContext;
	Implementation::ForceSslStatus forceSslStatus
		= localEndpoint.m_pimpl->m_forceSslStatus;
	if (forceSslStatus == Implementation::FSS_NONE) {
		assert(
			!localEndpoint.m_pimpl->m_sslClientContext
			|| !localEndpoint.m_pimpl->m_sslServerContext);
		if (localEndpoint.m_pimpl->m_sslClientContext) {
			forceSslStatus = Implementation::FSS_CLIENT;
		} else if (localEndpoint.m_pimpl->m_sslServerContext) {
			forceSslStatus = Implementation::FSS_SERVER;
		}
	}
	assert(
		(!localEndpoint.m_pimpl->m_sslServerContext && forceSslStatus != Implementation::FSS_SERVER)
		|| (!localEndpoint.m_pimpl->m_sslClientContext && forceSslStatus != Implementation::FSS_CLIENT));
	m_pimpl->m_certificate.Swap(certificate);
	m_pimpl->m_remoteCertificates.Swap(remoteCertificates);
	m_pimpl->m_privateKey.swap(privateKey);
	m_pimpl->m_remotePublicCertificate.swap(remotePublicCertificate);
	m_pimpl->m_sslServerContext.swap(sslServerContext);
	m_pimpl->m_sslClientContext.swap(sslClientContext);
	m_pimpl->m_forceSslStatus = forceSslStatus;
}

void TcpEndpointAddress::CopyRemoteCertificate(const TcpEndpointAddress &remoteEndpoint) {
	SslCertificateIdCollection remoteCertificates;
	boost::shared_ptr<X509Shared> remotePublicCertificate(
		remoteEndpoint.m_pimpl->m_remotePublicCertificate);
	boost::shared_ptr<ACE_SSL_Context> sslServerContext;
	boost::shared_ptr<ACE_SSL_Context> sslClientContext;
	Implementation::ForceSslStatus forceSslStatus = m_pimpl->m_forceSslStatus;
	if (forceSslStatus == Implementation::FSS_NONE) {
		assert(!m_pimpl->m_sslClientContext || !m_pimpl->m_sslServerContext);
		if (m_pimpl->m_sslClientContext) {
			forceSslStatus = Implementation::FSS_CLIENT;
		} else if (m_pimpl->m_sslServerContext) {
			forceSslStatus = Implementation::FSS_SERVER;
		}
	}
	assert(
		(!m_pimpl->m_sslServerContext && forceSslStatus != Implementation::FSS_SERVER)
		|| (!m_pimpl->m_sslClientContext && forceSslStatus != Implementation::FSS_CLIENT));
	m_pimpl->m_remoteCertificates.Swap(remoteCertificates);
	m_pimpl->m_remotePublicCertificate.swap(remotePublicCertificate);
	m_pimpl->m_sslServerContext.swap(sslServerContext);
	m_pimpl->m_sslClientContext.swap(sslClientContext);
	m_pimpl->m_forceSslStatus = forceSslStatus;
}

const WString & TcpEndpointAddress::GetAnonymousSslCertificateMagicName() {
	static const WString name = L"anonymous";
	return name;
}

//////////////////////////////////////////////////////////////////////////

class UdpEndpointAddress::Implementation {

public:

	Implementation(const WString &resourceIdentifier)
			: m_resourceIdentifier(resourceIdentifier) {
		//...//
	}

private:

	const Implementation & operator =(const Implementation &);

public:

	WString m_resourceIdentifier;

};

//////////////////////////////////////////////////////////////////////////

UdpEndpointAddress::UdpEndpointAddress()
		: m_pimpl(0) {
	//...//
}

UdpEndpointAddress::UdpEndpointAddress(
			const TunnelEx::WString &resourceIdentifier,
			Server::ConstPtr server /*= 0*/)
		: InetEndpointAddress(resourceIdentifier, server),
		m_pimpl(0) {
	//...//
}

UdpEndpointAddress::UdpEndpointAddress(const wchar_t *host, NetworkPort port)
		: InetEndpointAddress(host, port),
		m_pimpl(0) {
	//...//
}

UdpEndpointAddress::UdpEndpointAddress(const ACE_INET_Addr &addr)
		: InetEndpointAddress(addr),
		m_pimpl(0) {
	//...//
}

UdpEndpointAddress::~UdpEndpointAddress() throw() {
	delete m_pimpl;
}

UdpEndpointAddress::UdpEndpointAddress(const UdpEndpointAddress &rhs)
		: InetEndpointAddress(rhs),
		m_pimpl(!rhs.m_pimpl ? 0 : new Implementation(*rhs.m_pimpl)) {
	//...//
}

const UdpEndpointAddress & UdpEndpointAddress::operator =(
			const UdpEndpointAddress &rhs) {
	std::auto_ptr<Implementation> newImpl;
	if (rhs.m_pimpl) {
		newImpl.reset(new Implementation(*rhs.m_pimpl));
	}
	InetEndpointAddress::operator =(rhs);
	if (rhs.m_pimpl) {
		delete m_pimpl;
	}
	m_pimpl = newImpl.release();
	return *this;
}

UniquePtr<EndpointAddress> UdpEndpointAddress::Clone() const {
	return UniquePtr<EndpointAddress>(new UdpEndpointAddress(*this));
}

bool UdpEndpointAddress::IsHasMultiClientsType() const {
	return true;
}

UniquePtr<Acceptor> UdpEndpointAddress::OpenForIncomingConnections(
			const RuleEndpoint &ruleEndpoint,
			SharedPtr<const EndpointAddress> ruleEndpointAddress)
		const {
	return UniquePtr<Acceptor>(
		new UdpConnectionAcceptor<false>(
			*this,
			ruleEndpoint,
			ruleEndpointAddress));
}

UniquePtr<Connection> UdpEndpointAddress::CreateConnection(
				const RuleEndpoint &ruleEndpoint,
				SharedPtr<const EndpointAddress> ruleEndpointAddress) 
			const {
	UniquePtr<Connection> result(
		new OutcomingUdpConnection<false>(*this, ruleEndpoint, ruleEndpointAddress));
	return result;
}

UniquePtr<Connection> UdpEndpointAddress::CreateRemoteConnection(
			const RuleEndpoint &ruleEndpoint,
			SharedPtr<const EndpointAddress> ruleEndpointAddress)
		const {
	return CreateConnection(ruleEndpoint, ruleEndpointAddress);
}

UniquePtr<Connection> UdpEndpointAddress::CreateLocalConnection(
			const RuleEndpoint &ruleEndpoint,
			SharedPtr<const EndpointAddress> ruleEndpointAddress) 
		const {
	return CreateConnection(ruleEndpoint, ruleEndpointAddress);
}

const WString & UdpEndpointAddress::GetResourceIdentifier() const {
	if (!m_pimpl) {
		if (!GetAdapter().empty()) {
			const_cast<UdpEndpointAddress *>(this)->m_pimpl
				= new Implementation(
					CreateResourceIdentifier(GetAdapter(), GetPort(), GetHostName()));
		} else {
			const_cast<UdpEndpointAddress *>(this)->m_pimpl
				= new Implementation(
					CreateResourceIdentifier(GetHostName(), GetPort()));
		}
	}
	return m_pimpl->m_resourceIdentifier;
}

WString UdpEndpointAddress::CreateResourceIdentifier(
			const std::wstring &host,
			NetworkPort port) {
	return CreateEndpointResourceIdentifier(GetProto(), host, port);
}

WString UdpEndpointAddress::CreateResourceIdentifier(
			const std::wstring &adapter,
			NetworkPort port,
			const std::wstring &allowedHost) {
	return CreateEndpointResourceIdentifier(GetProto(), adapter, port, allowedHost);
}

std::wstring UdpEndpointAddress::GetHumanReadable(
			boost::function<std::wstring(const std::wstring &adapter)> adapterSearcher)
		const {
	std::wstring result;
	std::list<std::wstring> props;
	if (GetAdapter().empty()) {
		result += CreateEndpointResourceIdentifier(
				GetProto(),
				GetHostName(),
				GetPort())
			.GetCStr();
	} else {
		result += L"> ";
		result += CreateEndpointResourceIdentifier(
				GetProto(),
				adapterSearcher(GetAdapter().c_str()),
				GetPort())
			.GetCStr();
	}
	if (props.size() > 0) {
		result += L" (";
		result += boost::join(props, L", ");
		result += L")";
	}
	return result;
}

const wchar_t * UdpEndpointAddress::GetProto() {
	return L"udp";
}

//////////////////////////////////////////////////////////////////////////

namespace TunnelEx { namespace Mods { namespace Inet {
	
	UniquePtr<EndpointAddress> CreateTcpEndpointAddress(
				Server::ConstRef server,
				const WString &resourceIdentifier) {
		return UniquePtr<EndpointAddress>(
			new TcpEndpointAddress(resourceIdentifier, &server));
	}

	UniquePtr<EndpointAddress> CreateUdpEndpointAddress(
				Server::ConstRef server,
				const WString &resourceIdentifier) {
		return UniquePtr<EndpointAddress>(
			new UdpEndpointAddress(resourceIdentifier, &server));
	}

} } }

//////////////////////////////////////////////////////////////////////////
