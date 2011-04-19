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

namespace io = boost::asio;
using namespace Test;

//////////////////////////////////////////////////////////////////////////

class TcpServer::Implementation {

private:

	class Connection : public boost::enable_shared_from_this<Connection> {
	
	public:
		
		static std::auto_ptr<Connection> Create(io::io_service &ioService) {
			return std::auto_ptr<Connection>(new Connection(ioService));
		}

		io::ip::tcp::socket & socket() {
			return m_socket;
		}

	public:

		void Start() {
			StartRead();
		}

		template<class T>
		void Send(const T &data) {
			async_write(
				m_socket,
				io::buffer(data),
				boost::bind(
					&Connection::HandleWrite,
					shared_from_this(),
					io::placeholders::error,
					io::placeholders::bytes_transferred));
		}

		Buffer GetReceived() const {
			Buffer result;
			{
				boost::mutex::scoped_lock lock(m_bufferMutex);
				result = m_dataBuffer;
			}
			return result;
		}

		void ClearReceived() {
			boost::mutex::scoped_lock lock(m_bufferMutex);
			m_dataBuffer.clear();
		}

	private:

		explicit Connection(io::io_service &ioService)
				: m_socket(ioService) {
			//...//
		}

	private:

		void StartRead() {
			io::async_read_until(
				m_socket,
				m_inStreamBuffer,
				boost::regex(".+"),
				boost::bind(
					&Connection::HandleRead,
					shared_from_this(),
					io::placeholders::error,
					io::placeholders::bytes_transferred));
		}

		void HandleWrite(const boost::system::error_code &, size_t) {
			//...//
		}

		void HandleRead(const boost::system::error_code &error, size_t) {
			if (!error) {
				boost::mutex::scoped_lock lock(m_bufferMutex);
				std::istream is(&m_inStreamBuffer);
				is.unsetf(std::ios::skipws);
				std::copy(
					std::istream_iterator<char>(is), 
					std::istream_iterator<char>(), 
					std::back_inserter(m_dataBuffer));
			}
			StartRead();
		}

	private:

		io::ip::tcp::socket m_socket;
		Buffer m_dataBuffer;
		io::streambuf m_inStreamBuffer;
		mutable boost::mutex m_bufferMutex;
	
	};

	typedef std::vector<boost::shared_ptr<Connection> > Connections;

public:

	Implementation(const unsigned short port)
			: m_acceptor(m_ioService, io::ip::tcp::endpoint(io::ip::tcp::v4(), port)) {
		StartAccept();
		m_thread.reset(
			new boost::thread(
				boost::bind(&Implementation::AcceptorThreadMain, this)));
				
	}

	~Implementation() {
		m_ioService.stop();
		m_thread->join();
	}

private:

	Implementation(const Implementation &);
	const Implementation & operator =(const Implementation &);

public:

	unsigned int GetNumberOfAcceptedConnections() const {
		return unsigned int(m_connections.size());
	}

	void CloseConnection(size_t connectionIndex) {
		boost::mutex::scoped_lock lock(m_connectionsMutex);
		if (connectionIndex >= m_connections.size()) {
			throw std::logic_error("Could not find connection by index");
		}
		m_connections.erase(m_connections.begin() + connectionIndex);
	}

	void Send(size_t connectionIndex, const std::string &message) {
		BOOST_ASSERT(message.size());
		boost::mutex::scoped_lock lock(m_connectionsMutex);
		GetConnection(connectionIndex, lock).Send(message);
	}

	void Send(size_t connectionIndex, const Buffer &data) {
		BOOST_ASSERT(data.size());
		boost::mutex::scoped_lock lock(m_connectionsMutex);
		GetConnection(connectionIndex, lock).Send(data);
	}

	Buffer GetReceived(size_t connectionIndex) const {
		boost::mutex::scoped_lock lock(m_connectionsMutex);
		return GetConnection(connectionIndex, lock).GetReceived();
	}

	void ClearReceived(size_t connectionIndex) {
		boost::mutex::scoped_lock lock(m_connectionsMutex);
		return GetConnection(connectionIndex, lock).ClearReceived();
	}

private:

	void StartAccept() {
		boost::shared_ptr<Connection> newConnection(
			Connection::Create(m_acceptor.io_service()).release());
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

	Connection & GetConnection(size_t connectionIndex, boost::mutex::scoped_lock &) {
		if (connectionIndex >= m_connections.size()) {
			throw std::logic_error("Could not find connection by index");
		}
		return *m_connections[connectionIndex];
	}

	const Connection & GetConnection(
				size_t connectionIndex,
				boost::mutex::scoped_lock &lock)
			const {
		return const_cast<Implementation *>(this)
			->GetConnection(connectionIndex, lock);
	}

	void AcceptorThreadMain() {
		try {
			m_ioService.run();
		} catch (const std::exception &) {
			BOOST_ASSERT(false);
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

bool TcpServer::IsConnected() const {
	return m_pimpl->GetNumberOfAcceptedConnections() > 0;
}

unsigned int TcpServer::GetNumberOfAcceptedConnections() const {
	return m_pimpl->GetNumberOfAcceptedConnections();
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

Buffer TcpServer::GetReceived(size_t connectionIndex) const {
	return m_pimpl->GetReceived(connectionIndex);
}

void TcpServer::ClearReceived(size_t connectionIndex) {
	return m_pimpl->ClearReceived(connectionIndex);
}

//////////////////////////////////////////////////////////////////////////

class TcpClient::Implementation {

public:

	Implementation(unsigned short port)
			: m_resolver(m_ioService),
			m_query(io::ip::tcp::v4(), "localhost", boost::lexical_cast<std::string>(port)),
			m_endpointIterator(m_resolver.resolve(m_query)),
			m_socket(m_ioService, io::ip::tcp::endpoint(io::ip::tcp::v4(), 0)) {
		boost::system::error_code error = io::error::host_not_found;
		io::ip::tcp::resolver::iterator end;
		while (error && m_endpointIterator != end) {
			m_socket.close();
			m_socket.connect(*m_endpointIterator++, error);
		}
		if (error) {
			throw boost::system::system_error(error);
		}
	}

	~Implementation() {
		//...//
	}

private:

	Implementation(const Implementation &);
	const Implementation & operator =(const Implementation &);

public:

	void Send(const std::string &message) {
		BOOST_ASSERT(message.size());
		try {
			const size_t toSend = (message.size() + 1) * sizeof(std::string::value_type);
			const size_t sent = m_socket.send(io::buffer(message.c_str(), toSend));
			if (sent != toSend) {
				throw SendError("Could not send data to TCP: sent not equal toSend.");
			}
		} catch (const std::exception &ex) {
			throw SendError(ex.what());
		}
	}

	void Send(const Buffer &data) {
		BOOST_ASSERT(data.size());
		try {
			const size_t sent = m_socket.send(io::buffer(data));
			if (sent != data.size() * sizeof(Buffer::value_type)) {
				throw SendError("Could not send data to TCP: sent not equal buffer size.");
			}
		} catch (const std::exception &ex) {
			throw SendError(ex.what());
		}
	}

	Buffer Receive() {
		Buffer result(255);
		try {
			const size_t received = m_socket.receive(io::buffer(result));
			result.resize(received);
		} catch (const std::exception &ex) {
			throw ReceiveError(ex.what());
		}
		return result;		
	}

private:

	io::io_service m_ioService;
	io::ip::tcp::resolver m_resolver;
	mutable io::ip::tcp::socket m_socket;
	io::ip::tcp::resolver::query m_query;
	io::ip::tcp::resolver::iterator m_endpointIterator;
	io::ip::tcp::resolver::iterator end;


};

//////////////////////////////////////////////////////////////////////////


TcpClient::TcpClient(unsigned short port)
		: m_pimpl(new Implementation(port)) {
	//...//
}

TcpClient::~TcpClient() {
	//...//
}

Buffer TcpClient::Receive() {
	return m_pimpl->Receive();
}

void TcpClient::Send(const std::string &message) {
	m_pimpl->Send(message);
}

void TcpClient::Send(const Buffer &beffer) {
	m_pimpl->Send(beffer);
}

//////////////////////////////////////////////////////////////////////////

class UdpClient::Implementation {

public:

	Implementation(unsigned short port)
			: m_socket(m_ioService, io::ip::udp::endpoint(io::ip::udp::v4(), 0)),
			m_resolver(m_ioService),
			m_query(io::ip::udp::v4(), "localhost", boost::lexical_cast<std::string>(port)),
			m_resolverIterator(m_resolver.resolve(m_query)) {
		//...//
	}

	~Implementation() {
		//...//
	}

private:

	Implementation(const Implementation &);
	const Implementation & operator =(const Implementation &);

public:

	void Send(const std::string &message) {
		BOOST_ASSERT(message.size());
		try {
			const size_t toSend = (message.size() + 1) * sizeof(std::string::value_type);
			const size_t sent = m_socket.send_to(
				io::buffer(message.c_str(), toSend),
				*m_resolverIterator);
			if (sent != toSend) {
				throw SendError("Could not send data to UDP: sent not equal toSend.");
			}
		} catch (const std::exception &ex) {
			throw SendError(ex.what());
		}
	}

	void Send(const Buffer &data) {
		BOOST_ASSERT(data.size());
		try {
			const size_t sent = m_socket.send_to(io::buffer(data), *m_resolverIterator);
			if (sent != data.size() * sizeof(Buffer::value_type)) {
				throw SendError("Could not send data to UDP: sent not equal buffer size.");
			}
		} catch (const std::exception &ex) {
			throw SendError(ex.what());
		}
	}

	Buffer Receive() {
		Buffer result(255);
		try {
			io::ip::udp::endpoint senderEndpoint;
			const size_t received
				= m_socket.receive_from(io::buffer(result), senderEndpoint);
			result.resize(received);
		} catch (const std::exception &ex) {
			throw ReceiveError(ex.what());
		}
		return result;	
	}

private:

	io::io_service m_ioService;
	mutable io::ip::udp::socket m_socket;
	io::ip::udp::resolver m_resolver;
	io::ip::udp::resolver::query m_query;
	io::ip::udp::resolver::iterator m_resolverIterator;

};

UdpClient::UdpClient(unsigned short port)
		: m_pimpl(new Implementation(port)) {
	//...//
}

UdpClient::~UdpClient() {
	//...//
}

void UdpClient::Send(const std::string &message) {
	m_pimpl->Send(message);
}

void UdpClient::Send(const Buffer &buffer) {
	m_pimpl->Send(buffer);
}

Buffer UdpClient::Receive() {
	return m_pimpl->Receive();
}

//////////////////////////////////////////////////////////////////////////
