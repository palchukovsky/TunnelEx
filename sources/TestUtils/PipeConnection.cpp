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
		m_isActive(true),
		m_receiveBuffer(GetBufferSize(), 0),
		m_isConnectionState(false) {

	assert(
		std::distance(
			m_dataBufferStart,
			const_cast<const Self *>(this)->m_dataBuffer.end())
		== int(m_dataBufferSize));
	UpdateBufferState();
	
	m_overlaped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(m_overlaped.hEvent != INVALID_HANDLE_VALUE);

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
	if (!BOOST_INTERLOCKED_EXCHANGE(&m_isActive, 0)) {
		return;
	}
	Close(boost::mutex::scoped_lock(m_stateMutex));
}

void PipeConnection::Close(const boost::mutex::scoped_lock &) {
	if (!BOOST_INTERLOCKED_EXCHANGE(&m_isActive, 0)) {
		return;
	}
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
			const Buffer::size_type size = std::min(Buffer::size_type(m_dataBufferSize), maxSize);
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
	assert(bytesCount <= size_t(m_dataBufferSize));
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
		if (Buffer::size_type(m_dataBufferSize) >= minSize) {
			return true;
		}
		if (!m_dataReceivedCondition.timed_wait(lock, waitUntil)) {
			return Buffer::size_type(m_dataBufferSize) >= minSize;
		}
	}
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
		std::cerr
			<< "Failed to read from pipe: "
			<< TunnelEx::ConvertString<TunnelEx::String>(error.GetString()).GetCStr()
			<< " (" << error.GetErrorNo() << ")."
			<< std::endl;
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

bool PipeConnection::ReadOverlappedResult() {
	DWORD bytesToRead = 0;
	boost::mutex::scoped_lock lock(m_stateMutex);
	const auto isSuccess = GetOverlappedResult( 
		m_handle,
		&GetOverlaped(),
		&bytesToRead,
		FALSE);
	if (!isSuccess) {
		throw std::exception("Failed to read pipe overlapped result");
	} else if (bytesToRead > 0) {
		return true;
	} else {
		Close(lock);
		return false;
	}
}

void PipeConnection::ReadConnectionState() {
	switch (GetLastError()) { 
		case ERROR_IO_PENDING:
			m_isConnectionState = true;
			break;
		case ERROR_SUCCESS:
		case ERROR_PIPE_CONNECTED:
			m_isConnectionState = false;
			break;
		default:
			{
				TunnelEx::Error error(GetLastError());
				std::cerr
					<< "Failed to read pipe connection state: "
					<<  TunnelEx::ConvertString<TunnelEx::String>(error.GetString()).GetCStr()
					<< " (" << error.GetErrorNo() << ")."
					<< std::endl;
				throw std::exception("An error occurs during the pipe connect operation.");
			}
	}
}

////////////////////////////////////////////////////////////////////////////////

PipeClientConnection::PipeClientConnection(const std::string &path)
		: Base(INVALID_HANDLE_VALUE) {

	AutoHandle handle(
		CreateFileA( 
			path.c_str(),
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_OVERLAPPED,
			NULL));
	if (!handle.IsValid()) {
		TunnelEx::Error error(GetLastError());
		if (error.GetErrorNo() != ERROR_PIPE_BUSY) {
			std::cerr
				<< "Failed to open pipe \"" << path <<"\": "
				<<  TunnelEx::ConvertString<TunnelEx::String>(error.GetString()).GetCStr()
				<< " (" << error.GetErrorNo() << ")." << std::endl;
			throw std::exception("Failed to open pipe");
		}
	}

	ReadConnectionState();
	SetHandle(handle.Release());

}

PipeClientConnection::~PipeClientConnection() {
	//...//
}

void PipeClientConnection::SetAsConnected() {
	DWORD mode = PIPE_READMODE_MESSAGE; 
	const auto result = SetNamedPipeHandleState( 
		GetHandle(),
		&mode,
		NULL,
		NULL);
	if (!result) {
		TunnelEx::Error error(GetLastError());
		std::cerr
			<< "Failed to set pipe mode: "
			<< TunnelEx::ConvertString<TunnelEx::String>(error.GetString()).GetCStr()
			<< " (" << error.GetErrorNo() << ")." << std::endl;
		throw std::exception("Failed to set pipe mode");
	}
	Base::SetAsConnected();
}

// void PipeClientConnection::ReadThreadMain() {
// 
// 	AutoHandle handle;
// 
// 	try {
// 
// 		for ( ; ; ) {
// 
// 			handle.handle = CreateFileA( 
// 				m_path.c_str(),
// 				GENERIC_READ | GENERIC_WRITE,
// 				0,
// 				NULL,
// 				OPEN_EXISTING,
// 				0,
// 				NULL);
// 			if (handle.IsValid()) {
// 				break;
// 			} else {
// 				TunnelEx::Error error(GetLastError());
// 				if (error.GetErrorNo() != ERROR_PIPE_BUSY) {
// 					std::cerr
// 						<< "Failed to open pipe \"" << m_path <<"\": "
// 						<<  TunnelEx::ConvertString<TunnelEx::String>(error.GetString()).GetCStr()
// 						<< " (" << error.GetErrorNo() << ")." << std::endl;
// 					return;
// 				}
// 			}
// 
// 			if (!WaitNamedPipeA(m_path.c_str(), m_waitTimeout)) { 
// 				TunnelEx::Error error(GetLastError());
// 				std::cerr
// 					<< "Failed to wait pipe \"" << m_path <<"\": "
// 					<<  TunnelEx::ConvertString<TunnelEx::String>(error.GetString()).GetCStr()
// 					<< " (" << error.GetErrorNo() << ")." << std::endl;
// 				return;
// 			}
// 
// 		}
// 
// 		DWORD mode = PIPE_READMODE_MESSAGE; 
// 		if (!SetNamedPipeHandleState( 
// 				handle.handle,
// 				&mode,
// 				NULL,
// 				NULL)) {
// 			TunnelEx::Error error(GetLastError());
// 			std::cerr
// 				<< "Failed to set pipe mode for \"" << m_path <<"\": "
// 				<<  TunnelEx::ConvertString<TunnelEx::String>(error.GetString()).GetCStr()
// 				<< " (" << error.GetErrorNo() << ")." << std::endl;
// 			return;
// 		}
// 
// 	} catch (const std::exception &ex) {
// 		std::cerr << "Error in client pipe read thread: " << ex.what() << "." << std::endl;
// 	} catch (...) {
// 		std::cerr << "Unknown error in client pipe read thread." << std::endl;
// 	}
// 
// 	SetHandle(handle.Release());
// 
// }

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
		GetBufferSize(),
		GetBufferSize(),
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
	ReadConnectionState();
	SetHandle(handle.Release());

}

PipeServerConnection::~PipeServerConnection() {
	//...//
}
