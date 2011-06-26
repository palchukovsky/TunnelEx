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
	Buffer::size_type result;
	{
		boost::mutex::scoped_lock lock(m_mutex);
		result = m_dataBufferSize;
	}
	return result;
}

void TcpConnection::GetReceived(
			Buffer::size_type maxSize,
			Buffer &destination)
		const {
	Buffer destinationTmp;
	{
		boost::mutex::scoped_lock lock(m_mutex);
		if (m_dataBufferSize > 0) {
			assert(m_dataBufferStart != m_dataBuffer.end());
			assert(
				std::distance(
					m_dataBufferStart,
					const_cast<const TcpConnection *>(this)->m_dataBuffer.end())
				== int(m_dataBufferSize));
			const Buffer::size_type size = std::min(m_dataBufferSize, maxSize);
			destinationTmp.reserve(size);
			copy(
				m_dataBufferStart,
				m_dataBufferStart + size,
				std::back_inserter(destinationTmp));
		}
	}
	destinationTmp.swap(destination);
}

void TcpConnection::ClearReceived(size_t bytesCount /*= 0*/) {
	boost::mutex::scoped_lock lock(m_mutex);
	assert(bytesCount <= m_dataBufferSize);
	assert(
		int(bytesCount)
		<= std::distance(
			m_dataBufferStart,
			const_cast<const TcpConnection *>(this)->m_dataBuffer.end()));
	if (bytesCount == 0) {
		m_dataBufferStart = m_dataBuffer.end();
		m_dataBufferSize = 0;
	} else {
		std::advance(m_dataBufferStart, bytesCount);
		m_dataBufferSize -= bytesCount;
		assert(
			std::distance(
				m_dataBufferStart,
				const_cast<const TcpConnection *>(this)->m_dataBuffer.end())
			== int(m_dataBufferSize));
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
		m_dataBufferStart(m_dataBuffer.end()),
		m_dataBufferSize(0),
		m_isActive(false) {
	assert(
		std::distance(
			m_dataBufferStart,
			const_cast<const TcpConnection *>(this)->m_dataBuffer.end())
		== int(m_dataBufferSize));
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

void TcpConnection::HandleRead(const boost::system::error_code &error, size_t size) {
	{
		boost::mutex::scoped_lock lock(m_mutex);
		assert(m_isActive);
		if (!error) {
			std::istream is(&m_inStreamBuffer);
			is.unsetf(std::ios::skipws);
			assert(m_dataBufferSize > 0 || m_dataBufferStart == m_dataBuffer.end());
			m_dataBuffer.reserve(size);
			std::copy(
				std::istream_iterator<char>(is), 
				std::istream_iterator<char>(), 
				std::back_inserter(m_dataBuffer));
			assert(!m_dataBuffer.empty());
			m_dataBufferSize += size;
			m_dataBufferStart
				= m_dataBuffer.begin() + (m_dataBuffer.size() - m_dataBufferSize);
			assert(size <= m_dataBuffer.size());
			assert(size <= m_dataBufferSize);
			assert(
				std::distance(
					m_dataBufferStart,
					const_cast<const TcpConnection *>(this)->m_dataBuffer.end())
				== int(m_dataBufferSize));
			StartRead();
		} else {
			m_isActive = false;
		}
	}
	m_dataReceivedCondition.notify_all();
}

bool TcpConnection::WaitDataReceiveEvent(
			const boost::system_time &waitUntil,
			Buffer::size_type size)
		const {
	for ( ; ; ) {
		boost::mutex::scoped_lock lock(m_mutex);
		if (m_dataBufferSize >= size) {
			return true;
		}
		if (!m_dataReceivedCondition.timed_wait(lock, waitUntil)) {
			return false;
		}
	}
}
