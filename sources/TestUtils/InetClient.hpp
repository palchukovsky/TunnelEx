/**************************************************************************
 *   Created: 2011/06/05 2:11
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__InetClient_hpp__1106050211
#define INCLUDED_FILE__TUNNELEX__InetClient_hpp__1106050211

#include "ClientServer.hpp"
#include "InetConnection.hpp"

namespace TestUtil {

	//////////////////////////////////////////////////////////////////////////

	template<typename ConnectionTraitT>
	class InetClient : public TestUtil::Client {

	public:

		typedef InetClient<ConnectionTraitT> Self;

	private:

		typedef ConnectionTraitT ConnectionTrait;
		typedef typename ConnectionTrait::Proto Proto;
		typedef typename Proto::resolver Resolver;
		typedef typename Proto::endpoint Endpoint;
		typedef typename Proto::resolver_iterator ResolverIterator;
		typedef TestUtil::InetConnection<ConnectionTrait> Connection;

	public:
	
		explicit InetClient(
					const std::string &host,
					unsigned short port,
					const boost::posix_time::time_duration &waitTime)
				: Client(waitTime),
				m_connection(Connection::Create(m_ioService)),
				m_resolver(m_ioService) {

			namespace io = boost::asio;

			Proto::resolver::query query(
				Proto::v4(),
				host,
				boost::lexical_cast<std::string>(port));
			m_resolver.async_resolve(
				query,
				boost::bind(
					&Self::HandleResolve,
					this,
					io::placeholders::error,
					io::placeholders::iterator));

			m_thread.reset(
				new boost::thread(
					boost::bind(&Self::ServiceThreadMain, this)));
		
		}
		
		virtual ~InetClient() {
			try {
				if (m_connection) {
					m_connection->Close();
					m_connection.reset();
				}
				m_ioService.stop();
				m_thread->join();
			} catch (...) {
				assert(false);
			}
		}

	public:

		virtual void Send(const std::string &message) {
			std::auto_ptr<Buffer> buffer(
				new Buffer(message.begin(), message.end()));
			Send(buffer);
		}

		virtual void Send(std::auto_ptr<Buffer> buffer) {
			GetConnection().Send(buffer);
		}

		virtual Buffer::size_type GetReceivedSize() const {
			return GetConnection().GetReceivedSize();
		}
		
		virtual void GetReceived(
					Buffer::size_type maxSize,
					Buffer &result)
				const {
			GetConnection().GetReceived(maxSize, result);
		}

		virtual bool IsConnected() const {
			return m_connection.get() && m_connection->IsActive();
		}

		virtual void ClearReceived(size_t bytesCount = 0) {
			GetConnection().ClearReceived(bytesCount);
		}

		virtual void Disconnect() {
			boost::mutex::scoped_lock lock(m_mutex);
			if (!m_connection.get()) {
				throw ConnectionClosed();
			}
			m_connection->Close();
			m_connection.reset();
		}

	protected:

		virtual bool WaitDataReceiveEvent(
					const boost::system_time &waitUntil,
					Buffer::size_type minSize)
				const {
			return GetConnection().WaitDataReceiveEvent(waitUntil, minSize);
		}

	private:

		Connection & GetConnection() {
			if (!m_connection.get()) {
				throw ConnectionClosed();
			}
			return *m_connection;
		}
		const Connection & GetConnection() const {
			return const_cast<Self *>(this)->GetConnection();
		}

		void HandleResolve(
					const boost::system::error_code &error,
					ResolverIterator endpointIterator) {
			namespace io = boost::asio;
			assert(!m_connection->IsActive());
			if (!error) {
				std::auto_ptr<Endpoint> endpoint(new Endpoint(*endpointIterator));
				m_connection->GetSocket().async_connect(
					*endpoint,
					boost::bind(
						&Self::HandleConnect,
						this,
						io::placeholders::error,
						boost::ref(*endpoint),
						++endpointIterator));
				endpoint.release();
			} else {
				// error
			}
		}

		void HandleConnect(
					const boost::system::error_code &error,
					Endpoint &endpoint,
					ResolverIterator endpointIterator) {
			namespace io = boost::asio;
			std::auto_ptr<Endpoint> endpointHolder(&endpoint);
			boost::mutex::scoped_lock lock(m_mutex);
			assert(!m_connection->IsActive());
			if (!error) {
				m_connection->Start(endpoint);
			} else if (endpointIterator != ResolverIterator()) {
				m_connection->GetSocket().close();
				std::auto_ptr<Endpoint> endpoint(new Endpoint(*endpointIterator));
				m_connection->GetSocket().async_connect(
					*endpoint,
					boost::bind(
						&Self::HandleConnect,
						this,
						io::placeholders::error,
						boost::ref(*endpoint),
						++endpointIterator));
				endpoint.release();
			} else {
				// error
			}
		}

		void ServiceThreadMain() {
			try {
				m_ioService.run();
			} catch (const std::exception &) {
				assert(false);
			}
		}

	private:
		
		boost::asio::io_service m_ioService;
		Resolver m_resolver;
		mutable boost::mutex m_mutex;
		boost::shared_ptr<Connection> m_connection;
		boost::shared_ptr<boost::thread> m_thread;

	};

	//////////////////////////////////////////////////////////////////////////
	
	class TcpClient : public InetClient<TestUtil::TcpInetConnectionTrait> {

	public:

		typedef InetClient<TestUtil::TcpInetConnectionTrait> Base;

	public:

		explicit TcpClient(
					const std::string &host,
					unsigned short port,
					const boost::posix_time::time_duration &waitTime)
				: Base(host, port, waitTime) {
			//...//
		}

	};

	//////////////////////////////////////////////////////////////////////////

	class UdpClient : public InetClient<TestUtil::UdpInetConnectionTrait> {

	public:

		typedef InetClient<TestUtil::UdpInetConnectionTrait> Base;

	public:

		explicit UdpClient(
					const std::string &host,
					unsigned short port,
					const boost::posix_time::time_duration &waitTime)
				: Base(host, port, waitTime) {
			//...//
		}

	};

	//////////////////////////////////////////////////////////////////////////

}

#endif // INCLUDED_FILE__TUNNELEX__InetClient_hpp__1106050211
