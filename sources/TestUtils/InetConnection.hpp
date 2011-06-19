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
		void Send(const T &data) {
			namespace io = boost::asio;
			T *asd = new T(data);
			async_write(
				m_socket,
				io::buffer(*asd),
				boost::bind(
					&TcpConnection::HandleWrite,
					shared_from_this(),
					io::placeholders::error,
					io::placeholders::bytes_transferred));
		}

		Buffer::size_type GetReceivedSize() const;
		Buffer GetReceived() const;

		void ClearReceived(size_t bytesCount = 0);

		bool IsActive() const;

	private:

		explicit TcpConnection(boost::asio::io_service &);

	private:

		void StartRead();
		
		void HandleWrite(const boost::system::error_code &, size_t);
		void HandleRead(const boost::system::error_code &error, size_t);

	private:

		boost::asio::ip::tcp::socket m_socket;
		Buffer m_dataBuffer;
		boost::asio::streambuf m_inStreamBuffer;
		mutable boost::mutex m_mutex;
		bool m_isActive;
	
	};

}

#endif // INCLUDED_FILE__TUNNELEX__InetConnection_hpp__1106050334
