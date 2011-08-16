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
		typedef std::vector<Client *> Clients;

		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;

		typedef std::vector<HANDLE> Events;

	private:

		ClientsHandler()
				: m_clientsEvent(CreateEvent(NULL, TRUE, FALSE, NULL)),
				m_stopEvent(CreateEvent(NULL, TRUE, FALSE, NULL)) {
			try {
				m_thread.reset(
					new boost::thread(
						boost::bind(&ClientsHandler::ThreadMain, this)));
				CreateEvents();
			} catch (...) {
				assert(false);
				CloseHandle(m_stopEvent);
				CloseHandle(m_clientsEvent);
				throw;
			}

		}

		~ClientsHandler() {
			assert(m_clients.empty());
			SetEvent(m_stopEvent);
			try {
				m_thread->join();
			} catch (...) {
				assert(false);
			}
			CloseHandle(m_stopEvent);
			CloseHandle(m_clientsEvent);
		}

	public:

		static ClientsHandler & GetInstance() {
			static ClientsHandler instance;
			return instance;
		}

	public:

		void RegisterClient(Client &client) {
			SetEvent(m_clientsEvent);
			Lock lock(m_mutex);
			assert(std::find(m_clients.begin(), m_clients.end(), &client) == m_clients.end());
			m_clients.push_back(&client);
		}

		void UnregisterClient(Client &client) {
			SetEvent(m_clientsEvent);
			Lock lock(m_mutex);
			const auto pos = std::find(m_clients.begin(), m_clients.end(), &client);
			assert(pos != m_clients.end());
			m_clients.erase(pos);
		}

	private:

		void CreateEvents() {
			Events events;
			events.push_back(m_stopEvent);
			events.push_back(m_stopEvent);
			foreach (auto c, m_clients) {
				events.push_back(c->GetEvent());
			}
			events.swap(m_events);
		}

		void ThreadMain() {
			for ( ; ; ) {
				try {
					const auto object = WaitForMultipleObjects(
						m_events.size(),
						&m_events[0],
						FALSE,
						INFINITE);
					if (object == WAIT_OBJECT_0) {
						break;
					} else if (object == (WAIT_OBJECT_0 + 1)) {
						continue;
					} else {
						assert(object < m_clients.size() + WAIT_OBJECT_0 + 2);
						m_clients[object - 2 - m_clients.size()]->Read();
					}
				} catch (const std::exception &ex) {
					std::cerr << "Failed to handle pipe client: " << ex.what() << "." << std::endl;
					assert(false);
				} catch (...) {
					std::cerr << "Failed to handle pipe client: unknown error." << std::endl;
					assert(false);
				}
			}
		}

	private:

		Mutex m_mutex;

		Clients m_clients;
		HANDLE m_clientsEvent;

		HANDLE m_stopEvent;

		Events m_events;

		boost::shared_ptr<boost::thread> m_thread;
		

	};

}

PipeClient::PipeClient(const std::string &path)
		: m_connection(new Connection("\\\\.\\pipe\\" + path)) {
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

void PipeClient::ClearReceived(size_t bytesCount /* = 0*/) {
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
