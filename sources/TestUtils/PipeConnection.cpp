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

#define TEST_UTIL_TRAFFIC_PIPE_CONNECTION_LOGGIN 0

namespace {

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

	};

}

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
	
	m_overlaped.hHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(m_overlaped.hHandle != INVALID_HANDLE_VALUE);

}

PipeConnection::~PipeConnection() {
	try {
		Close();
		assert(m_handle == INVALID_HANDLE_VALUE);
	} catch (...) {
		assert(false);
	}
	CloseHandle(m_overlaped.hEvent);
}

size_t PipeConnection::GetBufferSize() {
	return 64;
}

void PipeConnection::Close() {
	Close(boost::mutex::scoped_lock(m_stateMutex));
}

void PipeConnection::Close(const boost::mutex::scoped_lock &) {
	if (m_handle == INVALID_HANDLE_VALUE) {
		return;
	}
	AutoHandle handle(m_handle);
	m_handle = INVALID_HANDLE_VALUE;
	FlushFileBuffers(handle.handle); 
	DisconnectNamedPipe(handle.handle);
}

Buffer::size_type PipeConnection::GetReceivedSize() const {
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
		BOOST_INTERLOCKED_EXCHANGE(&m_dataBufferSize, 0);
		UpdateBufferState();
	} else {
		BOOST_INTERLOCKED_EXCHANGE(&m_dataBufferSize, m_dataBufferSize - bytesCount);
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

std::auto_ptr<boost::mutex::scoped_lock> PipeConnection::CancelSyncIo() {
	while (!m_ioMutex.try_lock()) {
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
	}
	std::auto_ptr<boost::mutex::scoped_lock> ioLock;
	try {
		ioLock.reset(new boost::mutex::scoped_lock(m_ioMutex, boost::adopt_lock_t()));
	} catch (...) {
		m_ioMutex.unlock();
		throw;
	}
	return ioLock;
}

void PipeConnection::Send(std::auto_ptr<Buffer> data) {

	boost::mutex::scoped_lock stateLock(m_stateMutex);
	if (m_handle == INVALID_HANDLE_VALUE) {
		return;
	}

	DWORD sent = 0;
	assert(data->size() > 0);
	if (!WriteFile(m_handle, &(*data)[0], DWORD(data->size()), &sent, NULL)) {
		const TunnelEx::Error error(GetLastError());
		std::cerr
			<< "Failed to send data to pipe: "
			<<  TunnelEx::ConvertString<TunnelEx::String>(error.GetString()).GetCStr()
			<< " (" << error.GetErrorNo() << ")." << std::endl;
		throw SendError(
			"Could not send data to pipe: WriteFile returns FALSE.");
	} else if (sent != data->size()) {
		throw SendError(
			"Could not send data to pipe: sent not equal size");
	}

#	if TEST_UTIL_TRAFFIC_PIPE_CONNECTION_LOGGIN != 0
	{
		std::ostringstream oss;
		oss << this << ".PipeConnection.write";
		std::ofstream of(oss.str().c_str(), std::ios::binary |  std::ios::app);
		if (!data->empty()) {
			of.write(&(*data)[0], data->size());
		} else {
			of << "[ZERO]";
		}
	}
#	endif

}

void PipeConnection::Read() {

	boost::mutex::scoped_lock lock(m_stateMutex);
	if (m_handle == INVALID_HANDLE_VALUE) {
		return;
	}

	DWORD received = 0;
	ReadFile(
		m_handle,
		&m_receiveBuffer[0],
		DWORD(m_receiveBuffer.size()),
		&received,
		&m_overlaped);
	const TunnelEx::Error error(GetLastError());
	assert(received <= m_receiveBuffer.size());

	if (error.IsError()) {
		assert(received == 0);
		Close(lock);
	} else {
		if (received > 0) {
			m_dataBuffer.reserve(received);
			std::copy(
				m_receiveBuffer.begin(),
				m_receiveBuffer.begin() + received,
				std::back_inserter(m_dataBuffer));
			UpdateBufferState(received);
		}
	}

	lock.unlock();
	m_dataReceivedCondition.notify_all();

#	if TEST_UTIL_TRAFFIC_PIPE_CONNECTION_LOGGIN != 0
	{
		std::ostringstream oss;
		oss << this << ".PipeConnection.read";
		std::ofstream of(oss.str().c_str(), std::ios::binary |  std::ios::app);
		if (received > 0) {
			of.write(&m_receiveBuffer[0], received);
		} else {
			of << "[ZERO]";
		}
		if (m_handle == INVALID_HANDLE_VALUE) {
			of << "[CLOSED]";
		}
		if (error.IsError()) {
			of << "[ERROR " << error.GetErrorNo() << "]";
		}
	}
#	endif

}

void PipeConnection::UpdateBufferState() {
	UpdateBufferState(0);
}

void PipeConnection::UpdateBufferState(size_t addSize) {
	assert(!m_dataBuffer.empty() || addSize == 0);
	assert(m_dataBuffer.size() - m_dataBufferSize >= 0);
	BOOST_INTERLOCKED_EXCHANGE(&m_dataBufferSize, m_dataBufferSize += addSize);
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

HANDLE PipeConnection::GetEvent() {
	return m_overlaped.hEvent;
}

OVERLAPPED & PipeConnection::GetOverlaped() {
	return m_overlaped;
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

	AutoHandle handle;

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

}

////////////////////////////////////////////////////////////////////////////////

PipeServerConnection::PipeServerConnection(const std::string &path)
		: Base(INVALID_HANDLE_VALUE) {

	AutoHandle handle = CreateNamedPipeA(
		path.c_str(),
		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
		PIPE_TYPE_MESSAGE
			| PIPE_READMODE_MESSAGE
			| PIPE_WAIT,
		PIPE_UNLIMITED_INSTANCES,
		Connection::GetBufferSize(),
		Connection::GetBufferSize(),
		0,
		NULL);
	if (!handle.IsValid()) {
		TunnelEx::Error error(GetLastError());
		std::cerr
			<< "Failed to create pipe: "
			<<  TunnelEx::ConvertString<TunnelEx::String>(error.GetString()).GetCStr()
			<< " (" << error.GetErrorNo() << ")." << std::endl;
		throw std::exception("Failed to create pipe");
	}

	const auto connectResult
		= ConnectNamedPipe(handle.handle, &GetOverlaped());
	assert(connectResult == 0);
	UseUnused(connectResult);

	bool isPending = false;
	switch (GetLastError()) { 
		case ERROR_IO_PENDING: 
			isPending = true;
			break;
		case ERROR_PIPE_CONNECTED:
			isPending = false;
			break;
		default:
			{
				TunnelEx::Error error(GetLastError());
				std::cerr
					<< "Failed to create pipe: "
					<<  TunnelEx::ConvertString<TunnelEx::String>(error.GetString()).GetCStr()
					<< " (" << error.GetErrorNo() << ")." << std::endl;
				throw std::exception("An error occurs during the pipe connect operation.");
			}
	}

	SetHandle(handle.Release());

}

