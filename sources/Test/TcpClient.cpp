/**************************************************************************
 *   Created: 2011/05/30 23:13
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "ConnectClient.hpp"
#include "TestUtils/InetClient.hpp"

namespace {

	//////////////////////////////////////////////////////////////////////////

	class TcpClient : public testing::ConnectClient {
	public:
		virtual ~TcpClient() {
			//...//
		}
	protected:
		virtual std::auto_ptr<TestUtil::Client> CreateClient(
					const boost::posix_time::time_duration &waitTime)
				const {
			std::auto_ptr<TestUtil::Client> result(
				new TestUtil::TcpClient(
					testing::tcpServerHost,
					testing::tcpServerPort,
					waitTime));
			return result;
		}
	};

	//////////////////////////////////////////////////////////////////////////

	TEST_F(TcpClient, DataExchangeActive) {
		ASSERT_TRUE(TestDataExchangeActive());
	}

	TEST_F(TcpClient, DataExchangePassive) {
		ASSERT_TRUE(TestDataExchangePassive());
	}

	TEST_F(TcpClient, DataExchangeOneWayActive) {
		ASSERT_TRUE(TestDataExchangeOneWayActive());
	}

	TEST_F(TcpClient, DataOneWayExchangePassive) {
		ASSERT_TRUE(TestDataOneWayExchangePassive());
	}

	TEST_F(TcpClient, SeveralConnetions) {
		ASSERT_TRUE(TestSeveralConnetions());
	}

	//////////////////////////////////////////////////////////////////////////

}
