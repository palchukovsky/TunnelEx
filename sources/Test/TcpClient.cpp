/**************************************************************************
 *   Created: 2011/05/30 23:13
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "TestUtils/InetServer.hpp"

namespace {

	class TcpClient : public testing::Test {

	public:

		virtual ~TcpClient() {
			//...//
		}
	
	protected:

		static void SetUpTestCase() {
			//...//
		}

		static void TearDownTestCase() {
			//...//
		}

		virtual void SetUp() {
			// m_client.reset(new TestUtil::TcpServer(tcpS));
		}

		virtual void TearDown() {
			// m_client.reset();
		}

	private:

		std::auto_ptr<TestUtil::TcpClient> m_client;


	};


	TEST(Client, Tcp) {

		FAIL() << "Client";


	}


}
