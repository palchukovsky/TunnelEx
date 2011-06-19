/**************************************************************************
 *   Created: 2011/06/05 3:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "InetConnection.hpp"

namespace io = boost::asio;
using namespace TestUtil;

boost::shared_ptr<TcpConnection> TcpConnection::Create(io::io_service &ioService) {
	return boost::shared_ptr<TcpConnection>(new TcpConnection(ioService));
}

io::ip::tcp::socket & TcpConnection::socket() {
	return m_socket;
}

void TcpConnection::Start() {
	assert(!m_isActive);
	m_isActive = true;
	StartRead();
}

Buffer::size_type TcpConnection::GetReceivedSize() const {
	size_t result;
	{
		boost::mutex::scoped_lock lock(m_mutex);
		result = m_dataBuffer.size();
	}
	return result;
}

Buffer TcpConnection::GetReceived() const {
	Buffer result;
	{
		boost::mutex::scoped_lock lock(m_mutex);
		result = m_dataBuffer;
	}
	return result;
}

void TcpConnection::ClearReceived(size_t bytesCount /*= 0*/) {
	boost::mutex::scoped_lock lock(m_mutex);
	if (bytesCount == 0 || bytesCount >= m_dataBuffer.size()) {
		m_dataBuffer.clear();
	} else {
		Buffer(m_dataBuffer.begin() + bytesCount, m_dataBuffer.end())
			.swap(m_dataBuffer);
	}
}

bool TcpConnection::IsActive() const {
	bool result;
	{
		boost::mutex::scoped_lock lock(m_mutex);
		result = m_isActive;
	}
	return result;
}

TcpConnection::TcpConnection(io::io_service &ioService)
		: m_socket(ioService),
		m_isActive(false) {
	//...//
}

void TcpConnection::StartRead() {
	io::async_read_until(
		m_socket,
		m_inStreamBuffer,
		boost::regex(".+"),
		boost::bind(
			&TcpConnection::HandleRead,
			shared_from_this(),
			io::placeholders::error,
			io::placeholders::bytes_transferred));
}

void TcpConnection::HandleWrite(const boost::system::error_code &, size_t) {
	//...//
}

void TcpConnection::HandleRead(const boost::system::error_code &error, size_t) {
	boost::mutex::scoped_lock lock(m_mutex);
	assert(m_isActive);
	if (!error) {
		std::istream is(&m_inStreamBuffer);
		is.unsetf(std::ios::skipws);
		std::copy(
			std::istream_iterator<char>(is), 
			std::istream_iterator<char>(), 
			std::back_inserter(m_dataBuffer));
		assert(!m_dataBuffer.empty());
		StartRead();
	} else {
		m_isActive = false;
	}
}
