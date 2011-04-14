/**************************************************************************
 *   Created: 2008/11/27 4:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 * -------------------------------------------------------------------
 *       $Id: PipeServer.hpp 959 2010-06-22 11:36:04Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__PipeServer_hpp__0811270444
#define INCLUDED_FILE__TUNNELEX__PipeServer_hpp__0811270444

#include "Server.hpp"

namespace Test {

	//////////////////////////////////////////////////////////////////////////

	class PipeServer : public Test::Server {

	public:
		
		explicit PipeServer(const std::wstring &pipeName);
		virtual ~PipeServer();
	
	public:
	
		virtual bool IsConnected() const;
		virtual std::size_t GetNumberOfAcceptedConnections() const;

	public:

		void CloseConnection(std::size_t connectionIndex);

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
		virtual void ClearReceived(std::size_t connectionIndex);

	private:

		class Implementation;
		boost::shared_ptr<Implementation> m_pimpl;

	};
	
	//////////////////////////////////////////////////////////////////////////
	
	class PipeClient : public Test::Client {

	public:

		explicit PipeClient(const std::wstring &pipeName);

	public:

		virtual void Send(const std::string &);
		virtual void Send(const Buffer &);
		
		void Send(const char *data, size_t size);

		virtual Buffer Receive();

	private:

		class Implementation;
		boost::shared_ptr<Implementation> m_pimpl;

	};
	
	//////////////////////////////////////////////////////////////////////////

}

#endif // INCLUDED_FILE__TUNNELEX__PipeServer_hpp__0811270444
