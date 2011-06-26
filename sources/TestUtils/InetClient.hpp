/**************************************************************************
 *   Created: 2011/06/05 2:11
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__InetClient_hpp__1106050211
#define INCLUDED_FILE__TUNNELEX__InetClient_hpp__1106050211

#include "ClientServer.hpp"

namespace TestUtil {

	//////////////////////////////////////////////////////////////////////////

	class InetClient : public TestUtil::Client {
		//...//
	};
	
	//////////////////////////////////////////////////////////////////////////
	
	class TcpClient : public InetClient {

	public:

		explicit TcpClient(const std::string &host, unsigned short port);
		virtual ~TcpClient();

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

		class Implementation;
		std::auto_ptr<Implementation> m_pimpl;

	};

	//////////////////////////////////////////////////////////////////////////

	class UdpClient : public InetClient {

	public:

		explicit UdpClient(const std::string &host, unsigned short port);
		virtual ~UdpClient();

	public:

		virtual void Send(const std::string &);
		virtual void Send(const Buffer &);

		virtual Buffer::size_type GetReceivedSize() const;
		virtual Buffer GetReceived() const;

		virtual bool IsConnected() const;

		virtual void ClearReceived(size_t bytesCount = 0);

	private:

		class Implementation;
		std::auto_ptr<Implementation> m_pimpl;

	};

	//////////////////////////////////////////////////////////////////////////

}

#endif // INCLUDED_FILE__TUNNELEX__InetClient_hpp__1106050211
