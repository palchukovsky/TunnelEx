/**************************************************************************
 *   Created: 2011/07/12 22:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "PipeConnection.hpp"

using namespace TestUtil;

PipeConnection::PipeConnection(HANDLE handle)
		: m_handle(handle),
		m_dataBufferStart(m_dataBuffer.end()),
		m_dataBufferSize(0),
		m_isActive(true),
		m_receiveBuffer(256, 0) {
	assert(
		std::distance(
			m_dataBufferStart,
			const_cast<const Self *>(this)->m_dataBuffer.end())
		== int(m_dataBufferSize));
	UpdateBufferState();
	m_readThread.reset(
		new boost::thread(boost::bind(&Self::ReadThreadMain, this)));
}

PipeConnection::~PipeConnection() {
	try {
		Close();
		assert(m_handle == NULL);
		m_readThread->join();
	} catch (...) {
		assert(false);
	}
}

void PipeConnection::Close() {
	if (!BOOST_INTERLOCKED_EXCHANGE(&m_isActive, 0)) {
		return;
	}
	boost::mutex::scoped_lock lock(m_mutex);
	assert(!m_isActive);
	if (m_handle == NULL) {
		return;
	}
	CloseHandle(m_handle);
	m_handle = NULL;
}

Buffer::size_type PipeConnection::GetReceivedSize() const {
	boost::mutex::scoped_lock lock(m_mutex);
	return m_dataBufferSize;
}

void PipeConnection::GetReceived(Buffer::size_type maxSize, Buffer &destination) const {
	Buffer destinationTmp;
	{
		boost::mutex::scoped_lock lock(m_mutex);
		if (m_dataBufferSize > 0) {
			assert(m_dataBufferStart != m_dataBuffer.end());
			assert(
				std::distance(
					m_dataBufferStart,
					const_cast<const Self *>(this)->m_dataBuffer.end())
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

void PipeConnection::ClearReceived(size_t bytesCount /*= 0*/) {
	boost::mutex::scoped_lock lock(m_mutex);
	assert(bytesCount <= m_dataBufferSize);
	assert(
		int(bytesCount)
		<= std::distance(
			m_dataBufferStart,
			const_cast<const Self *>(this)->m_dataBuffer.end()));
	if (bytesCount == 0) {
		m_dataBufferSize = 0;
		UpdateBufferState();
	} else {
		m_dataBufferSize -= bytesCount;
		UpdateBufferState();
		assert(
			std::distance(
				m_dataBufferStart,
				const_cast<const Self *>(this)->m_dataBuffer.end())
			== int(m_dataBufferSize));
	}
}

bool PipeConnection::IsActive() const {
	return m_isActive != 0;
}

bool PipeConnection::WaitDataReceiveEvent(
			const boost::system_time &waitUntil,
			Buffer::size_type minSize)
		const {
	boost::mutex::scoped_lock lock(m_mutex);
	for ( ; ; ) {
		if (m_dataBufferSize >= minSize) {
			return true;
		}
		if (!m_dataReceivedCondition.timed_wait(lock, waitUntil)) {
			return m_dataBufferSize >= minSize;
		}
	}
}

void PipeConnection::Send(std::auto_ptr<Buffer> data) {
	boost::mutex::scoped_lock lock(m_mutex);
	if (m_handle == NULL) {
		return;
	}
	DWORD sent = 0;
	if (!WriteFile(m_handle, &(*data)[0], DWORD(data->size()), &sent, NULL)) {
		throw SendError(
			"Could not send data to pipe: WriteFile returns FALSE.");
	} else if (sent != data->size()) {
		throw SendError(
			"Could not send data to pipe: sent not equal size");
	}
}

void PipeConnection::ReadThreadMain() {

	for ( ; m_isActive; ) {

		DWORD received = 0;
		assert(m_handle != NULL);
		const BOOL readFileResult = ReadFile(
			m_handle,
			&m_receiveBuffer[0],
			DWORD(m_receiveBuffer.size()),
			&received,
			NULL);
		assert(received <= m_receiveBuffer.size());
		assert(readFileResult || received == 0);
		
		if (received == 0) {
			Close();
			m_dataReceivedCondition.notify_all();
			break;
		}

		{
			boost::mutex::scoped_lock lock(m_mutex);
			m_dataBuffer.reserve(received);
			std::copy(
				m_receiveBuffer.begin(),
				m_receiveBuffer.begin() + received,
				std::back_inserter(m_dataBuffer));
			UpdateBufferState(received);
		}

		m_dataReceivedCondition.notify_all();

	}

}

void PipeConnection::UpdateBufferState() {
	UpdateBufferState(0);
}

void PipeConnection::UpdateBufferState(size_t addSize) {
	assert(!m_dataBuffer.empty() || addSize == 0);
	assert(m_dataBuffer.size() - m_dataBufferSize >= 0);
	m_dataBufferSize += addSize;
	m_dataBufferStart = m_dataBuffer.begin() + (m_dataBuffer.size() - m_dataBufferSize);
#	ifdef DEV_VER
		m_dataBufferPch = !m_dataBuffer.empty() ? &m_dataBuffer[0] : 0;
		m_dataBufferFullSize = m_dataBuffer.size();
		m_dataBufferStartPch = m_dataBufferStart != m_dataBuffer.end() ? &m_dataBufferStart[0] : 0;
#	endif
	assert(addSize <= m_dataBuffer.size());
	assert(
		std::distance(
			m_dataBufferStart,
			const_cast<const Self *>(this)->m_dataBuffer.end())
		== int(m_dataBufferSize));
}
