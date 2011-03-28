/**************************************************************************
 *   Created: 2008/01/12 21:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: InetServer.cpp 959 2010-06-22 11:36:04Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "InetServer.hpp"

using namespace std;
using namespace boost;
using namespace boost::asio;
using boost::asio::ip::udp;
using boost::asio::ip::tcp;
using namespace Test;

//////////////////////////////////////////////////////////////////////////

class TcpServer::Implementation {

private:

	class Connection : public enable_shared_from_this<Connection> {
	
	public:
		
		static auto_ptr<Connection> Create(asio::io_service &ioService) {
			return auto_ptr<Connection>(new Connection(ioService));
		}

		tcp::socket & socket() {
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
				buffer(data),
				bind(
					&Connection::HandleWrite,
					shared_from_this(),
					asio::placeholders::error,
					asio::placeholders::bytes_transferred));
		}

		Buffer GetReceived() const {
			Buffer result;
			{
				mutex::scoped_lock lock(m_bufferMutex);
				result = m_dataBuffer;
			}
			return result;
		}

		void ClearReceived() {
			mutex::scoped_lock lock(m_bufferMutex);
			m_dataBuffer.clear();
		}

	private:

		explicit Connection(io_service &ioService)
				: m_socket(ioService) {
			//...//
		}

	private:

		void StartRead() {
			async_read_until(
				m_socket,
				m_inStreamBuffer,
				regex(".+"),
				bind(
					&Connection::HandleRead,
					shared_from_this(),
					placeholders::error,
					placeholders::bytes_transferred));
		}

		void HandleWrite(const system::error_code &, size_t) {
			//...//
		}

		void HandleRead(const system::error_code &error, size_t) {
			if (!error) {
				mutex::scoped_lock lock(m_bufferMutex);
				istream is(&m_inStreamBuffer);
				is.unsetf(ios_base::skipws);
				copy(
					istream_iterator<char>(is), 
					istream_iterator<char>(), 
					back_inserter(m_dataBuffer));
			}
			StartRead();
		}

	private:

		tcp::socket m_socket;
		Buffer m_dataBuffer;
		asio::streambuf m_inStreamBuffer;
		mutable mutex m_bufferMutex;
	
	};

	typedef vector<shared_ptr<Connection> > Connections;

public:

	Implementation(const unsigned short port)
			: m_acceptor(m_ioService, tcp::endpoint(tcp::v4(), port)) {
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
		mutex::scoped_lock lock(m_connectionsMutex);
		if (connectionIndex >= m_connections.size()) {
			throw logic_error("Could not find connection by index");
		}
		m_connections.erase(m_connections.begin() + connectionIndex);
	}

	void Send(size_t connectionIndex, const string &message) throw(SendError) {
		BOOST_ASSERT(message.size());
		mutex::scoped_lock lock(m_connectionsMutex);
		GetConnection(connectionIndex, lock).Send(message);
	}

	void Send(size_t connectionIndex, const Buffer &data) throw(SendError) {
		BOOST_ASSERT(data.size());
		mutex::scoped_lock lock(m_connectionsMutex);
		GetConnection(connectionIndex, lock).Send(data);
	}

	Buffer GetReceived(size_t connectionIndex) const throw(ReceiveError) {
		mutex::scoped_lock lock(m_connectionsMutex);
		return GetConnection(connectionIndex, lock).GetReceived();
	}

	void ClearReceived(size_t connectionIndex) {
		mutex::scoped_lock lock(m_connectionsMutex);
		return GetConnection(connectionIndex, lock).ClearReceived();
	}

private:

	void StartAccept() {
		shared_ptr<Connection> newConnection(
			Connection::Create(m_acceptor.io_service()).release());
		m_acceptor.async_accept(
			newConnection->socket(),
			bind(
				&Implementation::HandleAccept,
				this,
				shared_ptr<Connection>(newConnection),
				placeholders::error));
	}

	void HandleAccept(
				shared_ptr<Connection> newConnection,
				const system::error_code &error) {
		if (!error) {
			{
				mutex::scoped_lock lock(m_connectionsMutex);
				newConnection->Start();
				m_connections.push_back(newConnection);
			}
			StartAccept();
		}
	}

	Connection & GetConnection(size_t connectionIndex, mutex::scoped_lock &) {
		if (connectionIndex >= m_connections.size()) {
			throw logic_error("Could not find connection by index");
		}
		return *m_connections[connectionIndex];
	}

	const Connection & GetConnection(
				size_t connectionIndex,
				mutex::scoped_lock &lock)
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

	io_service m_ioService;
	tcp::acceptor m_acceptor;
	Connections m_connections;
	shared_ptr<boost::thread> m_thread;
	mutable mutex m_connectionsMutex;

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

void TcpServer::Send(size_t connectionIndex, const string &message) throw(SendError) {
	m_pimpl->Send(connectionIndex, message);
}

void TcpServer::Send(size_t connectionIndex, const Buffer &data) throw(SendError) {
	m_pimpl->Send(connectionIndex, data);
}

Buffer TcpServer::GetReceived(
			size_t connectionIndex)
		const
		throw(ReceiveError) {
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
			m_query(tcp::v4(), "localhost", lexical_cast<string>(port)),
			m_endpointIterator(m_resolver.resolve(m_query)),
			m_socket(m_ioService, tcp::endpoint(tcp::v4(), 0)) {
		system::error_code error = asio::error::host_not_found;
		tcp::resolver::iterator end;
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

	void Send(const string &message) throw(SendError) {
		BOOST_ASSERT(message.size());
		try {
			const size_t toSend = (message.size() + 1) * sizeof(string::value_type);
			const size_t sent = m_socket.send(buffer(message.c_str(), toSend));
			if (sent != toSend) {
				throw SendError("Could not send data to TCP: sent not equal toSend.");
			}
		} catch (const std::exception &ex) {
			throw SendError(ex.what());
		}
	}

	void Send(const Buffer &data) throw(SendError) {
		BOOST_ASSERT(data.size());
		try {
			const size_t sent = m_socket.send(buffer(data));
			if (sent != data.size() * sizeof(Buffer::value_type)) {
				throw SendError("Could not send data to TCP: sent not equal buffer size.");
			}
		} catch (const std::exception &ex) {
			throw SendError(ex.what());
		}
	}

	Buffer Receive() throw(ReceiveError) {
		Buffer result(255);
		try {
			const size_t received = m_socket.receive(buffer(result));
			result.resize(received);
		} catch (const std::exception &ex) {
			throw ReceiveError(ex.what());
		}
		return result;		
	}

private:

	io_service m_ioService;
	tcp::resolver m_resolver;
	mutable tcp::socket m_socket;
	tcp::resolver::query m_query;
	tcp::resolver::iterator m_endpointIterator;
	tcp::resolver::iterator end;


};

//////////////////////////////////////////////////////////////////////////


TcpClient::TcpClient(unsigned short port)
		: m_pimpl(new Implementation(port)) {
	//...//
}

TcpClient::~TcpClient() {
	//...//
}

Buffer TcpClient::Receive() throw(ReceiveError) {
	return m_pimpl->Receive();
}

void TcpClient::Send(const string &message) throw(SendError) {
	m_pimpl->Send(message);
}

void TcpClient::Send(const Buffer &beffer) throw(SendError) {
	m_pimpl->Send(beffer);
}

//////////////////////////////////////////////////////////////////////////

class UdpClient::Implementation {

public:

	Implementation(unsigned short port)
			: m_socket(m_ioService, udp::endpoint(udp::v4(), 0)),
			m_resolver(m_ioService),
			m_query(udp::v4(), "localhost", lexical_cast<string>(port)),
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

	void Send(const string &message) throw(SendError) {
		BOOST_ASSERT(message.size());
		try {
			const size_t toSend = (message.size() + 1) * sizeof(string::value_type);
			const size_t sent = m_socket.send_to(
				buffer(message.c_str(), toSend),
				*m_resolverIterator);
			if (sent != toSend) {
				throw SendError("Could not send data to UDP: sent not equal toSend.");
			}
		} catch (const std::exception &ex) {
			throw SendError(ex.what());
		}
	}

	void Send(const Buffer &data) throw(SendError) {
		BOOST_ASSERT(data.size());
		try {
			const size_t sent = m_socket.send_to(buffer(data), *m_resolverIterator);
			if (sent != data.size() * sizeof(Buffer::value_type)) {
				throw SendError("Could not send data to UDP: sent not equal buffer size.");
			}
		} catch (const std::exception &ex) {
			throw SendError(ex.what());
		}
	}

	Buffer Receive() throw(ReceiveError) {
		Buffer result(255);
		try {
			udp::endpoint senderEndpoint;
			const size_t received
				= m_socket.receive_from(buffer(result), senderEndpoint);
			result.resize(received);
		} catch (const std::exception &ex) {
			throw ReceiveError(ex.what());
		}
		return result;	
	}

private:

	io_service m_ioService;
	mutable udp::socket m_socket;
	udp::resolver m_resolver;
	udp::resolver::query m_query;
	udp::resolver::iterator m_resolverIterator;

};

UdpClient::UdpClient(unsigned short port)
		: m_pimpl(new Implementation(port)) {
	//...//
}

UdpClient::~UdpClient() {
	//...//
}

void UdpClient::Send(const string &message) throw(SendError) {
	m_pimpl->Send(message);
}

void UdpClient::Send(const Buffer &buffer) throw(SendError) {
	m_pimpl->Send(buffer);
}

Buffer UdpClient::Receive() throw(ReceiveError) {
	return m_pimpl->Receive();
}

//////////////////////////////////////////////////////////////////////////
