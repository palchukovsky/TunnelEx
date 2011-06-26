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

	class TcpConnection
		:  public boost::enable_shared_from_this<TcpConnection>,
		private boost::noncopyable {
	
	public:
		
		static boost::shared_ptr<TcpConnection> Create(boost::asio::io_service &);

		boost::asio::ip::tcp::socket & socket();

	public:

		void Start();

		template<class T>
		void Send(std::auto_ptr<T> data) {
			namespace io = boost::asio;
			async_write(
				m_socket,
				io::buffer(*data),
				boost::bind(
					&TcpConnection::HandleWrite<T>,
					shared_from_this(),
					io::placeholders::error,
					boost::ref(*data)));
			data.release();
		}

		Buffer::size_type GetReceivedSize() const;
		void GetReceived(Buffer::size_type maxSize, Buffer &destination) const;

		void ClearReceived(size_t bytesCount = 0);

		bool IsActive() const;

		bool WaitDataReceiveEvent(
				const boost::system_time &waitUntil,
				Buffer::size_type minSize)
			const;

	private:

		explicit TcpConnection(boost::asio::io_service &);

	private:

		void StartRead();
		
		template<typename T>
		void HandleWrite(const boost::system::error_code &, T &data) {
			delete &data;
		}
		
		void HandleRead(const boost::system::error_code &error, size_t);

	private:

		boost::asio::ip::tcp::socket m_socket;
		
		Buffer m_dataBuffer;
		Buffer::const_iterator m_dataBufferStart;
		Buffer::size_type m_dataBufferSize;
		boost::asio::streambuf m_inStreamBuffer;

		mutable boost::mutex m_mutex;

		bool m_isActive;

		mutable boost::condition_variable m_dataReceivedCondition;
	
	};

}

#endif // INCLUDED_FILE__TUNNELEX__InetConnection_hpp__1106050334
