/**************************************************************************
 *   Created: 2008/01/12 21:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2009 Eugene V. Palchukovsky
 * -------------------------------------------------------------------
 *       $Id: InetServer.hpp 959 2010-06-22 11:36:04Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Server_h__0801122134
#define INCLUDED_FILE__TUNNELEX__Server_h__0801122134

#include "Server.hpp"

namespace Test {

	//////////////////////////////////////////////////////////////////////////

	class TcpServer : public Test::Server {

	public:

		explicit TcpServer(unsigned short port);
		virtual ~TcpServer() throw();

	public:

		/** @throw SendError
		  */
		virtual void Send(std::size_t connectionIndex, const std::string &);
		/** @throw SendError
		  */
		virtual void Send(std::size_t connectionIndex, const Buffer &);

		/** @throw ReceiveError
		  */
		virtual Buffer GetReceived(std::size_t connectionIndex) const;
		/** @throw ReceiveError
		  */
		virtual void ClearReceived(std::size_t connectionIndex);

	public:

		bool IsConnected() const;
		unsigned int GetNumberOfAcceptedConnections() const;
		void CloseConnection(size_t connectionIndex);

	private:

		class Implementation;
		std::auto_ptr<Implementation> m_pimpl;

	};

	//////////////////////////////////////////////////////////////////////////

	class InetClient : public Test::Client {
		//...//
	};
	
	//////////////////////////////////////////////////////////////////////////
	
	class TcpClient : public InetClient {

	public:

		explicit TcpClient(unsigned short port);
		virtual ~TcpClient();

	public:

		virtual void Send(const std::string &);
		virtual void Send(const Buffer &);

		virtual Buffer Receive();

	private:

		class Implementation;
		std::auto_ptr<Implementation> m_pimpl;

	};

	//////////////////////////////////////////////////////////////////////////

	class UdpClient : public InetClient {

	public:

		explicit UdpClient(unsigned short port);
		virtual ~UdpClient();

	public:

		virtual void Send(const std::string &);
		virtual void Send(const Buffer &);

		virtual Buffer Receive();

	private:

		class Implementation;
		std::auto_ptr<Implementation> m_pimpl;

	};

	//////////////////////////////////////////////////////////////////////////

}

#endif // INCLUDED_FILE__TUNNELEX__Server_h__0801122134