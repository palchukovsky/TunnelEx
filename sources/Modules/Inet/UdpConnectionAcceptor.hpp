/**************************************************************************
 *   Created: 2010/09/04 1:30
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__UdpConnectionAcceptor_hpp__1009040130
#define INCLUDED_FILE__TUNNELEX__UdpConnectionAcceptor_hpp__1009040130

#include "Api.h"
#include "IncomingUdpConnection.hpp"
#include "UdpConnectionAcceptor.hpp"
#include "InetEndpointAddress.hpp"
#include "AceSockDgramCloser.h"
#include "Core/Acceptor.hpp"
#include "Core/Exceptions.hpp"
#include "Core/Log.hpp"
#include "Core/Error.hpp"

namespace TunnelEx { namespace Mods { namespace Inet {

	class InetEndpointAddress;

	template<bool isSecured>
	class TUNNELEX_MOD_INET_API UdpConnectionAcceptor : public TunnelEx::Acceptor {

	public:

		typedef TunnelEx::Acceptor Base;
		typedef IncomingUdpConnection<isSecured> Connection;
		typedef typename Connection::Trait ConnectionTrait;
		typedef typename ConnectionTrait::Stream Stream;

	private:

		typedef ACE_RW_Mutex DataConnectionMutex;
		typedef ACE_Read_Guard<DataConnectionMutex> DataConnectionReadLock;
		typedef ACE_Write_Guard<DataConnectionMutex> DataConnectionWriteLock;

	public:

		explicit UdpConnectionAcceptor(
					const InetEndpointAddress &address,
					const TunnelEx::RuleEndpoint &ruleEndpoint,
					SharedPtr<const EndpointAddress> ruleEndpointAddress)
				: Base(ruleEndpoint, ruleEndpointAddress),
				m_socket(new ACE_SOCK_Dgram, &AceSockDgramCloser),
				m_dataConnectionIncomingBuffer(new std::vector<char>),
				m_dataConnection(0) {

			const ACE_INET_Addr &inetAddr = address.GetAceInetAddr();
			if (inetAddr.is_any() && inetAddr.get_port_number() == 0) {
				// see TEX-308
				throw ConnectionOpeningException(
					L"Failed to open UDP listener: "
						L"The requested address is not valid in its context");
			}

			if (m_socket->open(inetAddr) != 0) {
				const Error error(errno);
				WFormat exception(L"Failed to open UDP listener: %1% (%2%)");
				exception % error.GetString().GetCStr() % error.GetErrorNo();
				throw ConnectionOpeningException(exception.str().c_str());
			}

		}

		virtual ~UdpConnectionAcceptor() throw() {
			if (m_dataConnection != 0) {
				DataConnectionWriteLock lock(m_dataConnectionMutex);
				if (m_dataConnection != 0) {
					m_dataConnection->NotifyAcceptorClose(*this);
				}
			}
		}

	public:

		void NotifyConnectionClose(const Connection &connection) {
			ACE_UNUSED_ARG(connection);
			DataConnectionWriteLock lock(m_dataConnectionMutex);
			assert(&connection == m_dataConnection);
			m_dataConnection = 0;
		}

	public:

		virtual IoHandleInfo GetIoHandle() {
			return IoHandleInfo(m_socket->get_handle(), IoHandleInfo::TYPE_SOCKET);
		}

		virtual AutoPtr<TunnelEx::Connection> Accept() {

			DataConnectionWriteLock lock(m_dataConnectionMutex);
			assert(m_dataConnection == 0);

			ACE_INET_Addr senderAddr;
			if (!ReadFromIncomingStream(senderAddr)) {
				throw ConnectionOpeningException(
					L"An existing connection was forcibly closed by the remote host");
			}

			std::auto_ptr<std::vector<char>> newIncomingDataBuffer(new std::vector<char>);
			newIncomingDataBuffer->reserve(m_dataConnectionIncomingBuffer->capacity());
			
			std::auto_ptr<std::vector<char>> incomingData(m_dataConnectionIncomingBuffer);
			m_dataConnectionIncomingBuffer = newIncomingDataBuffer;

			AutoPtr<Connection> result(
				new Connection(
					senderAddr,
					GetRuleEndpoint(),
					GetRuleEndpointAddress(),
					m_socket,
					incomingData,
					this));
			m_dataConnection = result.Get();
			
			return result;


		}

		virtual bool TryToAttach() {

			if (!m_dataConnection) {
				return false;
			}
			DataConnectionReadLock lock(m_dataConnectionMutex);
			if (!m_dataConnection) {
				return false;
			}

			ACE_INET_Addr senderAddr;
			if (ReadFromIncomingStream(senderAddr)) {
				m_dataConnection->SetRemoteAddress(senderAddr);
				m_dataConnection->SendToTunnel(
					&(*m_dataConnectionIncomingBuffer)[0],
					m_dataConnectionIncomingBuffer->size());
			}

			return true;

		}

		virtual AutoPtr<EndpointAddress> GetLocalAddress() const {
			ACE_INET_Addr addr;
			if (m_socket->get_local_addr(addr) != 0) {
				const Error error(errno);
				WFormat exception(L"Failed to get listening UDP socket address: %1% (%2%)");
				exception % error.GetString().GetCStr() % error.GetErrorNo();
				throw SystemException(exception.str().c_str());
			}
			return AutoPtr<EndpointAddress>(new UdpEndpointAddress(addr));
		}

	private:

		bool ReadFromIncomingStream(ACE_INET_Addr &senderAddr) {
			assert(m_dataConnectionIncomingBuffer.get());
			int dataLen;
			if (	ACE_OS::ioctl(m_socket->get_handle(), FIONREAD, &dataLen) == -1
					|| dataLen <= 0) {
				const Error error(errno);
				WFormat exception(L"Failed to get incoming UDP data size: %1% (%2%)");
				exception % error.GetString().GetCStr() % error.GetErrorNo();
				throw ConnectionOpeningException(exception.str().c_str());
			}
			m_dataConnectionIncomingBuffer->resize(dataLen);
			const ssize_t readResult = m_socket->recv(
				&(*m_dataConnectionIncomingBuffer)[0],
				m_dataConnectionIncomingBuffer->size(),
				senderAddr);
			if (readResult == -1) {
				const Error error(errno);
				if (error.GetErrorNo() == ECONNRESET) {
					return false;
				}
				WFormat exception(L"Failed to read incoming UDP data: %1% (%2%)");
				exception % error.GetString().GetCStr() % error.GetErrorNo();
				throw ConnectionOpeningException(exception.str().c_str());
			}
			return true;
		}

	private:

		boost::shared_ptr<Stream> m_socket;

		DataConnectionMutex m_dataConnectionMutex;
		std::auto_ptr<std::vector<char>> m_dataConnectionIncomingBuffer;
		Connection *m_dataConnection;

	};

} } }

#endif // INCLUDED_FILE__TUNNELEX__UdpConnectionAcceptor_hpp__1009040130
