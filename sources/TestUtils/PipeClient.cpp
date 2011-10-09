/**************************************************************************
 *   Created: 2011/07/12 17:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "PipeClient.hpp"
#include "PipeConnection.hpp"
#include "Core/Error.hpp"

using namespace TestUtil;

namespace {

	template<typename ClientT>
	class ClientsHandler : private boost::noncopyable {

	private:

		typedef ClientT Client;
		typedef std::map<HANDLE, Client *> Clients;

		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;

		typedef std::vector<HANDLE> Events;
		typedef std::set<HANDLE> RemovedEvents;

	private:

		class Waiter;

	private:

		ClientsHandler()
				: m_stopEvent(CreateEvent(NULL, TRUE, FALSE, NULL)),
				m_lastWaiter(0) {
			
			assert(m_stopEvent != INVALID_HANDLE_VALUE);
			assert(m_stopEvent != 0);

			try {
				m_lastWaiter = new Waiter(
					m_stopEvent,
					boost::bind(&ClientsHandler::EventHandler, this, _1));
				m_threads.create_thread(boost::bind(&Waiter::Wait, m_lastWaiter));
				m_lastWaiter->Start(Events());
			} catch (...) {
				assert(false);
				verify(CloseHandle(m_stopEvent));
				if (m_lastWaiter) {
					delete m_lastWaiter;
				}
				throw;
			}

		}

		~ClientsHandler() {
			assert(m_clients.empty());
			assert(m_removedEvents.empty());
			verify(SetEvent(m_stopEvent));
			try {
				m_threads.join_all();
			} catch (...) {
				assert(false);
			}
			verify(CloseHandle(m_stopEvent));
		}

	public:

		static ClientsHandler & GetInstance() {
			static ClientsHandler instance;
			return instance;
		}

	public:

		void RegisterClient(Client &client) {

			assert(m_lastWaiter != 0);
			auto waiterLock = m_lastWaiter->Pause();
			std::unique_ptr<Events> events;
			
			Lock lock(m_mutex);
			auto clients(m_clients);
			
			if (m_lastWaiter->GetEvents(*waiterLock).size() + 2 >= MAXIMUM_WAIT_OBJECTS) {
				m_lastWaiter->Release();
				m_lastWaiter->Resume(waiterLock);
				std::unique_ptr<Waiter> waiter(
					new Waiter(
						m_stopEvent,
						boost::bind(&ClientsHandler::EventHandler, this, _1)));
				m_threads.create_thread(boost::bind(&Waiter::Wait, waiter.get()));
				m_lastWaiter = waiter.release();
				events.reset(new Events);
			} else {
				events.reset(new Events(m_lastWaiter->GetEvents(*waiterLock)));
			}

			{
				assert(clients.find(client.GetReadEvent()) == clients.end());
				clients.insert(std::make_pair(client.GetReadEvent(), &client));
				assert(std::find(events->begin(), events->end(), client.GetReadEvent()) == events->end());
				events->push_back(client.GetReadEvent());
			}

			{
				assert(clients.find(client.GetWriteEvent()) == clients.end());
				clients.insert(std::make_pair(client.GetWriteEvent(), &client));
				assert(std::find(events->begin(), events->end(), client.GetWriteEvent()) == events->end());
				events->push_back(client.GetWriteEvent());
			}

			clients.swap(m_clients);

			if (waiterLock.get()) {
				m_lastWaiter->GetEvents(*waiterLock).swap(*events);
				m_lastWaiter->Resume(waiterLock);
			} else {
				m_lastWaiter->Start(*events);
			}

		}

		void UnregisterClient(Client &client) {

			Lock lock(m_mutex);
			auto clients(m_clients);

			assert(clients.find(client.GetReadEvent()) != clients.end());
			clients.erase(client.GetReadEvent());
			assert(clients.find(client.GetReadEvent()) == clients.end());
			assert(clients.find(client.GetWriteEvent()) != clients.end());
			clients.erase(client.GetWriteEvent());
			assert(clients.find(client.GetWriteEvent()) == clients.end());

			RemovedEvents removedEvents(m_removedEvents);
			assert(removedEvents.find(client.GetReadEvent()) == removedEvents.end());
			removedEvents.insert(client.GetReadEvent());
			assert(removedEvents.find(client.GetWriteEvent()) == removedEvents.end());
			removedEvents.insert(client.GetWriteEvent());

			clients.swap(m_clients);
			removedEvents.swap(m_removedEvents);

			verify(SetEvent(client.GetReadEvent()));
			verify(SetEvent(client.GetWriteEvent()));

			do {
				m_removeEventCondtion.wait(lock);
			} while (
				m_removedEvents.find(client.GetReadEvent()) != m_removedEvents.end()
				|| m_removedEvents.find(client.GetWriteEvent()) != m_removedEvents.end());

		}

	private:

		class Waiter : private boost::noncopyable {

		public:

			explicit Waiter(
						HANDLE stopEvent,
						boost::function<bool(HANDLE)> eventHandler)
					: m_isReleased(0),
					m_pauseEvent(CreateEvent(NULL, TRUE, FALSE, NULL)),
					m_stopEvent(stopEvent),
					m_eventHandler(eventHandler) {
				try {
					m_pauseMutex.lock();
				} catch (...) {
					verify(CloseHandle(m_pauseEvent));
					throw;
				}
			}

			~Waiter() {
				verify(CloseHandle(m_pauseEvent));
			}

		public:

			void Wait() {
				
				{
					Lock lock(m_pauseMutex);
				}
				
				try {
				
					for ( ; ; ) {
					
						assert(!m_events.empty());
						assert(m_events.size() <= MAXIMUM_WAIT_OBJECTS);
						
						Lock pauseLock(m_pauseMutex);

						const auto object = WaitForMultipleObjects(
							m_events.size(),
							&m_events[0],
							FALSE,
							INFINITE);

						if (object == WAIT_FAILED) {
							const TunnelEx::Error error(GetLastError());
							std::cerr
								<< "Client events wait failed: \""
								<< TunnelEx::ConvertString<TunnelEx::String>(error.GetString()).GetCStr()
								<< "\" (" << error.GetErrorNo() << ")."
								<< std::endl;
							break;
						} else if (
								object >= WAIT_ABANDONED_0
								&& object <= WAIT_ABANDONED_0  + m_events.size() - 1) {
							std::cerr << "Client events wait abandoned." << std::endl;
							break;
						}

						const auto signaledEvent = m_events[object - WAIT_OBJECT_0];
						if (signaledEvent == m_pauseEvent) {
							continue;
						} else if (signaledEvent == m_stopEvent) {
							break;
						} else if (!m_eventHandler(signaledEvent)) {
							const auto eventToDel
								= std::find(m_events.begin(), m_events.end(), signaledEvent);
							assert(eventToDel != m_events.end());
							m_events.erase(eventToDel);
							assert(m_events.size() >= 2);
							if (m_isReleased && m_events.size() <= 2) {
								break;
							}
						}
					
					}
				
				} catch (const std::exception &ex) {
					std::cerr << "Filed to wait pipe: " << ex.what() << "." << std::endl;
				} catch (...) {
					std::cerr << "Filed to wait pipe: unknown error." << std::endl;
				}
				
				delete this;
			
			}

			std::auto_ptr<Lock> Pause() {
				verify(SetEvent(m_pauseEvent));
				return std::auto_ptr<Lock>(new Lock(m_pauseMutex));
			}

			void Start(const Events &events) {
				Events newEvents;
				newEvents.push_back(m_pauseEvent);
				newEvents.push_back(m_stopEvent);
				std::copy(events.begin(), events.end(), std::back_inserter(newEvents));
				newEvents.swap(m_events);
				m_pauseMutex.unlock();
			}

			void Resume(std::auto_ptr<Lock>) throw() {
				verify(ResetEvent(m_pauseEvent));
			}

			void Release() throw() {
				BOOST_INTERLOCKED_COMPARE_EXCHANGE(&m_isReleased, 1, 0);
			}

			const Events & GetEvents(const Lock &pauseEvent) const throw() {
				return const_cast<Waiter *>(this)->GetEvents(pauseEvent);
			}

			Events & GetEvents(const Lock &/*pauseEvent*/) throw() {
				return m_events;
			}

		private:

			volatile long m_isReleased;
			HANDLE m_pauseEvent;
			HANDLE m_stopEvent;
			Events m_events;
			boost::function<bool(HANDLE)> m_eventHandler;
			Mutex m_pauseMutex;

		};

		bool EventHandler(HANDLE signaledEvent) {
			Lock lock(m_mutex);
			const Clients::const_iterator client = m_clients.find(signaledEvent);
			if (client == m_clients.end()) {
				assert(m_removedEvents.find(signaledEvent) != m_removedEvents.end());
				m_removedEvents.erase(signaledEvent);
				m_removeEventCondtion.notify_all();
				return false;
			}
			client->second->HandleEvent(signaledEvent);
			return true;
		}

	private:

		Mutex m_mutex;
		Clients m_clients;
		HANDLE m_stopEvent;
		boost::thread_group m_threads;
		Waiter *m_lastWaiter;

		boost::condition_variable m_removeEventCondtion;
		RemovedEvents m_removedEvents;

	};

}

PipeClient::PipeClient(
			const std::string &path,
			const boost::posix_time::time_duration &waitTime)
		: Client(waitTime),
		m_connection(new Connection("\\\\.\\pipe\\" + path, GetWaitTime())) {
	ClientsHandler<Connection>::GetInstance().RegisterClient(*m_connection);
}

PipeClient::~PipeClient() {
	try {
		ClientsHandler<Connection>::GetInstance().UnregisterClient(*m_connection);
	} catch (...) {
		assert(false);
	}
}

void PipeClient::Send(const std::string &message) {
	std::auto_ptr<Buffer> buffer(
		new Buffer(message.begin(), message.end()));
	Send(buffer);
}

void PipeClient::Send(std::auto_ptr<Buffer> buffer) {
	GetConnection().Send(buffer);
}

Buffer::size_type PipeClient::GetReceivedSize() const {
	return GetConnection().GetReceivedSize();
}
		
void PipeClient::GetReceived(
			Buffer::size_type maxSize,
			Buffer &result)
		const {
	GetConnection().GetReceived(maxSize, result);
}

bool PipeClient::IsConnected() const {
	return m_connection.get() && m_connection->IsActive();
}

void PipeClient::ClearReceived(size_t bytesCount /*= 0*/) {
	GetConnection().ClearReceived(bytesCount);
}

void PipeClient::Disconnect() {
	m_connection->Close();
}

bool PipeClient::WaitDataReceiveEvent(
			const boost::system_time &waitUntil,
			Buffer::size_type minSize)
		const {
	return GetConnection().WaitDataReceiveEvent(waitUntil, minSize);
}

PipeClient::Connection & PipeClient::GetConnection() {
	if (!m_connection.get()) {
		throw ConnectionClosed();
	}
	return *m_connection;
}
const PipeClient::Connection & PipeClient::GetConnection() const {
	return const_cast<Self *>(this)->GetConnection();
}
