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

	typedef TcpConnection Connection;
	typedef std::vector<boost::shared_ptr<Connection> > Connections;

public:

	Implementation(const unsigned short port)
			: m_acceptor(m_ioService, io::ip::tcp::endpoint(io::ip::tcp::v4(), port)) {
		StartAccept();
		m_thread.reset(
			new boost::thread(
				boost::bind(&Implementation::ServiceThreadMain, this)));
	}

	~Implementation() {
		m_ioService.stop();
		m_thread->join();
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

	void CloseConnection(size_t connectionIndex) {
		boost::mutex::scoped_lock lock(m_connectionsMutex);
		if (connectionIndex >= m_connections.size()) {
			throw std::logic_error("Could not find connection by index");
		}
		m_connections.erase(m_connections.begin() + connectionIndex);
	}

	void Send(size_t connectionIndex, const std::string &message) {
		assert(message.size());
		boost::mutex::scoped_lock lock(m_connectionsMutex);
		GetConnection(connectionIndex, lock).Send(message);
	}

	void Send(size_t connectionIndex, const Buffer &data) {
		assert(data.size());
		boost::mutex::scoped_lock lock(m_connectionsMutex);
		GetConnection(connectionIndex, lock).Send(data);
	}

	Buffer::size_type GetReceivedSize(size_t connectionIndex) const {
		boost::mutex::scoped_lock lock(m_connectionsMutex);
		return GetConnection(connectionIndex, lock).GetReceivedSize();
	}

	Buffer GetReceived(size_t connectionIndex) const {
		boost::mutex::scoped_lock lock(m_connectionsMutex);
		return GetConnection(connectionIndex, lock).GetReceived();
	}

	void ClearReceived(size_t connectionIndex, size_t bytesCount) {
		boost::mutex::scoped_lock lock(m_connectionsMutex);
		return GetConnection(connectionIndex, lock).ClearReceived(bytesCount);
	}

private:

	void StartAccept() {
		boost::shared_ptr<Connection> newConnection(
			Connection::Create(m_acceptor.io_service()));
		m_acceptor.async_accept(
			newConnection->socket(),
			boost::bind(
				&Implementation::HandleAccept,
				this,
				boost::shared_ptr<Connection>(newConnection),
				io::placeholders::error));
	}

	void HandleAccept(
				boost::shared_ptr<Connection> newConnection,
				const boost::system::error_code &error) {
		if (!error) {
			{
				boost::mutex::scoped_lock lock(m_connectionsMutex);
				newConnection->Start();
				m_connections.push_back(newConnection);
			}
			StartAccept();
		}
	}

	Connection & GetConnection(
				size_t connectionIndex,
				const boost::mutex::scoped_lock &) {
		if (connectionIndex >= m_connections.size()) {
			throw std::logic_error("Could not find connection by index");
		}
		return *m_connections[connectionIndex];
	}

	const Connection & GetConnection(
				size_t connectionIndex,
				const boost::mutex::scoped_lock &lock)
			const {
		return const_cast<Implementation *>(this)
			->GetConnection(connectionIndex, lock);
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
	m_pimpl->CloseConnection(connectionIndex);
}

void TcpServer::Send(size_t connectionIndex, const std::string &message)  {
	m_pimpl->Send(connectionIndex, message);
}

void TcpServer::Send(size_t connectionIndex, const Buffer &data) {
	m_pimpl->Send(connectionIndex, data);
}

Buffer::size_type TcpServer::GetReceivedSize(size_t connectionIndex) const {
	return m_pimpl->GetReceivedSize(connectionIndex);
}

Buffer TcpServer::GetReceived(size_t connectionIndex) const {
	return m_pimpl->GetReceived(connectionIndex);
}

void TcpServer::ClearReceived(size_t connectionIndex, size_t bytesCount) {
	return m_pimpl->ClearReceived(connectionIndex, bytesCount);
}

//////////////////////////////////////////////////////////////////////////
