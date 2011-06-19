/**************************************************************************
 *   Created: 2008/11/27 4:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "PipeServer.hpp"
 
using namespace TestUtil;

//////////////////////////////////////////////////////////////////////////

namespace {

	void Receive(HANDLE handle, Buffer &receivedBuffer) {
		Buffer operationBuffer(255);
		DWORD received = 0;
		if (!ReadFile(handle, &operationBuffer[0], DWORD(operationBuffer.size()), &received, NULL)) {
			throw ReceiveError("Could not read data from pipe: ReadFile returns FALSE.");
		}
		copy(
			operationBuffer.begin(),
			operationBuffer.begin() + received,
			back_inserter(receivedBuffer));
	}

}


//////////////////////////////////////////////////////////////////////////

class PipeServer::Implementation : private boost::noncopyable {

private:

	class Connection : private boost::noncopyable {
	
	public:
	
		explicit Connection(std::wstring pipeName) {
			boost::replace_all(pipeName, L"/", L"\\");
			pipeName = (boost::wformat(L"\\\\.\\pipe\\%1%") % pipeName).str();
			m_overlap.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			if (m_overlap.hEvent == NULL) {
				throw std::exception("Create pipe event failed.");
			}
			const size_t bufferSize = 4096;
			m_handle = CreateNamedPipeW( 
				pipeName.c_str(),
				PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
				PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
				PIPE_UNLIMITED_INSTANCES,
				bufferSize,
				bufferSize,
				0,
				NULL);
			assert(m_handle != INVALID_HANDLE_VALUE);
			if (m_handle == INVALID_HANDLE_VALUE) {
				CloseHandle(m_overlap.hEvent);
				throw std::exception("Could not start pipe server.");
			}
			Init();
		}
	
		~Connection() {
			BOOL operationResult = FlushFileBuffers(m_handle);
			assert(operationResult == TRUE);
			operationResult = DisconnectNamedPipe(m_handle);
			assert(operationResult == TRUE);
			CloseHandle(m_handle);
		}
	
	public:
	
		bool HandleEvent() {
			if (m_isPanding && !GetPendingResult()) {
				return false;
			}
			return true;
		}

		HANDLE GetEvent() {
			return m_overlap.hEvent;
		}

		void Send(const char *data, size_t size) {
			DWORD sent = 0;
			if (!WriteFile(m_handle, data, DWORD(size), &sent, NULL)) {
				throw SendError(
					"Could not send data to pipe: WriteFile returns FALSE.");
			} else if (sent != size) {
				throw SendError(
					"Could not send data to pipe: sent not equal size");
			}
		}

		Buffer GetReceived() {
			Receive(m_handle, m_received);
			return m_received;
		}

		void ClearReceived(size_t bytesCount) {
			if (bytesCount == 0 || bytesCount >= m_received.size()) {
				m_received.clear();
			} else {
				Buffer(m_received.begin() + bytesCount, m_received.end())
					.swap(m_received);
			}
		}

	private:
	
		void Init() {
			// Start an overlapped connection for this pipe instance.
			// Overlapped ConnectNamedPipe should return zero.
			const bool isConnected = ConnectNamedPipe(m_handle, &m_overlap) == 0;
			assert(isConnected);
			if (!isConnected ) {
				throw std::exception("Could not connect to named pipe.");
			}
			switch (GetLastError()) { 
				// The overlapped connection in progress. 
				case ERROR_IO_PENDING: 
					m_isPanding = true;
					break;
				// Client is already connected, so signal an event. 
				case ERROR_PIPE_CONNECTED:
					m_isPanding = false;
					break;
				// If an error occurs during the connect operation... 
				default: 
					throw std::exception("An error occurs during the pipe connect operation.");
			}
		}
	
		bool GetPendingResult() {
			assert(m_isPanding);
			DWORD bytesTransferred;
			const BOOL isSuccess = GetOverlappedResult( 
				m_handle, &m_overlap, &bytesTransferred, FALSE);
			assert(isSuccess == TRUE);
			return isSuccess == TRUE;
		}
	
	private:
	
		HANDLE m_handle;
		OVERLAPPED m_overlap;
		bool m_isPanding;
		Buffer m_received;

	};
	
	typedef std::vector<boost::shared_ptr<Connection> > Connections;
	
	enum ACCEPT_RESULT {
		ACCEPT_ERROR,
		ACCEPT_IO_PENDING,
		ACCEPT_OK
	};

 public:
 
	explicit Implementation(const std::wstring &pipeName)
			: m_pipeName(pipeName),
			m_activeConnection(new Connection(m_pipeName)),
			m_stopEvent(CreateEvent(NULL, FALSE, FALSE, NULL)) {
		m_acceptThread.reset(
			new boost::thread(
				boost::bind(&Implementation::AcceptingThreadMain, this)));
	}
	
	~Implementation() {
		SetEvent(m_stopEvent);
		m_acceptThread->join();
		CloseHandle(m_stopEvent);
	}
	
public:

	bool IsConnected() const {
		return m_connections.size() > 0;
	}

	size_t GetNumberOfAcceptedConnections() const {
		return m_connections.size();
	}

	void CloseConnection(size_t connectionIndex) {
		boost::mutex::scoped_lock lock(m_clientsMutex);
		if (connectionIndex < m_connections.size()) {
			Connections tmpConnnections(m_connections);
			tmpConnnections.erase(tmpConnnections.begin() + connectionIndex);
			tmpConnnections.swap(m_connections);
		}
	}

public:

	void Send(size_t connectionIndex, const std::string &message) {
		boost::mutex::scoped_lock lock(m_clientsMutex);
		if (connectionIndex >= m_connections.size()) {
			throw SendError("Could not send data to pipe: connection diesn't exist.");
		}
		m_connections[connectionIndex]
			->Send(message.c_str(), (message.size() + 1) * sizeof(std::string::value_type));
	}

	void Send(size_t connectionIndex, const Buffer &buffer) {
		boost::mutex::scoped_lock lock(m_clientsMutex);
		if (connectionIndex >= m_connections.size()) {
			throw SendError("Could not send data to pipe: connection diesn't exist.");
		}
		m_connections[connectionIndex]
			->Send(&buffer[0], buffer.size() * sizeof(Buffer::value_type));
	}

	Buffer GetReceived(size_t connectionIndex) const {
		boost::mutex::scoped_lock lock(m_clientsMutex);
		if (connectionIndex >= m_connections.size()) {
			throw ReceiveError("Could not receive data from pipe: connection diesn't exist.");
		}
		return m_connections[connectionIndex]->GetReceived();
	}

	void ClearReceived(size_t connectionIndex, size_t bytesCount) {
		boost::mutex::scoped_lock lock(m_clientsMutex);
		if (connectionIndex >= m_connections.size()) {
			throw ReceiveError("Could not clear data from pipe: connection diesn't exist.");
		}
		return m_connections[connectionIndex]->ClearReceived(bytesCount);
	}

private:

	void AcceptingThreadMain() {

		HANDLE connectionWaitingEvents[2] = {
			m_stopEvent,
			m_activeConnection->GetEvent()
		};

		for (bool stop = false; !stop; ) {
			while (!stop) {
				const DWORD object = WaitForMultipleObjects(2, connectionWaitingEvents, FALSE, INFINITE);
				if (object == WAIT_OBJECT_0) {
					stop = true;
					break;
				} else if (object == (WAIT_OBJECT_0 + 1) && m_activeConnection->HandleEvent()) {
					boost::shared_ptr<Connection> activeConnection = m_activeConnection;
					m_activeConnection.reset();
					boost::mutex::scoped_lock lock(m_clientsMutex);
					m_connections.push_back(activeConnection);
					break;
				} else {
					assert(false);
					stop = true;
				}
			}
			if (!stop) {
				m_activeConnection.reset(new Connection(m_pipeName));
				connectionWaitingEvents[1] = m_activeConnection->GetEvent();
			}
		}

	}

private:

	const std::wstring m_pipeName;
	boost::shared_ptr<Connection> m_activeConnection;
	HANDLE m_serverHandle;
	mutable boost::mutex m_clientsMutex;
	Connections m_connections;
	HANDLE m_stopEvent;
	boost::shared_ptr<boost::thread> m_acceptThread;

};

PipeServer::PipeServer(const std::wstring &pipeName)
		: m_pimpl(new Implementation(pipeName)) {
	//...//
}

PipeServer::~PipeServer() {
	//...//
}

bool PipeServer::IsConnected() const {
	return m_pimpl->IsConnected();
}

size_t PipeServer::GetNumberOfAcceptedConnections() const {
	return m_pimpl->GetNumberOfAcceptedConnections();
}

void PipeServer::CloseConnection(size_t connectionIndex) {
	m_pimpl->CloseConnection(connectionIndex);
}

void PipeServer::Send(size_t connectionIndex, const std::string &message) {
	m_pimpl->Send(connectionIndex, message);
}

void PipeServer::Send(size_t connectionIndex, const Buffer &buffer) {
	m_pimpl->Send(connectionIndex, buffer);
}

Buffer PipeServer::GetReceived(size_t connectionIndex) const {
	return m_pimpl->GetReceived(connectionIndex);
}

void PipeServer::ClearReceived(size_t connectionIndex, size_t bytesCount) {
	m_pimpl->ClearReceived(connectionIndex, bytesCount);
}

//////////////////////////////////////////////////////////////////////////

/* class PipeClient::Implementation : private boost::noncopyable {

public:

	explicit Implementation(std::wstring pipeName)
			: m_handle(INVALID_HANDLE_VALUE) {
		boost::replace_all(pipeName, L"/", L"\\");
		pipeName = (boost::wformat(L"\\\\.\\pipe\\%1%") % pipeName).str();
		for (	size_t attemtsNumb = 0;
				m_handle == INVALID_HANDLE_VALUE && attemtsNumb < 5;
				++attemtsNumb) {
			m_handle = CreateFileW( 
				pipeName.c_str(),   // pipe name 
				GENERIC_READ | GENERIC_WRITE, // read and write access 
				0,              // no sharing 
				NULL,           // default security attributes
				OPEN_EXISTING,  // opens existing pipe 
				0,              // default attributes 
				NULL);          // no template file 
			if (m_handle == INVALID_HANDLE_VALUE) {
				if (GetLastError() != ERROR_PIPE_BUSY || !WaitNamedPipeW(pipeName.c_str(), 300000)) {
					throw std::exception("Could not open pipe.");
				}
			}
		}
		if (m_handle == INVALID_HANDLE_VALUE) {
			throw std::exception("Could not open pipe.");
		}
		DWORD mode = PIPE_READMODE_MESSAGE; 
		if (!SetNamedPipeHandleState(m_handle, &mode, NULL, NULL)) {
			throw std::exception("Could not set pipe handle state.");
		}
	}
	
	~Implementation() {
		CloseHandle(m_handle);
	}

	HANDLE m_handle;

};

PipeClient::PipeClient(const std::wstring &pipeName)
		: m_pimpl(new Implementation(pipeName)) {
	//...//
}

void PipeClient::Send(const std::string &data) {
	Send(data.c_str(), data.size() + 1);
}

void PipeClient::Send(const Buffer &buffer) {
	Send(&buffer[0], buffer.size());
}

void PipeClient::Send(const char *data, size_t size) {
	DWORD sent = 0;
	if (!WriteFile(m_pimpl->m_handle, data, DWORD(size), &sent, NULL)) {
		throw SendError("Could not send data to pipe: WriteFile returns FALSE.");
	} else if (sent != size) {
		throw SendError("Could not send data to pipe: sent not equal size");
	}
}

Buffer PipeClient::Receive() {
	Buffer result;
	::Receive(m_pimpl->m_handle, result);
	return result;
} */

//////////////////////////////////////////////////////////////////////////
