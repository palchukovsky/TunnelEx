/**************************************************************************
 *   Created: 2010/05/31 8:59
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__UpnpEndpointAddress_hpp__1005310859
#define INCLUDED_FILE__TUNNELEX__UpnpEndpointAddress_hpp__1005310859

#include "Api.h"
#include "Core/SslCertificatesStorage.hpp"
#include "Core/EndpointAddress.hpp"
#include "Core/String.hpp"

namespace TunnelEx { namespace Mods { namespace Upnp {

	class TUNNELEX_MOD_UPNP_API UpnpEndpointAddress : public TunnelEx::EndpointAddress {

	public:

		UpnpEndpointAddress(const WString &resourceIdentifier);
		UpnpEndpointAddress(const UpnpEndpointAddress &);
		virtual ~UpnpEndpointAddress() throw();

		UpnpEndpointAddress & operator =(const UpnpEndpointAddress &);

		void Swap(UpnpEndpointAddress &) throw();

	public:

		virtual const WString & GetResourceIdentifier() const;
		
		virtual AutoPtr<Connection> CreateRemoteConnection(
				const RuleEndpoint &ruleEndpoint,
				SharedPtr<const EndpointAddress> ruleEndpointAddress) 
			const;

		virtual bool IsReadyToRecreateLocalConnection() const;

		virtual bool IsReadyToRecreateRemoteConnection() const;

	public:

		unsigned short GetExternalPort() const;

		std::wstring GetHumanReadable(const std::wstring &externalIp) const;

		const TunnelEx::SslCertificateId & GetCertificate() const;
		void SetCertificate(const TunnelEx::SslCertificateId &);

		const TunnelEx::SslCertificateIdCollection & GetRemoteCertificates() const;
		void SetRemoteCertificates(const TunnelEx::SslCertificateIdCollection &);

	private:

		virtual const wchar_t * GetSubProto() const = 0;

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

	//////////////////////////////////////////////////////////////////////////

	class TUNNELEX_MOD_UPNP_API UpnpTcpEndpointAddress : public UpnpEndpointAddress {

	public:

		UpnpTcpEndpointAddress(const WString &resourceIdentifier);
		UpnpTcpEndpointAddress(const UpnpTcpEndpointAddress &);
		virtual ~UpnpTcpEndpointAddress() throw();

		UpnpTcpEndpointAddress & operator =(const UpnpTcpEndpointAddress &);

		void Swap(UpnpTcpEndpointAddress &) throw();

	public:

		virtual bool IsHasMultiClientsType() const;

		virtual AutoPtr<Acceptor> OpenForIncomingConnections(
				const RuleEndpoint &,
				SharedPtr<const EndpointAddress>)
			const;

		virtual AutoPtr<Connection> CreateLocalConnection(
				const RuleEndpoint &,
				SharedPtr<const EndpointAddress>) 
			const;

		virtual AutoPtr<EndpointAddress> Clone() const;

	public:

		static WString CreateResourceIdentifier(
					unsigned short externalPort,
					const TunnelEx::SslCertificateId &certificate,
					const TunnelEx::SslCertificateIdCollection &remoteCertificates);

	private:

		virtual const wchar_t * GetSubProto() const;
		static const wchar_t * GetSubProtoImpl();

	};

	//////////////////////////////////////////////////////////////////////////

	class TUNNELEX_MOD_UPNP_API UpnpUdpEndpointAddress : public UpnpEndpointAddress {

	public:

		UpnpUdpEndpointAddress(const WString &resourceIdentifier);
		UpnpUdpEndpointAddress(const UpnpUdpEndpointAddress &);
		virtual ~UpnpUdpEndpointAddress() throw();

		UpnpUdpEndpointAddress & operator =(const UpnpUdpEndpointAddress &);

		void Swap(UpnpUdpEndpointAddress &) throw();

	public:

		virtual bool IsHasMultiClientsType() const;

		virtual AutoPtr<Acceptor> OpenForIncomingConnections(
				const RuleEndpoint &,
				SharedPtr<const EndpointAddress>)
			const;

		virtual AutoPtr<Connection> CreateLocalConnection(
				const RuleEndpoint &,
				SharedPtr<const EndpointAddress>) 
			const;

		virtual AutoPtr<EndpointAddress> Clone() const;

	public:

		static WString CreateResourceIdentifier(
				unsigned short externalPort,
				const TunnelEx::SslCertificateId &certificate,
				const TunnelEx::SslCertificateIdCollection &remoteCertificates);

	private:

		virtual const wchar_t * GetSubProto() const;
		static const wchar_t * GetSubProtoImpl();

	};

} } }

#endif // INCLUDED_FILE__TUNNELEX__UpnpEndpointAddress_hpp__1005310859
