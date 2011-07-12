/**************************************************************************
 *   Created: 2011/07/12 17:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__PipeClient_hpp__1107121745
#define INCLUDED_FILE__TUNNELEX__PipeClient_hpp__1107121745

#include "ClientServer.hpp"

namespace TestUtil {

	class PipeConnection;

	class PipeClient : public TestUtil::Client {

	public:

		typedef PipeClient Self;

	private:

		typedef PipeConnection Connection;

	public:
	
		explicit PipeClient(const std::wstring &path);
		virtual ~PipeClient();

	public:

		virtual void Send(const std::string &);
		virtual void Send(std::auto_ptr<Buffer>);
		
		virtual Buffer::size_type GetReceivedSize() const;
		virtual void GetReceived(
					Buffer::size_type maxSize,
					Buffer &result)
				const;

		virtual bool IsConnected() const;

		virtual void ClearReceived(size_t bytesCount = 0);

		virtual void Disconnect();

	protected:

		virtual bool WaitDataReceiveEvent(
					const boost::system_time &waitUntil,
					Buffer::size_type minSize)
				const;

	private:

		Connection & GetConnection();
		const Connection & GetConnection() const;

		void ServiceThreadMain();

	private:

		std::auto_ptr<Connection> m_connection;
		boost::shared_ptr<boost::thread> m_thread;

	};


}

#endif
