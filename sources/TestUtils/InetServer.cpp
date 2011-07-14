/**************************************************************************
 *   Created: 2008/01/12 21:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "InetServer.hpp"
#include "InetConnection.hpp"

namespace io = boost::asio;
using namespace TestUtil;

//////////////////////////////////////////////////////////////////////////

class TcpServer::Implementation : private boost::noncopyable {

private:

	typedef InetConnection<TcpInetConnectionTrait> Connection;
	typedef std::vector<boost::shared_ptr<Connection>> Connections;

public:

	Implementation(const unsigned short port)
			: m_acceptor(m_ioService, io::ip::tcp::endpoint(io::ip::tcp::v4(), port)) {
		StartAccept();
		m_thread.reset(
			new boost::thread(
				boost::bind(&Implementation::ServiceThreadMain, this)));
	}

	~Implementation() {
		try {
			m_acceptor.close();
			foreach (auto &c, m_connections) {
				c->Close();
			}
			m_ioService.stop();
			m_thread->join();
		} catch (...) {
			assert(false);
		}
	}

public:

	bool IsConnected(bool onlyIfActive) const {
		boost::mutex::scoped_lock lock(m_connectionsMutex);
		foreach (const Connections::value_type &c, m_connections) {
			if (!onlyIfActive || c->IsActive()) {
				return true;
			}
		}
		return false;
	}

	bool IsConnected(size_t connectionIndex, bool onlyIfActive) const {
		boost::mutex::scoped_lock lock(m_connectionsMutex);
		return
			connectionIndex < m_connections.size()
			&& (!onlyIfActive || m_connections[connectionIndex]->IsActive());
	}

	unsigned int GetNumberOfAcceptedConnections(bool onlyIfActive) const {
		unsigned int result = 0;
		boost::mutex::scoped_lock lock(m_connectionsMutex);
		foreach (const Connections::value_type &c, m_connections) {
			if (!onlyIfActive || c->IsActive()) {
				++result;
			}
		}
		return result;
	}

	boost::shared_ptr<Connection> GetConnection(size_t connectionIndex) {
		boost::mutex::scoped_lock lock(m_connectionsMutex);
		if (connectionIndex >= m_connections.size()) {
			throw std::logic_error("Could not find connection by index");
		}
		return m_connections[connectionIndex];
	}

	boost::shared_ptr<const Connection> GetConnection(size_t connectionIndex) const {
		return const_cast<Implementation *>(this)->GetConnection(connectionIndex);
	}

private:

	void StartAccept() {
		boost::shared_ptr<Connection> newConnection(
			Connection::Create(m_acceptor.io_service()));
		std::auto_ptr<io::ip::tcp::endpoint> endpoint(new io::ip::tcp::endpoint());
		m_acceptor.async_accept(
			newConnection->GetSocket(),
			*endpoint,
			boost::bind(
				&Implementation::HandleAccept,
				this,
				boost::shared_ptr<Connection>(newConnection),
				io::placeholders::error,
				boost::ref(*endpoint)));
		endpoint.release();
	}

	void HandleAccept(
				boost::shared_ptr<Connection> newConnection,
				const boost::system::error_code &error,
				io::ip::tcp::endpoint &endpoint) {
		const std::auto_ptr<const io::ip::tcp::endpoint> endpointHolder(&endpoint);
		if (!error) {
			{
				boost::mutex::scoped_lock lock(m_connectionsMutex);
				newConnection->Start(endpoint);
				m_connections.push_back(newConnection);
			}
			StartAccept();
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

	io::io_service m_ioService;
	io::ip::tcp::acceptor m_acceptor;
	Connections m_connections;
	boost::shared_ptr<boost::thread> m_thread;
	mutable boost::mutex m_connectionsMutex;

};

//////////////////////////////////////////////////////////////////////////

TcpServer::TcpServer(unsigned short port)
		: m_pimpl(new Implementation(port)) {
	//...//
}

TcpServer::~TcpServer() {
	//...//
}

bool TcpServer::IsConnected(bool onlyIfActive) const {
	return m_pimpl->IsConnected(onlyIfActive);
}

bool TcpServer::IsConnected(size_t connection, bool onlyIfActive) const {
	return m_pimpl->IsConnected(connection, onlyIfActive);
}

unsigned int TcpServer::GetNumberOfAcceptedConnections(bool onlyIfActive) const {
	return m_pimpl->GetNumberOfAcceptedConnections(onlyIfActive);
}

void TcpServer::CloseConnection(size_t connectionIndex) {
	m_pimpl->GetConnection(connectionIndex)->Close();
}

void TcpServer::Send(size_t connectionIndex, const std::string &message)  {
	assert(!message.empty());
	std::auto_ptr<Buffer> buffer(new Buffer(message.begin(), message.end()));
	Send(connectionIndex, buffer);
}

void TcpServer::Send(size_t connectionIndex, std::auto_ptr<Buffer> data) {
	assert(!data->empty());
	m_pimpl->GetConnection(connectionIndex)->Send(data);
}

Buffer::size_type TcpServer::GetReceivedSize(size_t connectionIndex) const {
	return m_pimpl->GetConnection(connectionIndex)->GetReceivedSize();
}

void TcpServer::GetReceived(
			size_t connectionIndex,
			size_t maxSize,
			Buffer &result)
		const {
	m_pimpl->GetConnection(connectionIndex)->GetReceived(maxSize, result);
}

void TcpServer::ClearReceived(size_t connectionIndex, size_t bytesCount) {
	return m_pimpl->GetConnection(connectionIndex)->ClearReceived(bytesCount);
}

bool TcpServer::WaitDataReceiveEvent(
			size_t connectionIndex,
			const boost::system_time &waitUntil,
			Buffer::size_type minSize)
		const {
	return m_pimpl
		->GetConnection(connectionIndex)
		->WaitDataReceiveEvent(waitUntil, minSize);
}

//////////////////////////////////////////////////////////////////////////

class UdpServer::Implementation : private boost::noncopyable {

private:

	typedef UdpInetConnectionTrait ConnectionTrait;
	typedef InetConnection<ConnectionTrait> Connection;
	typedef ConnectionTrait::Proto Proto;
	typedef Proto::endpoint Endpoint;
	typedef std::vector<boost::shared_ptr<Connection>> Connections;
	typedef Implementation Self;

public:

	Implementation(const unsigned short port)
			: m_acceptConnection(Connection::Create(m_ioService, port)) {
		StartRead();
		m_thread.reset(
			new boost::thread(
				boost::bind(&Implementation::ServiceThreadMain, this)));
	}

	~Implementation() {
		try {
			m_acceptConnection->Close();
			foreach (auto &c, m_connections) {
				c->Close();
			}
			m_ioService.stop();
			m_thread->join();
		} catch (...) {
			assert(false);
		}
	}

public:

	bool IsConnected(bool onlyIfActive) const {
		return GetNumberOfAcceptedConnections(onlyIfActive) > 0;
	}

	bool IsConnected(size_t connectionIndex, bool onlyIfActive) const {
		return GetNumberOfAcceptedConnections(onlyIfActive) > connectionIndex;
	}

	unsigned int GetNumberOfAcceptedConnections(bool /*onlyIfActive*/) const {
		boost::mutex::scoped_lock lock(m_connectionsMutex);
		return m_connections.size();
	}

	boost::shared_ptr<Connection> GetConnection(size_t connectionIndex) {
		boost::mutex::scoped_lock lock(m_connectionsMutex);
		if (connectionIndex >= m_connections.size()) {
			throw std::logic_error("Could not find connection by index");
		}
		return m_connections[connectionIndex];
	}

	boost::shared_ptr<const Connection> GetConnection(size_t connectionIndex) const {
		return const_cast<Implementation *>(this)->GetConnection(connectionIndex);
	}

private:

	void StartRead() {
		std::auto_ptr<Buffer> buffer(
			new Buffer(
				Connection::Trait::GetReceiveBufferSize()));
		m_acceptConnection->GetSocket().async_receive_from(
			io::buffer(*buffer, buffer->size()),
			m_endpoint,
			boost::bind(
				&Self::HandleRead,
				this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred,
				boost::ref(*buffer)));
		buffer.release();
	}

	void ServiceThreadMain() {
		try {
			m_ioService.run();
		} catch (const std::exception &) {
			assert(false);
		}
	}

	void HandleWrite(
				const boost::system::error_code &error,
				size_t size,
				Buffer &buffer,
				const Endpoint &endpoint)
			const {
		std::auto_ptr<const Buffer> bufferHolder(&buffer);
		boost::mutex::scoped_lock lock(m_connectionsMutex);
		foreach (auto &c, m_connections) {
			if (endpoint == c->GetEndpoint()) {
				c->HandleWrite(error, size, buffer);
				bufferHolder.release();
				return;
			}
		}
		// already closed
		assert(false);
	}

	void HandleRead(
				const boost::system::error_code &error,
				size_t size,
				Buffer &buffer) {
		
		std::auto_ptr<const Buffer> bufferHolder(&buffer);
		boost::mutex::scoped_lock lock(m_connectionsMutex);
		
		bool isConnectionExists = false;
		foreach (auto &c, m_connections) {
			assert(!isConnectionExists);
			isConnectionExists = m_endpoint == c->GetEndpoint();
			if (isConnectionExists) {
				try {
					c->HandleRead(error, size, buffer, m_endpoint, false);
				} catch (...) {
					bufferHolder.release();
					throw;
				}
				bufferHolder.release();
				break;
			}
		}
		
		if (!isConnectionExists) {
			boost::shared_ptr<Connection> connection
				= Connection::Create(*m_acceptConnection, m_endpoint);
			connection->HandleRead(error, size, buffer, m_endpoint, false);
			bufferHolder.release();
			m_connections.push_back(connection);
		}
		
		StartRead();

	}

private:

	io::io_service m_ioService;
	boost::shared_ptr<Connection> m_acceptConnection;
	Connections m_connections;
	boost::shared_ptr<boost::thread> m_thread;
	mutable boost::mutex m_connectionsMutex;
	Endpoint m_endpoint;

};

////////////////////////////////////////////////////////////////////////////////

UdpServer::UdpServer(unsigned short port)
		: m_pimpl(new Implementation(port)) {
	//...//
}

UdpServer::~UdpServer() {
	//...//
}

bool UdpServer::IsConnected(bool onlyIfActive) const {
	return m_pimpl->IsConnected(onlyIfActive);
}

bool UdpServer::IsConnected(size_t connection, bool onlyIfActive) const {
	return m_pimpl->IsConnected(connection, onlyIfActive);
}

unsigned int UdpServer::GetNumberOfAcceptedConnections(bool onlyIfActive) const {
	return m_pimpl->GetNumberOfAcceptedConnections(onlyIfActive);
}

void UdpServer::CloseConnection(size_t connectionIndex) {
	m_pimpl->GetConnection(connectionIndex);
}

void UdpServer::Send(size_t connectionIndex, const std::string &message)  {
	assert(!message.empty());
	std::auto_ptr<Buffer> buffer(new Buffer(message.begin(), message.end()));
	Send(connectionIndex, buffer);
}

void UdpServer::Send(size_t connectionIndex, std::auto_ptr<Buffer> data) {
	assert(!data->empty());
	m_pimpl->GetConnection(connectionIndex)->Send(data);
}

Buffer::size_type UdpServer::GetReceivedSize(size_t connectionIndex) const {
	return m_pimpl->GetConnection(connectionIndex)->GetReceivedSize();
}

void UdpServer::GetReceived(
			size_t connectionIndex,
			size_t maxSize,
			Buffer &result)
		const {
	m_pimpl->GetConnection(connectionIndex)->GetReceived(maxSize, result);
}

void UdpServer::ClearReceived(size_t connectionIndex, size_t bytesCount) {
	return m_pimpl->GetConnection(connectionIndex)->ClearReceived(bytesCount);
}

bool UdpServer::WaitDataReceiveEvent(
			size_t connectionIndex,
			const boost::system_time &waitUntil,
			Buffer::size_type minSize)
		const {
	return m_pimpl->GetConnection(connectionIndex)->WaitDataReceiveEvent(waitUntil, minSize);
}

////////////////////////////////////////////////////////////////////////////////
