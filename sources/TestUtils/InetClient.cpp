/**************************************************************************
 *   Created: 2011/06/05 2:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "InetClient.hpp"
#include "InetConnection.hpp"

using namespace TestUtil;
namespace io = boost::asio;

class TcpClient::Implementation : private boost::noncopyable {

private:

	typedef TcpConnection Connection;

public:

	Implementation(const std::string &host, unsigned short port)
			: m_connection(Connection::Create(m_ioService)) {

		io::ip::tcp::resolver resolver(m_ioService);
		io::ip::tcp::resolver::query query(
			io::ip::tcp::v4(),
			host,
			boost::lexical_cast<std::string>(port));
		io::ip::tcp::resolver::iterator endpointIterator(resolver.resolve(query));
		io::ip::tcp::endpoint endpoint = *endpointIterator;

		m_connection->socket().async_connect(
			endpoint,
			boost::bind(
				&Implementation::HandleConnect,
				this,
				io::placeholders::error,
				++endpointIterator));

		m_thread.reset(
			new boost::thread(
				boost::bind(&Implementation::ServiceThreadMain, this)));

	}

	~Implementation() {
		m_ioService.stop();
		m_thread->join();
	}

public:

	Connection & GetConnection() {
		if (!m_connection.get()) {
			throw ConnectionClosed();
		}
		return *m_connection;
	}
	const Connection & GetConnection() const {
		return const_cast<Implementation *>(this)->GetConnection();
	}

	void Disconnect() {
		if (!m_connection.get()) {
			throw ConnectionClosed();
		}
		m_connection.reset();
	}

private:

	void HandleConnect(
				const boost::system::error_code &error,
				io::ip::tcp::resolver::iterator endpointIterator) {
		assert(!m_connection->IsActive());
		if (!error) {
			m_connection->Start();
		} else if (endpointIterator != io::ip::tcp::resolver::iterator()) {
			m_connection->socket().close();
			io::ip::tcp::endpoint endpoint = *endpointIterator;
			m_connection->socket().async_connect(
				endpoint,
				boost::bind(
					&Implementation::HandleConnect,
					this,
					io::placeholders::error,
					++endpointIterator));
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
	boost::shared_ptr<TcpConnection> m_connection;
	boost::shared_ptr<boost::thread> m_thread;


};

//////////////////////////////////////////////////////////////////////////

TcpClient::TcpClient(const std::string &host, unsigned short port)
		: m_pimpl(new Implementation(host, port)) {
	//...//
}

TcpClient::~TcpClient() {
	//...//
}

Buffer::size_type TcpClient::GetReceivedSize() const {
	return m_pimpl->GetConnection().GetReceivedSize();
}

Buffer TcpClient::GetReceived() const {
	return m_pimpl->GetConnection().GetReceived();
}

void TcpClient::Send(const std::string &message) {
	m_pimpl->GetConnection().Send(message);
}

void TcpClient::Send(const Buffer &beffer) {
	m_pimpl->GetConnection().Send(beffer);
}

bool TcpClient::IsConnected() const {
	return m_pimpl->GetConnection().IsActive();
}

void TcpClient::ClearReceived(size_t bytesCount /*= 0*/) {
	return m_pimpl->GetConnection().ClearReceived(bytesCount);
}

void TcpClient::Disconnect() {
	return m_pimpl->Disconnect();
}

//////////////////////////////////////////////////////////////////////////

class UdpClient::Implementation : private boost::noncopyable {

public:

	Implementation(const std::string &host, unsigned short port)
			: m_socket(m_ioService, io::ip::udp::endpoint(io::ip::udp::v4(), 0)),
			m_resolver(m_ioService),
			m_query(io::ip::udp::v4(), host, boost::lexical_cast<std::string>(port)),
			m_resolverIterator(m_resolver.resolve(m_query)) {
		//...//
	}

	~Implementation() {
		//...//
	}

public:

	void Send(const std::string &message) {
		assert(message.size());
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
		assert(data.size());
		try {
			const size_t sent = m_socket.send_to(io::buffer(data), *m_resolverIterator);
			if (sent != data.size() * sizeof(Buffer::value_type)) {
				throw SendError("Could not send data to UDP: sent not equal buffer size.");
			}
		} catch (const std::exception &ex) {
			throw SendError(ex.what());
		}
	}

	Buffer GetReceived() const {
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
	Buffer m_buffer;

};

UdpClient::UdpClient(const std::string &host, unsigned short port)
		: m_pimpl(new Implementation(host, port)) {
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

Buffer::size_type UdpClient::GetReceivedSize() const {
	assert(false);
	// FIXME
	throw 0;
}

Buffer UdpClient::GetReceived() const {
	return m_pimpl->GetReceived();
}

bool UdpClient::IsConnected() const {
	assert(false);
	// FIXME
	throw 0;
}

void UdpClient::ClearReceived(size_t /*bytesCount*/ /*= 0*/) {
	assert(false);
	// FIXME
}

//////////////////////////////////////////////////////////////////////////


