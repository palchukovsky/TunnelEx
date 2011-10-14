/**************************************************************************
 *   Created: 2008/05/22 21:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__EndpointInetAdress_hpp__0805222122
#define INCLUDED_FILE__TUNNELEX__EndpointInetAdress_hpp__0805222122

#include "Api.h"

#include "Core/EndpointAddress.hpp"
#include "Core/SslCertificatesStorage.hpp"
#include "Core/Server.hpp"

class ACE_INET_Addr;
class ACE_SSL_Context;

namespace TunnelEx { namespace Mods { namespace Inet {

	//! Network port.
	typedef unsigned short NetworkPort;

	struct Proxy {

		std::wstring host;
		NetworkPort port;
		std::wstring user;
		std::wstring password;

		bool operator ==(const Proxy &rhs) const {
			return
				rhs.port == port
				&& rhs.host == host
				&& rhs.user == user
				&& rhs.password == password;
		}

		bool operator !=(const Proxy &rhs) const {
			return !operator ==(rhs);
		}

	};

	typedef std::list<Proxy> ProxyList;

	//////////////////////////////////////////////////////////////////////////

	//! Typed endpoint address for network endpoint.
	/** @sa	Endpoint
	  * @sa	EndpointAddress
	  */
	class TUNNELEX_MOD_INET_API InetEndpointAddress : public TunnelEx::EndpointAddress {

	public:

		InetEndpointAddress();
		explicit InetEndpointAddress(
				const TunnelEx::WString &resourceIdentifier,
				TunnelEx::Server::ConstPtr = 0);
		explicit InetEndpointAddress(
				const wchar_t *host,
				NetworkPort,
				TunnelEx::Server::ConstPtr = 0);
		explicit InetEndpointAddress(const ACE_INET_Addr &);
		InetEndpointAddress(const InetEndpointAddress &);
		virtual ~InetEndpointAddress() throw();

		const InetEndpointAddress & operator =(const InetEndpointAddress &);

		void Swap(InetEndpointAddress &) throw();

	public:

		//! Returns a host address in string, without port.
		const std::wstring & GetHostName() const;

		/** If address is "any" returns "*" (not resolved) or host name for
			the local computer (resolved). If "resolved" is true - returns
			name resolved via DNS. So you can set "google.com" and it will
			be resolved as "py-in-f99.google.com". If you set endpoint with
			IP address Endpoint::GetHostName(false) will return it,
			not host name. */
		std::wstring GetResovedHostName() const {
			std::wstring result;
			GetResovedHostName(result);
			return result;
		}
		void GetResovedHostName(std::wstring &) const;

		//! Returns the "dotted decimal" Internet address.
		/** If address is "any" returns "0.0.0.0". */
		const char * GetHostAddress() const;

		void SetHost(const wchar_t *);

		NetworkPort GetPort() const;
		void SetPort(NetworkPort);

		const std::wstring & GetAdapter() const;

		const ACE_INET_Addr & GetAceInetAddr() const;

		virtual std::wstring GetHumanReadable(
				boost::function<std::wstring(const std::wstring &adapter)> adapterSearcher)
			const
			= 0;

		Server::ConstPtr GetServer() const;

	protected:

		virtual void ClearResourceIdentifierCache() throw();

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

	//////////////////////////////////////////////////////////////////////////

	//! Typed endpoint address for TCP endpoint.
	/** @sa	Endpoint
	  * @sa	UdpEndpointAddress
	  * @sa TcpConnectionAcceptor
	  * @sa TcpConnection
	  * @sa OutcomingTcpConnection
	  */
	class TUNNELEX_MOD_INET_API TcpEndpointAddress : public InetEndpointAddress {

	public:

		TcpEndpointAddress();
		explicit TcpEndpointAddress(
				const TunnelEx::WString &resourceIdentifier,
				TunnelEx::Server::ConstPtr = 0);
		explicit TcpEndpointAddress(
				const wchar_t *host,
				NetworkPort,
				TunnelEx::Server::ConstPtr = 0);
		explicit TcpEndpointAddress(const ACE_INET_Addr &);
		explicit TcpEndpointAddress(
				const ACE_INET_Addr &,
				std::auto_ptr<TunnelEx::Helpers::Crypto::X509Shared> remoteCertificate);
		virtual ~TcpEndpointAddress() throw();

		TcpEndpointAddress(const TcpEndpointAddress &);
		const TcpEndpointAddress & operator =(const TcpEndpointAddress &);

		virtual TunnelEx::AutoPtr<TunnelEx::EndpointAddress> Clone() const;

	public:

		const ProxyList & GetProxyList() const;
		void SetProxyList(const ProxyList &);

	public:

		virtual bool IsHasMultiClientsType() const;

		virtual TunnelEx::AutoPtr<TunnelEx::Acceptor> OpenForIncomingConnections(
				const TunnelEx::RuleEndpoint &,
				SharedPtr<const EndpointAddress> ruleEndpointAddress)
			const;

		virtual TunnelEx::AutoPtr<TunnelEx::Connection> CreateRemoteConnection(
				const TunnelEx::RuleEndpoint &,
				SharedPtr<const EndpointAddress>) 
			const;

		virtual TunnelEx::AutoPtr<TunnelEx::Connection> CreateLocalConnection(
				const TunnelEx::RuleEndpoint &,
				SharedPtr<const EndpointAddress>) 
			const;

		virtual bool IsReadyToRecreateRemoteConnection() const {
			return false;
		}
		virtual bool IsReadyToRecreateLocalConnection() const {
			return false;
		}

		virtual std::wstring GetHumanReadable(
				boost::function<std::wstring(const std::wstring &adapter)> adapterSearcher)
			const;

		virtual const TunnelEx::WString & GetResourceIdentifier() const;
		
		static TunnelEx::WString CreateResourceIdentifier(
				const std::wstring &host,
				NetworkPort port,
				const TunnelEx::SslCertificateId &,
				const TunnelEx::SslCertificateIdCollection &);

		//! Creates string resource identifier.
		/** @param adapter		adapter name
		  * @param port			network port
		  * @param allowedHost	allowed host, IP address or * (if none)
		  */
		static TunnelEx::WString CreateResourceIdentifier(
				const std::wstring &adapter,
				NetworkPort port,
				const std::wstring &allowedHost,
				const TunnelEx::SslCertificateId &,
				const TunnelEx::SslCertificateIdCollection &);

		static TunnelEx::WString CreateResourceIdentifier(
				const std::wstring &host,
				NetworkPort port,
				const TunnelEx::SslCertificateId &,
				const TunnelEx::SslCertificateIdCollection &,
				const ProxyList &);

		static const wchar_t * GetProto();

		const TunnelEx::SslCertificateId & GetCertificate() const;
		void SetCertificate(const TunnelEx::SslCertificateId &);

		const TunnelEx::SslCertificateIdCollection & GetRemoteCertificates() const;
		void SetRemoteCertificates(const TunnelEx::SslCertificateIdCollection &);

		const ACE_SSL_Context & GetSslServerContext() const;
		const ACE_SSL_Context & GetSslClientContext() const;

		void CopyCertificate(
			const ::TunnelEx::Mods::Inet::TcpEndpointAddress &localEndpoint,
			const ::TunnelEx::Mods::Inet::TcpEndpointAddress &remoteEndpoint);
		void CopyRemoteCertificate(
			const ::TunnelEx::Mods::Inet::TcpEndpointAddress &remoteEndpoint);


		static const ::TunnelEx::WString & GetAnonymousSslCertificateMagicName();

	protected:

		virtual TunnelEx::AutoPtr<TunnelEx::Connection> CreateConnection(
				const TunnelEx::RuleEndpoint &,
				SharedPtr<const EndpointAddress>) 
			const;

		virtual void ClearResourceIdentifierCache() throw();

		virtual const ACE_INET_Addr * GetProxyAceInetAddr() const;

		virtual const ACE_INET_Addr * GetFirstProxyAceInetAddr(bool, bool) const;

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

	//////////////////////////////////////////////////////////////////////////

	//! Typed endpoint address for UDP endpoint.
	/** @sa	Endpoint
	  * @sa	TcpEndpointAddress
	  * @sa UdpConnectionAcceptor
	  * @sa UdpConnection
	  * @sa OutcomingUdpConnection
	  */
	class TUNNELEX_MOD_INET_API UdpEndpointAddress : public InetEndpointAddress {

	public:

		UdpEndpointAddress();
		explicit UdpEndpointAddress(
				const TunnelEx::WString &resourceIdentifier,
				TunnelEx::Server::ConstPtr = 0);
		explicit UdpEndpointAddress(const wchar_t *host, NetworkPort);
		explicit UdpEndpointAddress(const ACE_INET_Addr &);
		virtual ~UdpEndpointAddress() throw();

		UdpEndpointAddress(const UdpEndpointAddress &);
		const UdpEndpointAddress & operator =(const UdpEndpointAddress &);

		virtual TunnelEx::AutoPtr<TunnelEx::EndpointAddress> Clone() const;

	public:

		virtual bool IsHasMultiClientsType() const;

		virtual TunnelEx::AutoPtr<TunnelEx::Acceptor> OpenForIncomingConnections(
				const TunnelEx::RuleEndpoint &,
				SharedPtr<const EndpointAddress>)
			const;

		virtual TunnelEx::AutoPtr<TunnelEx::Connection> CreateRemoteConnection(
				const TunnelEx::RuleEndpoint &,
				SharedPtr<const EndpointAddress>) 
			const;

		virtual bool IsReadyToRecreateRemoteConnection() const {
			return false;
		}
		virtual bool IsReadyToRecreateLocalConnection() const {
			return false;
		}
		virtual TunnelEx::AutoPtr<TunnelEx::Connection> CreateLocalConnection(
				const TunnelEx::RuleEndpoint &,
				SharedPtr<const EndpointAddress>) 
			const;

		virtual const TunnelEx::WString & GetResourceIdentifier() const;

		virtual std::wstring GetHumanReadable(
				boost::function<std::wstring(const std::wstring &adapter)> adapterSearcher)
			const;

		static TunnelEx::WString CreateResourceIdentifier(
				const std::wstring &host,
				NetworkPort port);

		//! Creates string resource identifier.
		/** @param adapter		adapter name
		  * @param port			network port
		  * @param allowedHost	allowed host, IP address or * (if none)
		  */
		static TunnelEx::WString CreateResourceIdentifier(
				const std::wstring &adapter,
				NetworkPort port,
				const std::wstring &allowedHost);

		static const wchar_t * GetProto();

	private:

		virtual TunnelEx::AutoPtr<TunnelEx::Connection> CreateConnection(
				const TunnelEx::RuleEndpoint &,
				SharedPtr<const EndpointAddress>) 
			const;

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

	//////////////////////////////////////////////////////////////////////////

} } }

#endif // INCLUDED_FILE__TUNNELEX__EndpointInetAdress_hpp__0805222122
