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

	typedef std::vector<boost::shared_ptr<PipeConnection>> Connections;
	typedef boost::mutex ConnectionsMutex;
	typedef ConnectionsMutex::scoped_lock ConnectionsReadLock;
	typedef ConnectionsMutex::scoped_lock ConnectionsWriteLock;

public:

	Implementation(const std::string &path, const boost::posix_time::time_duration &waitTime)
			: m_path("\\\\.\\pipe\\" + path),
			m_stopEvent(CreateEvent(NULL, FALSE, FALSE, NULL)),
			m_waitTime(waitTime) {
		try {
			m_thread.reset(
				new boost::thread(boost::bind(&Implementation::ServerThreadMain, this)));
		} catch (...) {
			CloseHandle(m_stopEvent);
			throw;
		}
	}

	~Implementation() {
		SetEvent(m_stopEvent);
		try {
			m_thread->join();
		} catch (...) {
			assert(false);
		}
		CloseHandle(m_stopEvent);
	}

public:

	bool IsConnected(bool onlyIfActive) const {
		ConnectionsReadLock lock(m_connectionsMutex);
		foreach (const Connections::value_type &c, m_connections) {
			if (!onlyIfActive || c->IsActive()) {
				return true;
			}
		}
		return false;
	}

	bool IsConnected(size_t connectionIndex, bool onlyIfActive) const {
		ConnectionsReadLock lock(m_connectionsMutex);
		return
			connectionIndex < m_connections.size()
			&& (!onlyIfActive || m_connections[connectionIndex]->IsActive());
	}

	unsigned int GetNumberOfAcceptedConnections(bool onlyIfActive) const {
		unsigned int result = 0;
		ConnectionsReadLock lock(m_connectionsMutex);
		foreach (const Connections::value_type &c, m_connections) {
			if (!onlyIfActive || c->IsActive()) {
				++result;
			}
		}
		return result;
	}

	boost::shared_ptr<PipeConnection> GetConnection(size_t connectionIndex) {
		ConnectionsReadLock lock(m_connectionsMutex);
		if (connectionIndex >= m_connections.size()) {
			throw std::logic_error("Could not find connection by index");
		}
		return m_connections[connectionIndex];
	}

	boost::shared_ptr<const PipeConnection> GetConnection(size_t connectionIndex) const {
		return const_cast<Implementation *>(this)->GetConnection(connectionIndex);
	}

private:

	void ServerThreadMain() {

		typedef std::vector<HANDLE> Events;
		Events events;
		events.push_back(m_stopEvent);

		typedef std::map<HANDLE, size_t> EventToConnection;
		EventToConnection eventToConnection;

		boost::shared_ptr<PipeServerConnection> serverConnection;
		try {
			serverConnection.reset(new PipeServerConnection(m_path, m_waitTime));
		} catch (const std::exception &ex) {
			std::cerr << "Failed to start pipe server: " << ex.what() << "." << std::endl;
			return;
		} catch (...) {
			std::cerr << "Failed to start pipe server: unknown error." << std::endl;
			return;
		}
		events.push_back(serverConnection->GetReadEvent());

		for ( ; ; ) {

			const auto object = WaitForMultipleObjects(
				events.size(),
				&events[0],
				FALSE,
				INFINITE);

			if (object == WAIT_OBJECT_0) {
				
				break;
			
			} else if (object >= WAIT_OBJECT_0  + events.size() - 1) {

				assert(object - WAIT_OBJECT_0 ==  events.size() - 1);
			
				try {

					HANDLE evt = *events.rbegin();

					events.push_back(serverConnection->GetWriteEvent());
					size_t index = 0;
					{
						ConnectionsWriteLock lock(m_connectionsMutex);
						index = m_connections.size();
						m_connections.push_back(serverConnection);
					}
					
					assert(
						eventToConnection.find(serverConnection->GetReadEvent())
						== eventToConnection.end());
					eventToConnection.insert(
						std::make_pair(serverConnection->GetReadEvent(), index));
					assert(
						eventToConnection.find(serverConnection->GetWriteEvent())
						== eventToConnection.end());
					eventToConnection.insert(
						std::make_pair(serverConnection->GetWriteEvent(), index));

					serverConnection->HandleEvent(evt);
					
					serverConnection.reset(new PipeServerConnection(m_path, m_waitTime));
					events.push_back(serverConnection->GetReadEvent());
				
				} catch (const std::exception &ex) {
					std::cerr << "Failed to accept pipe connection: " << ex.what() << "." << std::endl;
				} catch (...) {
					std::cerr << "Failed to accept pipe connection: unknown error." << std::endl;
				}

			} else {

				assert(object > WAIT_OBJECT_0);
				assert(object < WAIT_OBJECT_0 + events.size() - 1);

				try {
					HANDLE evt = events[object - WAIT_OBJECT_0];
					assert(eventToConnection.find(evt) != eventToConnection.end());
					GetConnection(eventToConnection.find(evt)->second)->HandleEvent(evt);
				} catch (const std::exception &ex) {
					std::cerr << "Filed to read pipe data: " << ex.what() << "." << std::endl;
				} catch (...) {
					std::cerr << "Filed to read pipe data: unknown error." << std::endl;
				}

			}
			
		}

	}

private:

	const std::string m_path;

	Connections m_connections;
	mutable ConnectionsMutex m_connectionsMutex;

	HANDLE m_stopEvent;
	boost::shared_ptr<boost::thread> m_thread;
	const boost::posix_time::time_duration m_waitTime;

};

//////////////////////////////////////////////////////////////////////////

PipeServer::PipeServer(
			const std::string &path,
			const boost::posix_time::time_duration &waitTime)
		: Server(waitTime),
		m_pimpl(new Implementation(path, GetWaitTime())) {
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
