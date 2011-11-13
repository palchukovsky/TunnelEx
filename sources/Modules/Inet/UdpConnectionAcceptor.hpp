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

		typedef ACE_Thread_Mutex DataConnectionMutex;
		typedef ACE_Guard<DataConnectionMutex> DataConnectionLock;

		typedef std::map<ACE_INET_Addr, Connection *> Connections;

	public:

		explicit UdpConnectionAcceptor(
					const InetEndpointAddress &address,
					const TunnelEx::RuleEndpoint &ruleEndpoint,
					SharedPtr<const EndpointAddress> ruleEndpointAddress)
				: Base(ruleEndpoint, ruleEndpointAddress),
				m_socket(new ACE_SOCK_Dgram, &AceSockDgramCloser),
				m_dataConnectionIncomingBuffer(new std::vector<char>) {

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
			foreach (auto connection, m_dataConnections) {
				connection.second->NotifyAcceptorClose(*this);
			}
		}

	public:

		void NotifyConnectionClose(const Connection &connection) {
			DataConnectionLock lock(m_dataConnectionMutex);
			assert(m_dataConnections.find(connection.GetRemoteAceAddress()) != m_dataConnections.end());
			m_dataConnections.erase(connection.GetRemoteAceAddress());
		}

	public:

		virtual IoHandleInfo GetIoHandle() {
			return IoHandleInfo(m_socket->get_handle(), IoHandleInfo::TYPE_SOCKET);
		}

		virtual AutoPtr<TunnelEx::Connection> Accept() {
			
			assert(m_dataConnectionIncomingBuffer.get());
			assert(!m_dataConnectionIncomingBuffer->empty());
			
			std::auto_ptr<std::vector<char>> newIncomingDataBuffer(new std::vector<char>);
			newIncomingDataBuffer->reserve(m_dataConnectionIncomingBuffer->capacity());
			
			std::auto_ptr<std::vector<char>> incomingData(m_dataConnectionIncomingBuffer);
			m_dataConnectionIncomingBuffer = newIncomingDataBuffer;

			AutoPtr<Connection> result(
				new Connection(
					m_senderAddrCache,
					GetRuleEndpoint(),
					GetRuleEndpointAddress(),
					m_socket,
					incomingData,
					this));

			{
				DataConnectionLock lock(m_dataConnectionMutex);
				assert(m_dataConnections.find(m_senderAddrCache) == m_dataConnections.end());
				m_dataConnections.insert(std::make_pair(m_senderAddrCache, result.Get()));
			}
			
			return result;

		}

		virtual bool TryToAttach() {

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
				m_senderAddrCache);
			if (readResult == -1) {
				const Error error(errno);
				if (error.GetErrorNo() == ECONNRESET) {
					return true;
				}
				WFormat exception(L"Failed to read incoming UDP data: %1% (%2%)");
				exception % error.GetString().GetCStr() % error.GetErrorNo();
				throw ConnectionOpeningException(exception.str().c_str());
			} else if (readResult == 0) {
				assert(false); // just want to see when it happens
				return true;
			}
			assert(size_t(readResult) <= m_dataConnectionIncomingBuffer->size());
			m_dataConnectionIncomingBuffer->resize(readResult);

			DataConnectionLock lock(m_dataConnectionMutex);
			const auto connection = m_dataConnections.find(m_senderAddrCache);
			if (connection == m_dataConnections.end()) {
				return false;
			}
			
			connection->second->SendToTunnel(
				&(*m_dataConnectionIncomingBuffer)[0],
				m_dataConnectionIncomingBuffer->size());

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

		boost::shared_ptr<Stream> m_socket;

		DataConnectionMutex m_dataConnectionMutex;
		ACE_INET_Addr m_senderAddrCache;
		std::auto_ptr<std::vector<char>> m_dataConnectionIncomingBuffer;
		Connections m_dataConnections;

	};

} } }

#endif // INCLUDED_FILE__TUNNELEX__UdpConnectionAcceptor_hpp__1009040130
