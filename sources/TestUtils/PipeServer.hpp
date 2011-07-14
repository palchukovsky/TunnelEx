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

		explicit PipeServer(const std::string &path);
		virtual ~PipeServer() throw();

	public:

		/** @throw SendError
		  */
		virtual void Send(std::size_t connectionIndex, const std::string &);
		/** @throw SendError
		  */
		virtual void Send(std::size_t connectionIndex, std::auto_ptr<Buffer>);

		/** @throw ReceiveError
		  */
		virtual Buffer::size_type GetReceivedSize(std::size_t connectionIndex) const;
		/** @throw ReceiveError
		  */
		virtual void GetReceived(
					std::size_t connectionIndex,
					size_t maxSize,
					Buffer &result)
				const;

		/** @throw ReceiveError
		  */
		virtual void ClearReceived(size_t connectionIndex, size_t bytesCount = 0);

	public:

		virtual bool IsConnected(bool onlyIfActive) const;
		virtual bool IsConnected(size_t connectionId, bool onlyIfActive) const;
		virtual unsigned int GetNumberOfAcceptedConnections(bool onlyIfActive) const;
		virtual void CloseConnection(size_t connectionIndex);

	private:

		virtual bool WaitDataReceiveEvent(
				size_t connectionIndex,
				const boost::system_time &waitUntil,
				Buffer::size_type minSize)
			const;

	private:

		class Implementation;
		std::auto_ptr<Implementation> m_pimpl;

	};
	
}

#endif // INCLUDED_FILE__TUNNELEX__PipeServer_hpp__0811270444
