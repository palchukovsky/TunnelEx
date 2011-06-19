/**************************************************************************
 *   Created: 2008/01/12 21:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Server_h__0801122134
#define INCLUDED_FILE__TUNNELEX__Server_h__0801122134

#include "ClientServer.hpp"

namespace TestUtil {

	//////////////////////////////////////////////////////////////////////////

	class TcpServer : public TestUtil::Server {

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
		virtual Buffer::size_type GetReceivedSize(std::size_t connectionIndex) const;
		/** @throw ReceiveError
		  */
		virtual Buffer GetReceived(std::size_t connectionIndex) const;

		/** @throw ReceiveError
		  */
		virtual void ClearReceived(size_t connectionIndex, size_t bytesCount = 0);

	public:

		virtual bool IsConnected(bool onlyIfActive) const;
		virtual bool IsConnected(size_t connectionId, bool onlyIfActive) const;
		virtual unsigned int GetNumberOfAcceptedConnections(bool onlyIfActive) const;
		virtual void CloseConnection(size_t connectionIndex);

	private:

		class Implementation;
		std::auto_ptr<Implementation> m_pimpl;

	};

}

#endif // INCLUDED_FILE__TUNNELEX__Server_h__0801122134
