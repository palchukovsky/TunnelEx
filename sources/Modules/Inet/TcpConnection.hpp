/**************************************************************************
 *   Created: 2008/09/23 15:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__TcpConnection_hpp__0809231544
#define INCLUDED_FILE__TUNNELEX__TcpConnection_hpp__0809231544

#include "InetConnection.hpp"
#include "InetEndpointAddress.hpp"
#include "SslSockStream.hpp"
#include "Core/MessageBlock.hpp"
#include "Core/Exceptions.hpp"
#include "Core/Error.hpp"

namespace TunnelEx { namespace Mods { namespace Inet {

	template<typename StreamTypename>
	class TcpConnection : public InetConnection<TcpEndpointAddress> {

	public:

		typedef StreamTypename Stream;
		typedef InetConnection<TcpEndpointAddress> Base;
		
	protected:

		explicit TcpConnection(
					const RuleEndpoint &ruleEndpoint,
					SharedPtr<const EndpointAddress> ruleEndpointAddress)
				: Base(ruleEndpoint, ruleEndpointAddress) {
			//...//
		}

	public:

		virtual ~TcpConnection() throw() {
			try {
				CloseDataStream();
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
			}
		}

	protected:

		void SetDataStream(std::auto_ptr<Stream> stream) {
			assert(!m_dataStream.get());
			m_dataStream = stream;
		}

	public:

		virtual AutoPtr<EndpointAddress> GetRemoteAddress() const {
			ACE_INET_Addr aceAddr;
			GetRemoteAceAddress(aceAddr);
			return AutoPtr<EndpointAddress>(new TcpEndpointAddress(aceAddr));
		}

	protected:

		void GetRemoteAceAddress(ACE_INET_Addr &result) const {
			assert(m_dataStream.get());
			if (GetIoStream().get_remote_addr(result) != 0) {
				const Error error(errno);
				WFormat message(L"Failed to get remote inet (TCP) address: \"%1% (%2%)\".");
				message % error.GetStringW() % error.GetErrorNo();
				throw SystemException(message.str().c_str());
			}
		}

	protected:

		virtual void ReadRemote(MessageBlock &messageBlock) {
			BOOST_MPL_ASSERT((boost::is_same<Stream, ACE_SOCK_Stream>));
			assert(m_dataStream.get());
			Base::ReadRemote(messageBlock);
		}

		virtual DataTransferCommand Write(MessageBlock &messageBlock) {
			BOOST_MPL_ASSERT((boost::is_same<Stream, ACE_SOCK_Stream>));
			assert(m_dataStream.get());
			return Base::Write(messageBlock);
		}

	protected:

		virtual ACE_SOCK & GetIoStream() throw() {
			return GetDataStream();
		}
		virtual const ACE_SOCK & GetIoStream() const throw() {
			return const_cast<TcpConnection *>(this)->GetIoStream();
		}

		Stream & GetDataStream() {
			assert(m_dataStream.get());
			return *m_dataStream;
		}
		const Stream & GetDataStream() const {
			return const_cast<TcpConnection *>(this)->GetDataStream();
		}

		void CloseDataStream() {
			BOOST_MPL_ASSERT((boost::is_same<Stream, ACE_SOCK_Stream>));
			if (!m_dataStream.get()) {
				return;
			}
			const int result = m_dataStream->close();
			assert(result == 0);
			ACE_UNUSED_ARG(result);
			m_dataStream.reset();
		}

	private:

		std::auto_ptr<Stream> m_dataStream;

	};

	template<>
	void TcpConnection<SslSockStream>::CloseDataStream() {
		if (!m_dataStream.get()) {
			return;
		}
		m_dataStream->SwitchToStreamMode();
		const int result = m_dataStream->close();
		assert(result == 0);
		ACE_UNUSED_ARG(result);
		m_dataStream.reset();
	}

	template<>
	void TcpConnection<SslSockStream>::ReadRemote(MessageBlock &messageBlock) {
		assert(m_dataStream.get());
		if (	GetDataStream().IsDecryptorEncryptorMode()
				&& messageBlock.GetUnreadedDataSize() > 0) {
			GetDataStream().Decrypt(messageBlock);
			if (GetDataStream().GetEncryptorDecryptorAnswer().size() > 0) {
				try {
					WriteDirectly(
						*CreateMessageBlock(
							GetDataStream().GetEncryptorDecryptorAnswer().size(),
							&GetDataStream().GetEncryptorDecryptorAnswer()[0]));
				} catch (...) {
					GetDataStream().ResetEncryptorDecryptorAnswer();
					throw;
				}
				GetDataStream().ResetEncryptorDecryptorAnswer();
			}
		}
		Base::ReadRemote(messageBlock);
	}

	template<>
	DataTransferCommand TcpConnection<SslSockStream>::Write(MessageBlock &messageBlock) {
		if (	GetDataStream().IsDecryptorEncryptorMode()
				&& messageBlock.GetUnreadedDataSize() > 0) {
			GetDataStream().Encrypt(messageBlock);
			assert(GetDataStream().GetEncryptorDecryptorAnswer().empty());
		}
		return Base::Write(messageBlock);
	}

	template<>
	AutoPtr<EndpointAddress> TcpConnection<SslSockStream>::GetRemoteAddress() const {
		assert(m_dataStream.get());
		using namespace TunnelEx::Helpers::Crypto;
		ACE_INET_Addr aceAddr;
		GetRemoteAceAddress(aceAddr);
		X509 *const x509Ptr = SSL_get_peer_certificate(m_dataStream->ssl());
		if (x509Ptr != 0) {
			std::auto_ptr<X509Shared> x509(new X509Shared(X509_dup(x509Ptr)));
			return AutoPtr<EndpointAddress>(new TcpEndpointAddress(aceAddr, x509));
		} else {
			return AutoPtr<EndpointAddress>(new TcpEndpointAddress(aceAddr));
		}
	}

} } }

#endif // INCLUDED_FILE__TUNNELEX__TcpConnection_hpp__0809231544
