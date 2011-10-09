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

	typedef std::vector<boost::shared_ptr<PipeServerConnection>> Connections;
	
	typedef boost::mutex EventsMutex;
	typedef EventsMutex::scoped_lock EventsLock;

	typedef boost::mutex ConnectionsMutex;
	typedef ConnectionsMutex::scoped_lock ConnectionsReadLock;
	typedef ConnectionsMutex::scoped_lock ConnectionsWriteLock;
	
	typedef std::vector<HANDLE> Events;
	typedef std::map<HANDLE, size_t> EventToConnection;

public:

	Implementation(const std::string &path, const boost::posix_time::time_duration &waitTime)
			: m_path("\\\\.\\pipe\\" + path),
			m_stopEvent(CreateEvent(NULL, TRUE, FALSE, NULL)),
			m_waitTime(waitTime) {
		
		try {

			std::auto_ptr<Events> events(new Events);
			events->push_back(m_stopEvent);

			try {
				m_serverConnection.reset(new PipeServerConnection(m_path, m_waitTime));
			} catch (const std::exception &ex) {
				std::cerr << "Failed to start pipe server: " << ex.what() << "." << std::endl;
				throw;
			} catch (...) {
				std::cerr << "Failed to start pipe server: unknown error." << std::endl;
				throw;
			}
			
			events->push_back(m_serverConnection->GetReadEvent());
			
			std::unique_ptr<Waiter> waiter(
				new Waiter(
					events,
					boost::bind(&Implementation::EventHandler, this, _1, _2),
					m_eventsMutex));
			m_waiters.create_thread(boost::bind(&Waiter::Wait, waiter.get()));
			waiter.release();

		} catch (...) {
			CloseHandle(m_stopEvent);
			throw;
		}
	}

	~Implementation() {
		SetEvent(m_stopEvent);
		try {
			m_waiters.join_all();
		} catch (...) {
			assert(false);
		}
		verify(CloseHandle(m_stopEvent));
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

	boost::shared_ptr<PipeServerConnection> GetConnection(size_t connectionIndex) {
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

	class Waiter : private boost::noncopyable {

	public:

		explicit Waiter(
					std::auto_ptr<Events> events,
					boost::function<bool(Events &, DWORD)> eventHandler,
					EventsMutex &mutex)
				: m_events(events),
				m_eventHandler(eventHandler),
				m_mutex(mutex) {
			assert(!m_events->empty());
		}

	public:

		void Wait() {
			try {
				for ( ; ; ) {
					assert(!m_events->empty());
					assert(m_events->size() <= MAXIMUM_WAIT_OBJECTS);
					const auto object = WaitForMultipleObjects(
						m_events->size(),
						&(*m_events)[0],
						FALSE,
						INFINITE);
					EventsLock lock(m_mutex);
					if (!m_eventHandler(*m_events, object)) {
						break;
					}
				}
			} catch (const std::exception &ex) {
				std::cerr << "Filed to wait pipe: " << ex.what() << "." << std::endl;
			} catch (...) {
				std::cerr << "Filed to wait pipe: unknown error." << std::endl;
			}
			delete this;
		}

	private:

		std::auto_ptr<Events> m_events;
		boost::function<bool(Events &, DWORD)> m_eventHandler;
		EventsMutex &m_mutex;

	};

	bool EventHandler(Events &events, DWORD object) {

		assert(object != WAIT_TIMEOUT);

		if (object == WAIT_FAILED) {
			const TunnelEx::Error error(GetLastError());
			std::cerr
				<< "Server events wait failed: \""
				<< TunnelEx::ConvertString<TunnelEx::String>(error.GetString()).GetCStr()
				<< "\" (" << error.GetErrorNo() << ")."
				<< std::endl;
			throw std::exception("Server events wait failed");
		} else if (object == WAIT_OBJECT_0) {
			return false;
		} else if (object >= WAIT_ABANDONED_0 && object <= WAIT_ABANDONED_0  + events.size() - 1) {
			throw std::exception("Server events wait abandoned");
		}

		assert(object > WAIT_OBJECT_0);
		assert(object < WAIT_OBJECT_0 + events.size());
		HANDLE evt = events[object - WAIT_OBJECT_0];

		if (m_serverConnection->GetReadEvent() == evt) {
		
			try {

				const size_t maximumWaitObjects = MAXIMUM_WAIT_OBJECTS;
				assert(events.size() + 2 <= maximumWaitObjects);

				events.push_back(m_serverConnection->GetWriteEvent());
				events.push_back(m_serverConnection->GetCloseEvent());
				size_t index = 0;
				{
					ConnectionsWriteLock lock(m_connectionsMutex);
					index = m_connections.size();
					m_connections.push_back(m_serverConnection);
				}
					
				assert(
					m_eventToConnection.find(m_serverConnection->GetReadEvent())
					== m_eventToConnection.end());
				m_eventToConnection.insert(
					std::make_pair(m_serverConnection->GetReadEvent(), index));
				assert(
					m_eventToConnection.find(m_serverConnection->GetWriteEvent())
					== m_eventToConnection.end());
				m_eventToConnection.insert(
					std::make_pair(m_serverConnection->GetWriteEvent(), index));
				assert(
					m_eventToConnection.find(m_serverConnection->GetCloseEvent())
					== m_eventToConnection.end());
				m_eventToConnection.insert(
					std::make_pair(m_serverConnection->GetCloseEvent(), index));

				m_serverConnection->HandleEvent(evt);
					
				m_serverConnection.reset(new PipeServerConnection(m_path, m_waitTime));

				if (events.size() + 3 > maximumWaitObjects) {
					std::auto_ptr<Events> newEvents(new Events);
					newEvents->push_back(m_stopEvent);
					newEvents->push_back(m_serverConnection->GetReadEvent());
					std::unique_ptr<Waiter> waiter(
						new Waiter(
							newEvents,
							boost::bind(&Implementation::EventHandler, this, _1, _2),
							m_eventsMutex));
					m_waiters.create_thread(boost::bind(&Waiter::Wait, waiter.get()));
					waiter.release();

				} else {
					events.push_back(m_serverConnection->GetReadEvent());
				}
				
			} catch (const std::exception &ex) {
				std::cerr << "Failed to accept pipe connection: " << ex.what() << "." << std::endl;
			} catch (...) {
				std::cerr << "Failed to accept pipe connection: unknown error." << std::endl;
			}

		} else {

			assert(m_eventToConnection.find(evt) != m_eventToConnection.end());

			auto connection = GetConnection(m_eventToConnection.find(evt)->second);

			bool isActive = true;
			try {
				connection->HandleEvent(evt);
				isActive = connection->IsActive();
			} catch (const std::exception &ex) {
				std::cerr << "Filed to read pipe data: " << ex.what() << "." << std::endl;
			} catch (...) {
				std::cerr << "Filed to read pipe data: unknown error." << std::endl;
			}

			if (!isActive) {
				connection->ReleaseCloseEvent();
				assert(
					m_eventToConnection.find(connection->GetReadEvent())
					!= m_eventToConnection.end());
				m_eventToConnection.erase(connection->GetReadEvent());
				assert(
					m_eventToConnection.find(connection->GetReadEvent())
					== m_eventToConnection.end());
				assert(
					m_eventToConnection.find(connection->GetWriteEvent())
					!= m_eventToConnection.end());
				m_eventToConnection.erase(connection->GetWriteEvent());
				assert(
					m_eventToConnection.find(connection->GetWriteEvent())
					== m_eventToConnection.end());
				assert(
					m_eventToConnection.find(connection->GetCloseEvent())
					!= m_eventToConnection.end());
				m_eventToConnection.erase(connection->GetCloseEvent());
				assert(
					m_eventToConnection.find(connection->GetCloseEvent())
					== m_eventToConnection.end());
				assert(
					std::find(events.begin(), events.end(), connection->GetReadEvent())
					!= events.end());
				events.erase(std::find(events.begin(), events.end(), connection->GetReadEvent()));
				assert(
					std::find(events.begin(), events.end(), connection->GetWriteEvent())
					!= events.end());
				events.erase(std::find(events.begin(), events.end(), connection->GetWriteEvent()));
				assert(
					std::find(events.begin(), events.end(), connection->GetCloseEvent())
					!= events.end());
				events.erase(std::find(events.begin(), events.end(), connection->GetCloseEvent()));
			}

		}

		return events.size() > 1;

	}

private:

	const std::string m_path;

	Connections m_connections;
	mutable ConnectionsMutex m_connectionsMutex;

	HANDLE m_stopEvent;
	const boost::posix_time::time_duration m_waitTime;

	EventToConnection m_eventToConnection;

	boost::shared_ptr<PipeServerConnection> m_serverConnection;

	EventsMutex m_eventsMutex;

	boost::thread_group m_waiters;

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
