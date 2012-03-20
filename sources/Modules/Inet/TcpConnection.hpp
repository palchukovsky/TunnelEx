/**************************************************************************
 *   Created: 2008/09/23 15:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#pragma once

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

	protected:

		Stream & GetDataStream() {
			assert(m_dataStream.get());
			return *m_dataStream;
		}
		const Stream & GetDataStream() const {
			return const_cast<TcpConnection *>(this)->GetDataStream();
		}

		void CloseDataStream() throw() {
			static_assert(
				boost::is_same<Stream, ACE_SOCK_Stream>::value,
				"Stream is not an ACE_SOCK_Stream");
			if (!m_dataStream.get()) {
				return;
			}
			verify(m_dataStream->close() == 0);
			m_dataStream.reset();
		}

	private:

		std::auto_ptr<Stream> m_dataStream;

	};

	template<>
	void TcpConnection<SslSockStream>::CloseDataStream() throw() {
		if (!m_dataStream.get()) {
			return;
		}
		m_dataStream->SwitchToStreamMode();
		verify(m_dataStream->close() == 0);
		m_dataStream.reset();
	}

	template<>
	void TcpConnection<SslSockStream>::ReadRemote(MessageBlock &messageBlock) {
		
		assert(m_dataStream.get());

		Stream::Lock streamLock(GetDataStream().GetMutex());
		
		if (	!GetDataStream().IsDecryptorEncryptorMode()
				|| messageBlock.GetUnreadedDataSize() == 0) {
			streamLock.unlock();
			Base::ReadRemote(messageBlock);
			return;
		}
		
		GetDataStream().Decrypt(messageBlock);

		AutoPtr<MessageBlock> messageBlockEncrypted;
		if (!GetDataStream().GetEncrypted().empty()) {
			auto cleanFunc = [](Stream *stream) {
				stream->ClearEncrypted();
			};
			std::unique_ptr<Stream, decltype(cleanFunc)> cleaner(
				&GetDataStream(),
				cleanFunc);
			messageBlockEncrypted = CreateMessageBlock(
				GetDataStream().GetEncrypted().size(),
				&GetDataStream().GetEncrypted()[0]);
		}

		AutoPtr<MessageBlock> messageBlockDecrypted;
		MessageBlock *messageBlockToSend = &messageBlock;
		if (GetDataStream().GetDecrypted().size() > messageBlock.GetBlockSize()) {
			messageBlockDecrypted = CreateMessageBlock(
				GetDataStream().GetDecrypted().size(),
				&GetDataStream().GetDecrypted()[0]);
			messageBlockToSend = messageBlockDecrypted.Get();
		} else if (!GetDataStream().GetDecrypted().empty()) {
			messageBlock.SetData(
				&GetDataStream().GetDecrypted()[0],
				GetDataStream().GetDecrypted().size());
		} else {
			messageBlock.Read();
		}

		streamLock.unlock();

		if (messageBlockEncrypted) {
			WriteDirectly(*messageBlockEncrypted);
		}
		Base::ReadRemote(*messageBlockToSend);

	}

	template<>
	DataTransferCommand TcpConnection<SslSockStream>::Write(MessageBlock &messageBlock) {

		Stream::Lock streamLock(GetDataStream().GetMutex());
		
		if (	!GetDataStream().IsDecryptorEncryptorMode()
				|| messageBlock.GetUnreadedDataSize() == 0) {
			streamLock.unlock();
			return Base::Write(messageBlock);
		}

		GetDataStream().Encrypt(messageBlock);
		
		AutoPtr<MessageBlock> messageBlockEncrypted;
		MessageBlock *messageBlockToSend = &messageBlock;
		{
			auto cleanFunc = [&streamLock](Stream *stream) {
				stream->ClearEncrypted();
				streamLock.unlock();
			};
			std::unique_ptr<Stream, decltype(cleanFunc)> cleaner(
				&GetDataStream(),
				cleanFunc);
			if (GetDataStream().GetEncrypted().size() <= messageBlock.GetBlockSize()) {
				if (GetDataStream().GetEncrypted().empty()) {
					messageBlock.Read();
				} else {
					messageBlock.SetData(
						&GetDataStream().GetEncrypted()[0],
						GetDataStream().GetEncrypted().size());
				}
			} else {
				messageBlockEncrypted = CreateMessageBlock(
					GetDataStream().GetEncrypted().size(),
					&GetDataStream().GetEncrypted()[0]);
				messageBlockToSend = messageBlockEncrypted.Get();
			}
		}

		return Base::Write(*messageBlockToSend);

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
