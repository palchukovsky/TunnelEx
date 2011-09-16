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

PipeConnection::PipeConnection(HANDLE handle, const boost::posix_time::time_duration &waitTime)
		: m_handle(handle),
		m_dataBufferStart(m_dataBuffer.end()),
		m_dataBufferSize(0),
		m_isActive(false),
		m_waitTime(waitTime) {

	assert(
		std::distance(
			m_dataBufferStart,
			const_cast<const Self *>(this)->m_dataBuffer.end())
		== int(m_dataBufferSize));
	UpdateBufferState();
	
	ZeroMemory(&m_readOverlaped, sizeof(m_readOverlaped));
	m_readOverlaped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(m_readOverlaped.hEvent != INVALID_HANDLE_VALUE);

	ZeroMemory(&m_writeOverlaped, sizeof(m_writeOverlaped));
	m_writeOverlaped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(m_writeOverlaped.hEvent != INVALID_HANDLE_VALUE);

}

PipeConnection::~PipeConnection() {
	assert(m_sentBuffers.empty());
	try {
		Close();
		assert(m_handle == INVALID_HANDLE_VALUE);
	} catch (...) {
		assert(false);
	}
	verify(CloseHandle(m_writeOverlaped.hEvent));
	verify(CloseHandle(m_readOverlaped.hEvent));
}

size_t PipeConnection::GetBufferSize() {
	return 512;
}

void PipeConnection::Close() {
	Close(boost::mutex::scoped_lock(m_stateMutex));
}

void PipeConnection::Close(const boost::mutex::scoped_lock &) {
	BOOST_INTERLOCKED_EXCHANGE(&m_isActive, 0);
	m_dataSentCondition.notify_all();
	CloseHandles();	
}

void PipeConnection::CloseHandles() {
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
	if (!IsActive()) {
		return;
	} else if (!m_sentBuffers.empty()) {
		m_dataSentCondition.timed_wait(
			stateLock,
			boost::get_system_time() + m_waitTime);
		if (!IsActive()) {
			return;
		} else if (!m_sentBuffers.empty()) {
			std::cerr << "Failed to send data to pipe: send data timeout." << std::endl;
			throw SendError("Failed to send data to pipe: send data timeout");
		}
	}

	DWORD sent = 0;
	assert(data->size() > 0);
	if (!WriteFile(m_handle, &(*data)[0], DWORD(data->size()), &sent, &GetWriteOverlaped())) {
		const TunnelEx::Error error(GetLastError());
		assert(error.IsError());
		if (error.GetErrorNo() != ERROR_IO_PENDING) {
			std::cerr
				<< "Failed to send data to pipe: "
				<<  TunnelEx::ConvertString<TunnelEx::String>(error.GetString()).GetCStr()
				<< " (" << error.GetErrorNo() << ")." << std::endl;
			throw SendError("Could not send data to pipe: WriteFile returns FALSE");
		}
	} else if (sent != data->size()) {
		throw SendError("Could not send data to pipe: sent not equal size");
	}
	m_sentBuffers.push_back(boost::shared_ptr<Buffer>(data));

}

void PipeConnection::HandleEvent(HANDLE evt) {
	if (evt == GetReadOverlaped().hEvent) {
		 HandleRead();
	} else if (evt == GetWriteOverlaped().hEvent) {
		HandleWrite();
	} else {
		assert(!IsActive());
		HandleClose();
	}
}

void PipeConnection::HandleRead() {

	DWORD overlappedResult = 0;

	{

		boost::mutex::scoped_lock lock(m_stateMutex);

		overlappedResult = ReadOverlappedReadResult(lock);
		assert(overlappedResult <= m_receiveBuffer.size());

		if (overlappedResult > 0) {
			ReadReceived(overlappedResult, lock);
		}

		StartRead(lock);

	}

	if (overlappedResult > 0) {
		m_dataReceivedCondition.notify_all();
	}

}

void PipeConnection::ReadReceived(DWORD bytesNumber, const boost::mutex::scoped_lock &) {
	assert(bytesNumber > 0);
	m_dataBuffer.reserve(bytesNumber);
	std::copy(
		m_receiveBuffer.begin(),
		m_receiveBuffer.begin() + bytesNumber,
		std::back_inserter(m_dataBuffer));
#	if TEST_UTIL_TRAFFIC_PIPE_CONNECTION_LOGGIN != 0
	{
		std::ostringstream oss;
		oss << this << ".PipeConnection.read";
		std::ofstream of(oss.str().c_str(), std::ios::binary |  std::ios::app);
		of.write(&m_receiveBuffer[0], bytesNumber);
	}
#	endif
	m_receiveBuffer.clear();
	UpdateBufferState(bytesNumber);
}

void PipeConnection::HandleWrite() {
	boost::mutex::scoped_lock lock(m_stateMutex);
	const DWORD sent = ReadOverlappedWriteResult(lock);
	if (!sent) {
		return;
	}
	assert(!m_sentBuffers.empty());
	auto &sentBuffer = *m_sentBuffers.begin();
	assert(!sentBuffer.buffer->empty());
	assert(sentBuffer.sentBytes + sent <= sentBuffer.buffer->size());
#	if TEST_UTIL_TRAFFIC_PIPE_CONNECTION_LOGGIN != 0
	{
		std::ostringstream oss;
		oss << this << ".PipeConnection.write";
		std::ofstream of(oss.str().c_str(), std::ios::binary |  std::ios::app);
		of.write(&(*sentBuffer.buffer)[sentBuffer.sentBytes], sent);
	}
#	endif
	sentBuffer.sentBytes += sent;
	if (sentBuffer.sentBytes >= sentBuffer.buffer->size()) {
		m_sentBuffers.pop_front();
		m_dataSentCondition.notify_all();
	}
}

void PipeConnection::StartRead(const boost::mutex::scoped_lock &lock) {
	if (!IsActive()) {
		return;
	}
	while (StartReadAndRead(lock));
}

bool PipeConnection::StartReadAndRead(const boost::mutex::scoped_lock &lock) {

	assert(m_isActive);
	assert(m_handle != INVALID_HANDLE_VALUE);
	
	if (!m_receiveBuffer.empty()) {
		return false;
	}

	for ( ; ; ) {

		assert(m_receiveBuffer.empty());
		m_receiveBuffer.resize(GetBufferSize());
		DWORD numberOfBytesRead = 0;
		const auto readStartResult = ReadFile(
			m_handle,
			&m_receiveBuffer[0],
			DWORD(m_receiveBuffer.size()),
			&numberOfBytesRead,
			&GetReadOverlaped());
		assert(readStartResult || TunnelEx::Error(GetLastError()).IsError());

		if (readStartResult) {
			assert(numberOfBytesRead > 0);
			ReadReceived(numberOfBytesRead, lock);
			return true;
		}
		
		const TunnelEx::Error error(GetLastError());
		switch (error.GetErrorNo()) {
			case ERROR_MORE_DATA:
				assert(numberOfBytesRead == 0);
				ReadReceived(m_receiveBuffer.size(), lock);
				break;
			default:
				std::cerr
					<< "Failed to start read from pipe: "
					<< TunnelEx::ConvertString<TunnelEx::String>(error.GetString()).GetCStr()
					<< " (" << error.GetErrorNo() << ")."
					<< std::endl;
				Close(lock);
			case ERROR_IO_PENDING:
				return false;
		}

	}

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

HANDLE PipeConnection::GetReadEvent() {
	return GetReadOverlaped().hEvent;
}

HANDLE PipeConnection::GetWriteEvent() {
	return GetWriteOverlaped().hEvent;
}

DWORD PipeConnection::ReadOverlappedWriteResult(const boost::mutex::scoped_lock &lock) {
	DWORD bytesSent = 0;
	const auto isSuccess = GetOverlappedResult( 
		m_handle,
		&GetWriteOverlaped(),
		&bytesSent,
		FALSE);
	if (!isSuccess) {
		const TunnelEx::Error error(GetLastError());
		std::cerr
			<< "Failed to read pipe overlapped write result: "
			<<  TunnelEx::ConvertString<TunnelEx::String>(error.GetString()).GetCStr()
			<< " (" << error.GetErrorNo() << ")." << std::endl;
		throw std::exception("Failed to read pipe overlapped write result");
	} else if (bytesSent > 0) {
		return bytesSent;
	} else {
		Close(lock);
		return 0;
	}
}

DWORD PipeConnection::ReadOverlappedReadResult(const boost::mutex::scoped_lock &lock) {

	if (m_handle == INVALID_HANDLE_VALUE) {
		assert(!IsActive());
		return 0;
	}

	DWORD bytesToRead = 0;
	const auto isSuccess = GetOverlappedResult( 
		m_handle,
		&GetReadOverlaped(),
		&bytesToRead,
		FALSE);
	assert(isSuccess || TunnelEx::Error(GetLastError()).IsError());
	if (!isSuccess) {
		const TunnelEx::Error error(GetLastError());
		switch (error.GetErrorNo()) {
			case ERROR_MORE_DATA:
				assert(bytesToRead == m_receiveBuffer.size());
				break;
			case ERROR_BROKEN_PIPE:
			case ERROR_PIPE_NOT_CONNECTED:
				Close(lock);
				return bytesToRead;
			default:
				std::cerr
					<< "Failed to read pipe overlapped read result: "
					<<  TunnelEx::ConvertString<TunnelEx::String>(error.GetString()).GetCStr()
					<< " (" << error.GetErrorNo() << ")." << std::endl;
				throw std::exception("Failed to read pipe overlapped read result");
		}
	}
	
	if (bytesToRead > 0) {
		return bytesToRead;
	} else if (!m_isActive) {
		SetAsConnected(lock);
		return 0;
	} else {
		Close(lock);
		return 0;
	}

}

////////////////////////////////////////////////////////////////////////////////

PipeClientConnection::PipeClientConnection(
			const std::string &path,
			const boost::posix_time::time_duration &timeOut)
		: Base(INVALID_HANDLE_VALUE, timeOut) {

	AutoHandle handle;

	for ( ; ; ) {

		handle.handle = CreateFileA( 
			path.c_str(),
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_OVERLAPPED,
			NULL);
		if (handle.IsValid()) {
			break;
		} else {
			TunnelEx::Error error(GetLastError());
			if (error.GetErrorNo() != ERROR_PIPE_BUSY) {
				std::cerr
					<< "Failed to open pipe \"" << path <<"\": "
					<<  TunnelEx::ConvertString<TunnelEx::String>(error.GetString()).GetCStr()
					<< " (" << error.GetErrorNo() << ")." << std::endl;
				throw std::exception("Failed to open pipe");
			}
		}
			
		if (!WaitNamedPipeA(path.c_str(), DWORD(GetWaitTime().total_milliseconds()))) { 
			TunnelEx::Error error(GetLastError());
			std::cerr
				<< "Failed to wait pipe \"" << path <<"\": "
				<<  TunnelEx::ConvertString<TunnelEx::String>(error.GetString()).GetCStr()
				<< " (" << error.GetErrorNo() << ")." << std::endl;
			throw std::exception("Failed to wait pipe");
		}

	}

	DWORD mode = PIPE_READMODE_MESSAGE; 
	const auto result = SetNamedPipeHandleState( 
		handle.handle,
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

	SetHandle(handle.Release());
	SetAsConnected();

}

////////////////////////////////////////////////////////////////////////////////

PipeServerConnection::PipeServerConnection(
			const std::string &path,
			const boost::posix_time::time_duration &timeOut)
		: Base(INVALID_HANDLE_VALUE, timeOut) {

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
		= ConnectNamedPipe(handle.handle, &GetReadOverlaped());
	assert(connectResult == 0);
	UseUnused(connectResult);

	SetHandle(handle.Release());

	switch (GetLastError()) { 
		case ERROR_IO_PENDING:
			break;
		case ERROR_PIPE_CONNECTED:
			SetAsConnected();
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

	m_closeEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	assert(m_closeEvent != INVALID_HANDLE_VALUE);

}

PipeServerConnection::~PipeServerConnection() {
	verify(CloseHandle(m_closeEvent));
}

void PipeServerConnection::CloseHandles() {
	{
		boost::mutex::scoped_lock lock(m_closeMutex);
		verify(SetEvent(m_closeEvent));
		m_closeCondition.wait(lock);
	}
	PipeConnection::CloseHandles();
}

void PipeServerConnection::HandleClose() {
	boost::mutex::scoped_lock(m_closeMutex);
	m_closeCondition.notify_all();
}

HANDLE PipeServerConnection::GetCloseEvent() {
	return m_closeEvent;
}
