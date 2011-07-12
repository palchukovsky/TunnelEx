/**************************************************************************
 *   Created: 2008/11/27 4:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__PipeServer_hpp__0811270444
#define INCLUDED_FILE__TUNNELEX__PipeServer_hpp__0811270444

#include "ClientServer.hpp"

namespace TestUtil {

	//////////////////////////////////////////////////////////////////////////

	class PipeServer : public TestUtil::Server {

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
		virtual Buffer GetReceived(size_t connectionIndex) const;
		virtual void ClearReceived(size_t connectionIndex, size_t bytesCount);

	private:

		class Implementation;
		boost::shared_ptr<Implementation> m_pimpl;

	};
	
}

#endif // INCLUDED_FILE__TUNNELEX__PipeServer_hpp__0811270444
