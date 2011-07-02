/**************************************************************************
 *   Created: 2011/06/05 3:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__InetConnection_hpp__1106050334
#define INCLUDED_FILE__TUNNELEX__InetConnection_hpp__1106050334

#include "ClientServer.hpp"

namespace TestUtil {

	////////////////////////////////////////////////////////////////////////////////

	struct TcpInetConnectionTrait {
		typedef boost::asio::ip::tcp Proto;
	};
	struct UdpInetConnectionTrait {
		typedef boost::asio::ip::udp Proto;
	};

	////////////////////////////////////////////////////////////////////////////////

	template<typename ConnectionTraitT>
	class InetConnection
		:  public boost::enable_shared_from_this<InetConnection<ConnectionTraitT>>,
		private boost::noncopyable {

	public:

		typedef ConnectionTraitT ConnectionTrait;
		typedef typename ConnectionTrait::Proto Proto;
		typedef typename Proto::socket Socket;
		typedef typename Proto::endpoint Endpoint;
		typedef InetConnection<ConnectionTrait> Self;

	public:

		~InetConnection() {
			assert(!m_endpoint);
		}

	public:
		
		static boost::shared_ptr<Self> Create(
					boost::asio::io_service &ioService) {
			return boost::shared_ptr<Self>(new Self(ioService));
		}

		static boost::shared_ptr<Self> Create(
					boost::asio::io_service &ioService,
					unsigned short port) {
			return boost::shared_ptr<Self>(new Self(ioService, port));
		}

		static boost::shared_ptr<Self> Create(
					InetConnection &mainConnection,
					const Endpoint &endpoint) {
			return boost::shared_ptr<Self>(new Self(mainConnection.m_socket, endpoint));
		}

	public:

		void Start(const Endpoint &endpoint) {
			assert(!m_endpoint);
			m_endpoint = endpoint;
			StartRead();
		}

		void Stop() {
			if (!IsActive()) {
				return;
			}
			GetSocket().get_io_service().post(boost::bind(&Self::Close, shared_from_this()));
		}

		template<class T>
		void Send(std::auto_ptr<T> data) {
			static_assert(false, "Method must have explicit specialization.");
		}

		Buffer::size_type GetReceivedSize() const {
			Buffer::size_type result;
			{
				boost::mutex::scoped_lock lock(m_mutex);
				result = m_dataBufferSize;
			}
			return result;
		}

		void GetReceived(Buffer::size_type maxSize, Buffer &destination) const {
			Buffer destinationTmp;
			{
				boost::mutex::scoped_lock lock(m_mutex);
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

		void ClearReceived(size_t bytesCount = 0) {
			boost::mutex::scoped_lock lock(m_mutex);
			assert(bytesCount <= m_dataBufferSize);
			assert(
				int(bytesCount)
				<= std::distance(
					m_dataBufferStart,
					const_cast<const Self *>(this)->m_dataBuffer.end()));
			if (bytesCount == 0) {
				m_dataBufferStart = m_dataBuffer.end();
				m_dataBufferSize = 0;
			} else {
				std::advance(m_dataBufferStart, bytesCount);
				m_dataBufferSize -= bytesCount;
				assert(
					std::distance(
						m_dataBufferStart,
						const_cast<const Self *>(this)->m_dataBuffer.end())
					== int(m_dataBufferSize));
			}
		}

		bool IsActive() const {
			boost::mutex::scoped_lock lock(m_mutex);
			return m_endpoint ? true : false;
		}

		bool WaitDataReceiveEvent(
					const boost::system_time &waitUntil,
					Buffer::size_type minSize)
				const {
			for ( ; ; ) {
				boost::mutex::scoped_lock lock(m_mutex);
				if (m_dataBufferSize >= minSize) {
					return true;
				}
				if (!m_dataReceivedCondition.timed_wait(lock, waitUntil)) {
					return false;
				}
			}
		}

		Socket & GetSocket() {
			return *m_socket;
		}

		const Endpoint & GetEndpoint() const {
			return *m_endpoint;
		}

		template<typename T>
		void HandleWrite(
					const boost::system::error_code &error,
					size_t size,
					T &data)
				const {
			assert(!error);
			assert(data.size() == size);
			UseUnused(error, size);
			delete &data;
		}
		
		void HandleRead(
					const boost::system::error_code &error,
					size_t size,
					Buffer &buffer) {
			{
				const std::auto_ptr<const Buffer> bufferHolder(&buffer);
				boost::mutex::scoped_lock lock(m_mutex);
				assert(m_endpoint);
				if (!error) {
					assert(size <= buffer.size());
					assert(m_dataBufferSize > 0 || m_dataBufferStart == m_dataBuffer.end());
					m_dataBuffer.reserve(size);
					std::copy(
						buffer.begin(),
						buffer.end(),
						std::back_inserter(m_dataBuffer));
					UpdateBufferState(size);
				} else {
					Close();
				}
			}
			m_dataReceivedCondition.notify_all();
		}

	private:

		explicit InetConnection(boost::asio::io_service &ioService)
				: m_socket(new Socket(ioService)),
				m_dataBufferStart(m_dataBuffer.end()),
				m_dataBufferSize(0) {
			assert(
				std::distance(
					m_dataBufferStart,
					const_cast<const Self *>(this)->m_dataBuffer.end())
				== int(m_dataBufferSize));
		}

		explicit InetConnection(boost::asio::io_service &ioService, unsigned short port)
				: m_socket(new Socket(ioService, Endpoint(Proto::v4(), port))),
				m_dataBufferStart(m_dataBuffer.end()),
				m_dataBufferSize(0) {
			assert(
				std::distance(
					m_dataBufferStart,
					const_cast<const Self *>(this)->m_dataBuffer.end())
				== int(m_dataBufferSize));
		}

		explicit InetConnection(
					boost::shared_ptr<Socket> socket,
					const Endpoint &endpoint)
				: m_socket(socket),
				m_dataBufferStart(m_dataBuffer.end()),
				m_dataBufferSize(0),
				m_endpoint(endpoint) {
			assert(
				std::distance(
					m_dataBufferStart,
					const_cast<const Self *>(this)->m_dataBuffer.end())
				== int(m_dataBufferSize));
		}

	private:

		void Close() {
			if (!m_endpoint) {
				assert(!GetSocket().is_open());
				return;
			}
			assert(GetSocket().is_open());
			GetSocket().close();
			m_endpoint.reset();
		}

		void HandleRead(const boost::system::error_code &error, size_t size) {
			{
				boost::mutex::scoped_lock lock(m_mutex);
				assert(m_endpoint || size == 0);
				if (!error) {
					std::istream is(&m_inStreamBuffer);
					is.unsetf(std::ios::skipws);
					assert(m_dataBufferSize > 0 || m_dataBufferStart == m_dataBuffer.end());
					m_dataBuffer.reserve(size);
					std::copy(
						std::istream_iterator<char>(is), 
						std::istream_iterator<char>(), 
						std::back_inserter(m_dataBuffer));
					UpdateBufferState(size);
				} else {
					Close();
				}
			}
			m_dataReceivedCondition.notify_all();
		}

		void StartRead() {
			static_assert(false, "Method must have explicit specialization.");
		}

		void UpdateBufferState(size_t addSize) {
			assert(!m_dataBuffer.empty());
			m_dataBufferSize += addSize;
			m_dataBufferStart
				= m_dataBuffer.begin() + (m_dataBuffer.size() - m_dataBufferSize);
			assert(addSize <= m_dataBuffer.size());
			assert(
				std::distance(
					m_dataBufferStart,
					const_cast<const Self *>(this)->m_dataBuffer.end())
				== int(m_dataBufferSize));
			StartRead();
		}

	private:

		boost::shared_ptr<Socket> m_socket;
		
		Buffer m_dataBuffer;
		Buffer::const_iterator m_dataBufferStart;
		Buffer::size_type m_dataBufferSize;
		boost::asio::streambuf m_inStreamBuffer;

		boost::optional<Endpoint> m_endpoint;

		mutable boost::mutex m_mutex;

		mutable boost::condition_variable m_dataReceivedCondition;
	
	};

	template<>
	void InetConnection<TcpInetConnectionTrait>::StartRead() {
		namespace io = boost::asio;
		assert(m_endpoint);
		io::async_read_until(
			GetSocket(),
			m_inStreamBuffer,
			boost::regex(".+"),
			boost::bind(
				&Self::HandleRead,
				shared_from_this(),
				io::placeholders::error,
				io::placeholders::bytes_transferred));
	}

	template<>
	template<class T>
	void InetConnection<TcpInetConnectionTrait>::Send(std::auto_ptr<T> data) {
		namespace io = boost::asio;
		boost::mutex::scoped_lock lock(m_mutex);
		if (!m_endpoint) {
			throw TestUtil::ConnectionClosed();
		}
		async_write(
			GetSocket(),
			io::buffer(*data),
			boost::bind(
				&Self::HandleWrite<T>,
				shared_from_this(),
				io::placeholders::error,
				io::placeholders::bytes_transferred,
				boost::ref(*data)));
		data.release();
	}

	template<>
	void InetConnection<UdpInetConnectionTrait>::StartRead() {
		namespace io = boost::asio;
		assert(m_endpoint);
		std::auto_ptr<Buffer> buffer(new Buffer(128));
		GetSocket().async_receive(
			io::buffer(*buffer, buffer->size()),
			boost::bind(
				&Self::HandleRead,
				shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred,
				boost::ref(*buffer)));
		buffer.release();
	}

	template<>
	template<class T>
	void InetConnection<UdpInetConnectionTrait>::Send(std::auto_ptr<T> data) {
		namespace io = boost::asio;
		boost::mutex::scoped_lock lock(m_mutex);
		if (!m_endpoint) {
			throw TestUtil::ConnectionClosed();
		}
		GetSocket().async_send_to(
			io::buffer(*data, data->size()),
			*m_endpoint,
			boost::bind(
				&Self::HandleWrite<T>,
				shared_from_this(),
				io::placeholders::error,
				io::placeholders::bytes_transferred,
				boost::ref(*data)));
		data.release();
	}


}

#endif // INCLUDED_FILE__TUNNELEX__InetConnection_hpp__1106050334
