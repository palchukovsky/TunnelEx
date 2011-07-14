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
#include "PipeConnection.hpp"
#include "Core/Error.hpp"
 
using namespace TestUtil;

//////////////////////////////////////////////////////////////////////////

class PipeServer::Implementation : private boost::noncopyable {

private:

	typedef PipeConnection Connection;
	typedef std::vector<boost::shared_ptr<Connection>> Connections;

public:

	Implementation(const std::string &path)
			: m_path("\\\\.\\pipe\\" + path) {
		m_acceptThread.reset(
			new boost::thread(
				boost::bind(&Implementation::AcceptThreadMain, this)));
	}

	~Implementation() {
		try {
			m_acceptThread->join();
		} catch (...) {
			assert(false);
		}
	}

public:

	bool IsConnected(bool onlyIfActive) const {
		boost::mutex::scoped_lock lock(m_connectionsMutex);
		foreach (const Connections::value_type &c, m_connections) {
			if (!onlyIfActive || c->IsActive()) {
				return true;
			}
		}
		return false;
	}

	bool IsConnected(size_t connectionIndex, bool onlyIfActive) const {
		boost::mutex::scoped_lock lock(m_connectionsMutex);
		return
			connectionIndex < m_connections.size()
			&& (!onlyIfActive || m_connections[connectionIndex]->IsActive());
	}

	unsigned int GetNumberOfAcceptedConnections(bool onlyIfActive) const {
		unsigned int result = 0;
		boost::mutex::scoped_lock lock(m_connectionsMutex);
		foreach (const Connections::value_type &c, m_connections) {
			if (!onlyIfActive || c->IsActive()) {
				++result;
			}
		}
		return result;
	}

	boost::shared_ptr<Connection> GetConnection(size_t connectionIndex) {
		boost::mutex::scoped_lock lock(m_connectionsMutex);
		if (connectionIndex >= m_connections.size()) {
			throw std::logic_error("Could not find connection by index");
		}
		return m_connections[connectionIndex];
	}

	boost::shared_ptr<const Connection> GetConnection(size_t connectionIndex) const {
		return const_cast<Implementation *>(this)->GetConnection(connectionIndex);
	}

private:

	void AcceptThreadMain() {

		struct AutoHandle {
			HANDLE handle;
			AutoHandle()
					: handle(INVALID_HANDLE_VALUE) {
				//...//
			}
			~AutoHandle() {
				if (handle != INVALID_HANDLE_VALUE) {
					CloseHandle(handle);
				}
			}
			void Release() throw() {
				handle = INVALID_HANDLE_VALUE;
			}
		} ;
		
		for (size_t i = 1; ; ++i) {

			AutoHandle handleHolder;

			DWORD pipeOpenMode = PIPE_ACCESS_DUPLEX;
			if (i == i) {
				pipeOpenMode |= FILE_FLAG_FIRST_PIPE_INSTANCE;
			}
			handleHolder.handle = CreateNamedPipeA( 
				m_path.c_str(),
				pipeOpenMode,
				PIPE_TYPE_MESSAGE				// message type pipe 
					| PIPE_READMODE_MESSAGE		// message-read mode 
					| PIPE_WAIT,                // blocking mode 
				PIPE_UNLIMITED_INSTANCES,
				Connection::GetBufferSize(),
				Connection::GetBufferSize(),
				0,                        // client time-out 
				NULL);
			if (handleHolder.handle == INVALID_HANDLE_VALUE) {
				TunnelEx::Error error(GetLastError());
				std::cerr
					<< "Failed to create pipe: "
					<<  TunnelEx::ConvertString<TunnelEx::String>(error.GetString()).GetCStr()
					<< " (" << error.GetErrorNo() << ")." << std::endl;
				throw std::exception("Failed to create pipe");
			}

			const BOOL isConnect
				= ConnectNamedPipe(handleHolder.handle, NULL)
				|| GetLastError() == ERROR_PIPE_CONNECTED;
			if (!isConnect) {
				TunnelEx::Error error(GetLastError());
				std::cerr
					<< "Failed to create pipe: "
					<<  TunnelEx::ConvertString<TunnelEx::String>(error.GetString()).GetCStr()
					<< " (" << error.GetErrorNo() << ")." << std::endl;
				break;
			}

			boost::shared_ptr<Connection> connection(
				new Connection(handleHolder.handle));
			handleHolder.Release();

			boost::mutex::scoped_lock lock(m_connectionsMutex);
			m_connections.push_back(connection);
			
		}

	}

private:

	const std::string m_path;
	Connections m_connections;
	boost::shared_ptr<boost::thread> m_acceptThread;
	mutable boost::mutex m_connectionsMutex;

};

//////////////////////////////////////////////////////////////////////////

PipeServer::PipeServer(const std::string &path)
		: m_pimpl(new Implementation(path)) {
	//...//
}

PipeServer::~PipeServer() {
	//...//
}

bool PipeServer::IsConnected(bool onlyIfActive) const {
	return m_pimpl->IsConnected(onlyIfActive);
}

bool PipeServer::IsConnected(size_t connection, bool onlyIfActive) const {
	return m_pimpl->IsConnected(connection, onlyIfActive);
}

unsigned int PipeServer::GetNumberOfAcceptedConnections(bool onlyIfActive) const {
	return m_pimpl->GetNumberOfAcceptedConnections(onlyIfActive);
}

void PipeServer::CloseConnection(size_t connectionIndex) {
	m_pimpl->GetConnection(connectionIndex)->Close();
}

void PipeServer::Send(size_t connectionIndex, const std::string &message)  {
	assert(!message.empty());
	std::auto_ptr<Buffer> buffer(new Buffer(message.begin(), message.end()));
	Send(connectionIndex, buffer);
}

void PipeServer::Send(size_t connectionIndex, std::auto_ptr<Buffer> data) {
	assert(!data->empty());
	m_pimpl->GetConnection(connectionIndex)->Send(data);
}

Buffer::size_type PipeServer::GetReceivedSize(size_t connectionIndex) const {
	return m_pimpl->GetConnection(connectionIndex)->GetReceivedSize();
}

void PipeServer::GetReceived(
			size_t connectionIndex,
			size_t maxSize,
			Buffer &result)
		const {
	m_pimpl->GetConnection(connectionIndex)->GetReceived(maxSize, result);
}

void PipeServer::ClearReceived(size_t connectionIndex, size_t bytesCount) {
	return m_pimpl->GetConnection(connectionIndex)->ClearReceived(bytesCount);
}

bool PipeServer::WaitDataReceiveEvent(
			size_t connectionIndex,
			const boost::system_time &waitUntil,
			Buffer::size_type minSize)
		const {
	return m_pimpl
		->GetConnection(connectionIndex)
		->WaitDataReceiveEvent(waitUntil, minSize);
}
