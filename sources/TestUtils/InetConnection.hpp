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

#define TEST_UTIL_TRAFFIC_INET_CONNECTION_LOGGIN 0

namespace TestUtil {

	////////////////////////////////////////////////////////////////////////////////

	struct TcpInetConnectionTrait {
		
		typedef boost::asio::ip::tcp Proto;

		struct AdditionalState {
			boost::asio::streambuf inStreamBuffer;
		};

	};
	
	struct UdpInetConnectionTrait {
	
		typedef boost::asio::ip::udp Proto;

		struct AdditionalState {
			AdditionalState()
					: sendingNow(false) {
				//...//
			}
			bool sendingNow;
			mutable boost::condition_variable dataSentCondition;
			Proto::endpoint actualRemoteEndpoint;
		};

		static size_t GetReceiveBufferSize() {
			return 256;
		}

	};

	////////////////////////////////////////////////////////////////////////////////

	template<typename TraitT>
	class InetConnection
		:  public boost::enable_shared_from_this<InetConnection<TraitT>>,
		private boost::noncopyable {

	public:

		typedef TraitT Trait;
		typedef typename Trait::Proto Proto;
		typedef typename Proto::socket Socket;
		typedef typename Proto::endpoint Endpoint;
		typedef InetConnection<Trait> Self;

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
			assert(!m_isActive);
			m_endpoint = endpoint;
			StartRead();
			BOOST_INTERLOCKED_COMPARE_EXCHANGE(&m_isActive, 1, 0);
		}

		void Close() {
			if (!IsActive()) {
				return;
			}
			GetSocket().get_io_service().post(boost::bind(&Self::HandleClose, shared_from_this()));
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
				m_dataBufferSize = 0;
				UpdateBufferState();
			} else {
				m_dataBufferSize -= bytesCount;
				UpdateBufferState();
				assert(
					std::distance(
						m_dataBufferStart,
						const_cast<const Self *>(this)->m_dataBuffer.end())
					== int(m_dataBufferSize));
			}
		}

		bool IsActive() const {
			return m_isActive != 0;
		}

		bool WaitDataReceiveEvent(
					const boost::system_time &waitUntil,
					Buffer::size_type minSize)
				const {
			boost::mutex::scoped_lock lock(m_mutex);
			for ( ; ; ) {
				if (m_dataBufferSize >= minSize) {
					return true;
				}
				if (!m_dataReceivedCondition.timed_wait(lock, waitUntil)) {
					return m_dataBufferSize >= minSize;
				}
			}
		}

		Socket & GetSocket() {
			return *m_socket;
		}

		Endpoint GetEndpoint() const {
			boost::mutex::scoped_lock lock(m_mutex);
			assert(m_endpoint);
			if (!m_endpoint) {
				throw ConnectionClosed();
			}
			return *m_endpoint;
		}

		template<typename T>
		void HandleWrite(
					const boost::system::error_code &error,
					size_t size,
					T &data) {
			const std::auto_ptr<T> dataHolder(&data);
			assert(data.size() == size || (error && size == 0));
#			if TEST_UTIL_TRAFFIC_INET_CONNECTION_LOGGIN != 0
			{
				std::ostringstream oss;
				oss << this << ".InetConnection.write";
				std::ofstream of(oss.str().c_str(), std::ios::binary |  std::ios::app);
				if (size > 0) {
					of.write(&data[0], size);
				} else {
					of << "[ZERO]";
				}
			}
#			endif
			UseUnused(error, size);
			if (error) {
				std::cerr
					<< "TestUtil::InetConnection::HandleWrite: "
					<< error.message() << " (" << error.value() << ")."
					<< std::endl;
			}
		}
		
		void HandleRead(
					const boost::system::error_code &error,
					size_t size,
					Buffer &buffer,
					Endpoint &actualRemoteEndpoint,
					bool startNextRead) {
			{
				
				const std::auto_ptr<const Buffer> bufferHolder(&buffer);
				boost::mutex::scoped_lock lock(m_mutex);

#				if TEST_UTIL_TRAFFIC_INET_CONNECTION_LOGGIN
				{
					std::ostringstream oss;
					oss << this << ".InetConnection.1.read";
					std::ofstream of(oss.str().c_str(), std::ios::binary|  std::ios::app);
					if (size > 0) {
						of.write(&buffer[0], size);
					} else {
						of << "[ZERO]";
					}
					if (!m_isActive) {
						of << "[CLOSED]";
					}
				}
#				endif

				assert(m_endpoint);
				if (!m_isActive) {
					return;
				}

				assert(actualRemoteEndpoint == *m_endpoint);
				if (actualRemoteEndpoint != *m_endpoint) {
					throw std::logic_error("Wrong endpoint used");
				}

				if (!error || size == buffer.size()) {
					
					assert(size <= buffer.size());
					assert(m_dataBufferSize > 0 || m_dataBufferStart == m_dataBuffer.end());

#					ifdef DEV_VER
						if (error) {
							const std::string message = error.message();
							const char *const messagePch = message.c_str();
							UseUnused(messagePch);
						}
#					endif

					m_dataBuffer.reserve(size);
					std::copy(
						buffer.begin(),
						buffer.begin() + size,
						std::back_inserter(m_dataBuffer));
					UpdateBufferState(size);
				
					if (startNextRead) {
						StartRead();
					}
				
				} else {
					if (	error.value() != WSAECONNRESET
							&& error.value() != WSA_OPERATION_ABORTED) {
						std::cerr
							<< "TestUtil::InetConnection::HandleRead: "
							<< error.message() << " (" << error.value() << ")."
							<< std::endl;
					}
					assert(size == 0);
					OnZeroReceived(lock);

				}
			
			}
			
			m_dataReceivedCondition.notify_all();
		
		}

	private:

		explicit InetConnection(boost::asio::io_service &ioService)
				: m_socket(new Socket(ioService)),
				m_dataBufferStart(m_dataBuffer.end()),
				m_dataBufferSize(0),
				m_isActive(false) {
			assert(
				std::distance(
					m_dataBufferStart,
					const_cast<const Self *>(this)->m_dataBuffer.end())
				== int(m_dataBufferSize));
			UpdateBufferState();
		}

		explicit InetConnection(boost::asio::io_service &ioService, unsigned short port)
				: m_socket(new Socket(ioService, Endpoint(Proto::v4(), port))),
				m_dataBufferStart(m_dataBuffer.end()),
				m_dataBufferSize(0),
				m_isActive(false) {
			assert(
				std::distance(
					m_dataBufferStart,
					const_cast<const Self *>(this)->m_dataBuffer.end())
				== int(m_dataBufferSize));
			UpdateBufferState();
		}

		explicit InetConnection(
					boost::shared_ptr<Socket> socket,
					const Endpoint &endpoint)
				: m_socket(socket),
				m_dataBufferStart(m_dataBuffer.end()),
				m_dataBufferSize(0),
				m_endpoint(endpoint),
				m_isActive(true) {
			assert(
				std::distance(
					m_dataBufferStart,
					const_cast<const Self *>(this)->m_dataBuffer.end())
				== int(m_dataBufferSize));
			UpdateBufferState();
		}

	private:

		void HandleClose() {
			Close(boost::mutex::scoped_lock(m_mutex));
		}

		void Close(const boost::mutex::scoped_lock &) {
			if (!m_isActive) {
				assert(!GetSocket().is_open());
				return;
			}
			GetSocket().close();
			BOOST_INTERLOCKED_COMPARE_EXCHANGE(&m_isActive, 0, 1);
		}

		void OnZeroReceived(const boost::mutex::scoped_lock &lock) {
			Close(lock);
		}

		void HandleRead(const boost::system::error_code &error, size_t size) {
			{
				boost::mutex::scoped_lock lock(m_mutex);
				assert(m_endpoint);
				assert(m_isActive || size == 0);
				if (!error) {
					std::istream is(&m_additionalState.inStreamBuffer);
					is.unsetf(std::ios::skipws);
					assert(m_dataBufferSize > 0 || m_dataBufferStart == m_dataBuffer.end());
					m_dataBuffer.reserve(size);
					std::copy(
						std::istream_iterator<char>(is), 
						std::istream_iterator<char>(), 
						std::back_inserter(m_dataBuffer));
					UpdateBufferState(size);
#					if TEST_UTIL_TRAFFIC_INET_CONNECTION_LOGGIN != 0
					{
						std::ostringstream oss;
						oss << this << ".InetConnection.2.read";
						std::ofstream of(oss.str().c_str(), std::ios::binary |  std::ios::app);
						assert(m_dataBufferSize >= size);
						if (size > 0) {
							of.write(&m_dataBufferStart[0] + m_dataBufferSize - size, size);
						} else {
							of << "[ZERO]";
						}
						if (!m_isActive) {
							of << "[CLOSED]";
						}
					}
#					endif
					StartRead();
				} else {
					if (	error.value() != WSAECONNRESET
							&& error.value() != WSA_OPERATION_ABORTED) {
						std::cerr
							<< "TestUtil::InetConnection::HandleRead: "
							<< error.message() << " (" << error.value() << ")."
							<< std::endl;
					}
					OnZeroReceived(lock);
				}
			}
			m_dataReceivedCondition.notify_all();
		}

		void StartRead() {
			static_assert(false, "Method must have explicit specialization.");
		}

		void UpdateBufferState() {
			UpdateBufferState(0);
		}

		void UpdateBufferState(size_t addSize) {
			assert(!m_dataBuffer.empty() || addSize == 0);
			m_dataBufferSize += addSize;
			m_dataBufferStart
				= m_dataBuffer.begin() + (m_dataBuffer.size() - m_dataBufferSize);
#			ifdef DEV_VER
				m_dataBufferPch = !m_dataBuffer.empty() ? &m_dataBuffer[0] : 0;
				m_dataBufferFullSize = m_dataBuffer.size();
				m_dataBufferStartPch = m_dataBufferStart != m_dataBuffer.end() ? &m_dataBufferStart[0] : 0;
#			endif
			assert(addSize <= m_dataBuffer.size());
			assert(
				std::distance(
					m_dataBufferStart,
					const_cast<const Self *>(this)->m_dataBuffer.end())
				== int(m_dataBufferSize));
		}

	private:

		boost::shared_ptr<Socket> m_socket;
		
		Buffer m_dataBuffer;
		Buffer::const_iterator m_dataBufferStart;
#		ifdef DEV_VER
			const char *m_dataBufferPch;
			size_t m_dataBufferFullSize;
			const char *m_dataBufferStartPch;
#		endif
		Buffer::size_type m_dataBufferSize;

		boost::optional<Endpoint> m_endpoint;

		volatile long m_isActive;

		mutable boost::mutex m_mutex;

		mutable boost::condition_variable m_dataReceivedCondition;

		typename Trait::AdditionalState m_additionalState;

	};

	template<>
	void InetConnection<TcpInetConnectionTrait>::StartRead() {
		namespace io = boost::asio;
		assert(m_endpoint);
		io::async_read_until(
			GetSocket(),
			m_additionalState.inStreamBuffer,
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
		assert(m_endpoint);
		if (!m_isActive) {
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
		std::auto_ptr<Buffer> buffer(new Buffer(Trait::GetReceiveBufferSize()));
		GetSocket().async_receive_from(
			io::buffer(*buffer, buffer->size()),
			m_additionalState.actualRemoteEndpoint,
			boost::bind(
				&Self::HandleRead,
				shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred,
				boost::ref(*buffer),
				boost::ref(m_additionalState.actualRemoteEndpoint),
				true));
		buffer.release();
	}

	template<>
	template<class T>
	void InetConnection<UdpInetConnectionTrait>::Send(std::auto_ptr<T> data) {
		namespace io = boost::asio;
		boost::mutex::scoped_lock lock(m_mutex);
		assert(data->size() > 0);
		if (!m_isActive) {
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
		m_additionalState.sendingNow = true;
	}

	template<>
	void InetConnection<UdpInetConnectionTrait>::OnZeroReceived(
				const boost::mutex::scoped_lock &) {
		//...//
	}

}

#endif // INCLUDED_FILE__TUNNELEX__InetConnection_hpp__1106050334
