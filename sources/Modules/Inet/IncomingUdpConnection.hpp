/**************************************************************************
 *   Created: 2008/10/25 20:51
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__IncomingUdpConnection_hpp__0810252051
#define INCLUDED_FILE__TUNNELEX__IncomingUdpConnection_hpp__0810252051

#include "UdpConnection.hpp"
#include "ConnectionsTraits.hpp"

#include "Core/Error.hpp"
#include "Core/Log.hpp"
#include "Core/String.hpp"
#include "Core/MessageBlock.hpp"

namespace TunnelEx { namespace Mods { namespace Inet {

	template<bool>
	class UdpConnectionAcceptor;

	template<bool isSecured>
	class IncomingUdpConnection
			: public UdpConnection<typename ConnectionsTraits::Udp<isSecured, true>::Stream> {

	public:

		typedef ConnectionsTraits::Udp<isSecured, true> Trait;
		typedef typename Trait::Stream Stream;
		typedef UdpConnection<Stream> Base;
		typedef UdpConnectionAcceptor<isSecured> Acceptor;

	private:

		typedef ACE_RW_Mutex SendMutex;
		typedef ACE_Read_Guard<SendMutex> SendReadLock;
		typedef ACE_Write_Guard<SendMutex> SendWriteLock;

		typedef ACE_Thread_Mutex AcceptorMutex;
		typedef ACE_Guard<AcceptorMutex> AcceptorLock;

	public:

		explicit IncomingUdpConnection(
					const ACE_INET_Addr &address,
					const TunnelEx::RuleEndpoint &ruleEndpoint,
					SharedPtr<const EndpointAddress> ruleEndpointAddress,
					boost::shared_ptr<Stream> socket,
					const std::vector<char> &incomingData,
					Acceptor *const acceptor)
				: UdpConnection(ruleEndpoint, ruleEndpointAddress),
				m_remoteAddress(address),
				m_socket(socket),
				m_incomingData(incomingData),
				m_acceptor(acceptor) {
			ResetIdleTimeout(60);
		}

		virtual ~IncomingUdpConnection() throw() {
			if (m_acceptor != 0) {
				AcceptorLock lock(m_acceptorMutex);
				if (m_acceptor != 0) {
					m_acceptor->NotifyConnectionClose(*this);
				}
			}
		}

	public:

		virtual UniquePtr<EndpointAddress> GetRemoteAddress() const {
			using namespace TunnelEx;
			// Because remote address can be changed dynamically
			return UniquePtr<EndpointAddress>(
				new UdpEndpointAddress(WString(L"*:*")));
		}

	public:

		void SetRemoteAddress(const ACE_INET_Addr &address) {
			SendWriteLock lock(m_sendMutex);
			m_remoteAddress = address;
		}

		void NotifyAcceptorClose(const Acceptor &acceptor) {
			ACE_UNUSED_ARG(acceptor);
			AcceptorLock lock(m_acceptorMutex);
			BOOST_ASSERT(&acceptor == m_acceptor);
			m_acceptor = 0;
		}

	protected:

		virtual void Setup() {
			BOOST_ASSERT(m_incomingData.size() > 0);
			StartReadRemote();
			SendToTunnel(&m_incomingData[0], m_incomingData.size());
			StopReadRemote();
			Base::Setup();
			std::vector<char>().swap(m_incomingData);
		}

		virtual TunnelEx::IoHandleInfo GetIoHandle() {
			return IoHandleInfo(INVALID_HANDLE_VALUE, IoHandleInfo::TYPE_SOCKET);
		}

		virtual ACE_SOCK & GetIoStream() {
			return *m_socket;
		}

		virtual const ACE_SOCK & GetIoStream() const {
			return const_cast<IncomingUdpConnection *>(this)->GetIoStream();
		}

		virtual DataTransferCommand Write(MessageBlock &messageBlock) {
			BOOST_ASSERT(messageBlock.GetUnreadedDataSize() > 0);
			SendReadLock addrLock(m_sendMutex);
			const ssize_t sentBytesNumb = m_socket->send(
				messageBlock.GetData(),
				messageBlock.GetUnreadedDataSize(),
				m_remoteAddress);
			if (	(sentBytesNumb < 0
						|| size_t(sentBytesNumb) != messageBlock.GetUnreadedDataSize())
					&& Log::GetInstance().IsDebugRegistrationOn()) {
				const Error error(errno);
				const UdpEndpointAddress addr(m_remoteAddress);
				Log::GetInstance().AppendDebug(
					"Failed to send UDP data to %3%: %1% (%2%)",
					ConvertString<String>(error.GetString()).GetCStr(),
					error.GetErrorNo(),
					ConvertString<String>(addr.GetResourceIdentifier()).GetCStr());
			}
			return DATA_TRANSFER_CMD_SEND_PACKET;
		}

	private:

		virtual bool OnIdleTimeout() throw() {
			return false;
		}

	private:

		SendMutex m_sendMutex;
		ACE_INET_Addr m_remoteAddress;
		boost::shared_ptr<Stream> m_socket;
		std::vector<char> m_incomingData;
		AcceptorMutex m_acceptorMutex;
		Acceptor *m_acceptor;

	};

} } }

#endif // INCLUDED_FILE__TUNNELEX__IncomingUdpConnection_hpp__0810252051
