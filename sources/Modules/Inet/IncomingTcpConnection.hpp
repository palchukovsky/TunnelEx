/**************************************************************************
 *   Created: 2007/06/12 20:24
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__NetworkIncomingConnection_h__0706122024
#define INCLUDED_FILE__NetworkIncomingConnection_h__0706122024

#include "TcpConnection.hpp"
#include "ConnectionsTraits.hpp"
#include "Core/Endpoint.hpp"
#include "Core/Log.hpp"
#include "Core/Exceptions.hpp"
#include "Core/Error.hpp"

namespace TunnelEx { namespace Mods { namespace Inet {

	template<bool isSecured>
	class IncomingTcpConnection
			: public TcpConnection<typename ConnectionsTraits::Tcp<isSecured, true>::Stream> {

	public:

		typedef ConnectionsTraits::Tcp<isSecured, true> Trait;
		typedef SubProtoDirectionTraits::TcpIn<isSecured, true> AcceptionTrait;
		typedef typename Trait::Stream Stream;
		typedef typename AcceptionTrait::Acceptor Acceptor;
		typedef TcpConnection<Stream> Base;

	public:

		explicit IncomingTcpConnection(
					const RuleEndpoint &ruleEndpoint,
					SharedPtr<const EndpointAddress> ruleEndpointAddress,
					const Acceptor &acceptor)
				: Base(ruleEndpoint, ruleEndpointAddress) {
			AcceptConnection(acceptor, ruleEndpoint, *ruleEndpointAddress);
		}

		virtual ~IncomingTcpConnection() throw() {
			//...//
		}

	public:

		virtual bool IsOneWay() const {
			BOOST_STATIC_ASSERT(isSecured == false);
			return false;
		}

		virtual AutoPtr<EndpointAddress> GetRemoteAddress() const {
			return AutoPtr<EndpointAddress>(new TcpEndpointAddress(*m_remoteAddress));
		}

	private:

		std::auto_ptr<Stream> CreateStream(
					const EndpointAddress &/*ruleEndpointAddress*/)
				const {
			BOOST_STATIC_ASSERT(isSecured == false);
			std::auto_ptr<Stream> result(new Stream);
			return result;
		}
	
		void HandleAcceptError(Stream &) const {
			BOOST_STATIC_ASSERT(isSecured == false);
			const Error error(errno);
			WFormat message(L"Failed to accept incoming connection: \"%1% (%2%)\"");
			message % error.GetString().GetCStr() % error.GetErrorNo();
			throw ConnectionOpeningException(message.str().c_str());
		}

		void HandleAcceptSuccess(Stream &) const {
			BOOST_STATIC_ASSERT(isSecured == false);
		}

		void AcceptConnection(
					const Acceptor &acceptor,
					const RuleEndpoint &ruleEndpoint,
					const EndpointAddress &ruleEndpointAddress) {

			std::auto_ptr<Stream> stream(CreateStream(ruleEndpointAddress));
			ACE_Time_Value timeout(ruleEndpoint.GetOpenTimeout());
			ACE_INET_Addr aceRemoteAddr;

			if (acceptor.accept(*stream, &aceRemoteAddr, &timeout) != 0) {
				HandleAcceptError(*stream);
			} else {
				HandleAcceptSuccess(*stream);
			}

			AutoPtr<const TcpEndpointAddress> remoteAddress(
			new TcpEndpointAddress(aceRemoteAddr));

			SetDataStream(stream);
			remoteAddress.Swap(m_remoteAddress);

		}

	private:

		TunnelEx::AutoPtr<const TunnelEx::Mods::Inet::TcpEndpointAddress>
			m_remoteAddress;

	};

	template<>
	bool IncomingTcpConnection<true>::IsOneWay() const {
		return GetIoStream().get_handle() == ACE_INVALID_HANDLE;
	}
	
	template<>
	std::auto_ptr<IncomingTcpConnection<true>::Stream>
	IncomingTcpConnection<true>::CreateStream(
				const EndpointAddress &address)
			const {
		std::auto_ptr<Stream> result(
			new Stream(
				boost::polymorphic_downcast<const TcpEndpointAddress *>(&address)
					->GetSslServerContext()));
		return result;
	}

	template<>
	void IncomingTcpConnection<true>::HandleAcceptSuccess(Stream &stream) const {

		assert(
			SSL_get_peer_certificate(stream.ssl()) != 0
			|| boost::polymorphic_downcast<const TcpEndpointAddress *>(
					GetRuleEndpointAddress().Get())
				->GetRemoteCertificates().GetSize() == 0);
		assert(
			SSL_get_peer_certificate(stream.ssl()) == 0
			|| SSL_get_verify_result(stream.ssl()) == X509_V_OK);

		stream.SwitchToDecryptorEncryptorMode();

	}

	template<>
	void IncomingTcpConnection<true>::HandleAcceptError(Stream &stream) const {

		using namespace Helpers::Crypto;

		const Error sysError(errno);
		const OpenSslError openSslError(OpenSslError::GetLast(true));

		if (sysError.IsError() || openSslError.GetErrorsNumb() > 0) {
			
			stream.close();

			WFormat message(L"Failed to accept incoming connection: \"%1% (%2%)\"");
			const wchar_t *const accessDeniedNoCertMessage
				= L"access denied, remote certificate does not presented";
			const wchar_t *const accessDeniedWronCertMessage
				= L"access denied, remote certificate does not allowed";

			if (sysError.IsError()) {
			
				if (openSslError.GetErrorsNumb() > 0) {
					Log::GetInstance().AppendDebug(openSslError.GetAsString());
				}

				if (!sysError.CheckError()) {
					
					switch (ERR_GET_REASON(sysError.GetErrorNo())) {
						case SSL_R_PEER_DID_NOT_RETURN_A_CERTIFICATE:
							message % accessDeniedNoCertMessage;
							break;
						case SSL_R_NO_CERTIFICATE_RETURNED:
							message % accessDeniedWronCertMessage;
							break;
						default:
							if (openSslError.CheckError(sysError.GetErrorNo())) {
								const std::string errorStr
									= OpenSslError::ErrorNoToString(sysError.GetErrorNo());
								message % ConvertString<WString>(errorStr.c_str()).GetCStr();
						} else {
							message % sysError.GetString().GetCStr();
						}
						break;
					}
				
				} else {
					message % sysError.GetString().GetCStr();
				}
			
			} else {
			
				if (openSslError.IsReason(SSL_R_PEER_DID_NOT_RETURN_A_CERTIFICATE)) { 
					message % accessDeniedNoCertMessage;
				} else if (openSslError.IsReason(SSL_R_NO_CERTIFICATE_RETURNED)) {
					message % accessDeniedWronCertMessage;
				} else {
					message % ConvertString<WString>(openSslError.GetAsString()).GetCStr();
				}

			}
		
			message % sysError.GetErrorNo();

			throw ConnectionOpeningException(message.str().c_str());

		}

		assert(stream.get_handle() == ACE_INVALID_HANDLE);
		ACE_UNUSED_ARG(stream);

	}

} } }

#endif // INCLUDED_FILE__NetworkIncomingConnection_h__0706122024
