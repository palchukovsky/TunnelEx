/**************************************************************************
 *   Created: 2011/05/30 22:43
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "TestUtils/InetServer.hpp"

namespace pt = boost::posix_time;

namespace {

	class TcpServer : public testing::Test {
	
	public:
	
		virtual ~TcpServer() {
			//...//
		}
	
	protected:
		
		static void SetUpTestCase() {
			m_server.reset(new TestUtil::TcpServer(testing::tcpServerPort));
		}
		
		static void TearDownTestCase() {
			m_server.reset();
		}
		
		virtual void SetUp() {
			//...//
		}
		
		virtual void TearDown() {
			//...//
		}
	
		static std::auto_ptr<TestUtil::TcpServer> TcpServer::m_server;

	};

	std::auto_ptr<TestUtil::TcpServer> TcpServer::m_server;

	////////////////////////////////////////////////////////////////////////////////


	TEST_F(TcpServer, DataExchange) {

		ASSERT_TRUE(m_server->WaitConnect(pt::time_duration(0, 1, 0), 1));

	}

}
