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
#include "Core/Error.hpp"

using namespace TestUtil;

PipeConnection::PipeConnection(HANDLE handle)
		: m_handle(handle),
		m_dataBufferStart(m_dataBuffer.end()),
		m_dataBufferSize(0),
		m_isActive(false),
		m_receiveBuffer(GetBufferSize(), 0) {
	assert(
		std::distance(
			m_dataBufferStart,
			const_cast<const Self *>(this)->m_dataBuffer.end())
		== int(m_dataBufferSize));
	UpdateBufferState();
}

PipeConnection::~PipeConnection() {
	try {
		Close();
		assert(m_handle == INVALID_HANDLE_VALUE);
		m_readThread->join();
	} catch (...) {
		assert(false);
	}
}

void PipeConnection::Start() {
	assert(!m_isActive);
	m_readThread.reset(
		new boost::thread(boost::bind(&Self::ReadThreadMain, this)));
}

size_t PipeConnection::GetBufferSize() {
	return 1024;
}

void PipeConnection::Close() {
	if (!BOOST_INTERLOCKED_EXCHANGE(&m_isActive, 0)) {
		return;
	}
	boost::mutex::scoped_lock lock(m_stateMutex);
	assert(!m_isActive);
	if (m_handle == INVALID_HANDLE_VALUE) {
		return;
	}
	FlushFileBuffers(m_handle); 
	DisconnectNamedPipe(m_handle); 
	CloseHandle(m_handle);
	m_handle = INVALID_HANDLE_VALUE;
}

Buffer::size_type PipeConnection::GetReceivedSize() const {
	boost::mutex::scoped_lock lock(m_stateMutex);
	return m_dataBufferSize;
}

void PipeConnection::GetReceived(Buffer::size_type maxSize, Buffer &destination) const {
	Buffer destinationTmp;
	{
		boost::mutex::scoped_lock lock(m_stateMutex);
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
	boost::mutex::scoped_lock lock(m_stateMutex);
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
	boost::mutex::scoped_lock lock(m_stateMutex);
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
	boost::mutex::scoped_lock stateLock(m_stateMutex);
	if (m_handle == INVALID_HANDLE_VALUE) {
		return;
	}
	if (!CancelSynchronousIo(m_readThread->native_handle())) {
		const TunnelEx::Error error(GetLastError());
		if (error.GetErrorNo() != ERROR_NOT_FOUND) {
			std::cerr
				<< "Failed to cancel pipe IO: "
				<<  TunnelEx::ConvertString<TunnelEx::String>(error.GetString()).GetCStr()
				<< " (" << error.GetErrorNo() << ")." << std::endl;
			throw std::exception("Failed to cancel pipe IO");
		}
	}
	boost::mutex::scoped_lock ioLock(m_ioMutex);
	DWORD sent = 0;
	assert(data->size() > 0);
	if (!WriteFile(m_handle, &(*data)[0], DWORD(data->size()), &sent, NULL)) {
		throw SendError(
			"Could not send data to pipe: WriteFile returns FALSE.");
	} else if (sent != data->size()) {
		throw SendError(
			"Could not send data to pipe: sent not equal size");
	}
}

void PipeConnection::ReadThreadMain() {

	{
		const long prevActiveState = BOOST_INTERLOCKED_EXCHANGE(&m_isActive, 1);
		assert(!prevActiveState);
		UseUnused(prevActiveState);
	}

	try {

		for ( ; m_isActive; ) {

			DWORD received = 0;
			{
				boost::mutex::scoped_lock lock(m_ioMutex);
				ReadFile(
					m_handle,
					&m_receiveBuffer[0],
					DWORD(m_receiveBuffer.size()),
					&received,
					NULL);
				assert(received <= m_receiveBuffer.size());
			}
		
			const TunnelEx::Error error(GetLastError());
			if (error.IsError()) {
				bool isError = true;
				if (error.GetErrorNo() == ERROR_OPERATION_ABORTED) {
					boost::mutex::scoped_lock lock(m_stateMutex);
					isError = m_handle == INVALID_HANDLE_VALUE;
				}
				if (isError) {
					assert(received == 0);
					Close();
					m_dataReceivedCondition.notify_all();
					break;
				}
			}

			{
				// should be locked in any case, for Send method
				boost::mutex::scoped_lock lock(m_stateMutex);
				if (received > 0) {
					m_dataBuffer.reserve(received);
					std::copy(
						m_receiveBuffer.begin(),
						m_receiveBuffer.begin() + received,
						std::back_inserter(m_dataBuffer));
					UpdateBufferState(received);
				}
			}

			if (received > 0) {
				m_dataReceivedCondition.notify_all();
			}

		}

	} catch (const std::exception &ex) {
		std::cerr << "Error in pipe read thread: " << ex.what() << "." << std::endl;
	} catch (...) {
		std::cerr << "Unknown error in pipe read thread." << std::endl;
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

////////////////////////////////////////////////////////////////////////////////

PipeClientConnection::PipeClientConnection(
			const std::string &path,
			const boost::posix_time::time_duration &waitTimeout)
		: Base(INVALID_HANDLE_VALUE),
		m_path(path),
		m_waitTimeout(DWORD(waitTimeout.total_milliseconds())) {
	//...//
}

PipeClientConnection::~PipeClientConnection() {
	//...//
}

void PipeClientConnection::ReadThreadMain() {

	struct AutoHandle {
		HANDLE handle;
		AutoHandle(HANDLE handle = INVALID_HANDLE_VALUE)
				: handle(handle) {
			//...//
		}
		~AutoHandle() {
			if (handle != INVALID_HANDLE_VALUE) {
				CloseHandle(handle);
			}
		}
		bool IsValid() const {
			return handle != INVALID_HANDLE_VALUE;
		}
		HANDLE Release() throw() {
			const auto result = handle;
			handle = INVALID_HANDLE_VALUE;
			return result;
		}
	} handle;

	try {

		for ( ; ; ) {

			handle.handle = CreateFileA( 
				m_path.c_str(),
				GENERIC_READ | GENERIC_WRITE,
				0,
				NULL,
				OPEN_EXISTING,
				0,
				NULL);
			if (handle.IsValid()) {
				break;
			} else {
				TunnelEx::Error error(GetLastError());
				if (error.GetErrorNo() != ERROR_PIPE_BUSY) {
					std::cerr
						<< "Failed to open pipe \"" << m_path <<"\": "
						<<  TunnelEx::ConvertString<TunnelEx::String>(error.GetString()).GetCStr()
						<< " (" << error.GetErrorNo() << ")." << std::endl;
					return;
				}
			}

			if (!WaitNamedPipeA(m_path.c_str(), m_waitTimeout)) { 
				TunnelEx::Error error(GetLastError());
				std::cerr
					<< "Failed to wait pipe \"" << m_path <<"\": "
					<<  TunnelEx::ConvertString<TunnelEx::String>(error.GetString()).GetCStr()
					<< " (" << error.GetErrorNo() << ")." << std::endl;
				return;
			}

		}

		DWORD mode = PIPE_READMODE_MESSAGE; 
		if (!SetNamedPipeHandleState( 
				handle.handle,
				&mode,
				NULL,
				NULL)) {
			TunnelEx::Error error(GetLastError());
			std::cerr
				<< "Failed to set pipe mode for \"" << m_path <<"\": "
				<<  TunnelEx::ConvertString<TunnelEx::String>(error.GetString()).GetCStr()
				<< " (" << error.GetErrorNo() << ")." << std::endl;
			return;
		}

	} catch (const std::exception &ex) {
		std::cerr << "Error in client pipe read thread: " << ex.what() << "." << std::endl;
	} catch (...) {
		std::cerr << "Unknown error in client pipe read thread." << std::endl;
	}

	SetHandle(handle.Release());
	Base::ReadThreadMain();

}
