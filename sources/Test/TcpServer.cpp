/**************************************************************************
 *   Created: 2011/05/30 22:43
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "ConnectServer.hpp"
#include "TestUtils/InetServer.hpp"

namespace {

	//////////////////////////////////////////////////////////////////////////
		
	class TcpServer : public testing::ConnectServer {
	public:
		virtual ~TcpServer() {
			//...//
		}
	protected:
		virtual std::auto_ptr<TestUtil::Server> CreateServer(
					const boost::posix_time::time_duration &waitTime)
				const {
			std::auto_ptr<TestUtil::Server> result(
				new TestUtil::TcpServer(testing::tcpServerPort, waitTime));
			return result;
		}
	};

	//////////////////////////////////////////////////////////////////////////

	TEST_F(TcpServer, Any) {
		ASSERT_TRUE(TestAny());
	}

	//////////////////////////////////////////////////////////////////////////

}
