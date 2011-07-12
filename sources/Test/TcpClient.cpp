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

template<>
std::auto_ptr<TestUtil::Client>
testing::ConnectClient<TestUtil::TcpClient>::CreateClient()
		const {
	std::auto_ptr<TestUtil::Client> result(
		new Client("localhost", testing::tcpServerPort));
	return result;
}

template<>
std::auto_ptr<TestUtil::Client>
testing::ConnectClient<TestUtil::TcpClient>::CreateCloseControlConnection()
		const {
	std::auto_ptr<TestUtil::Client> result(
		new Client("localhost", testing::tcpServerPort));
	result->SetWaitTime(GetClient().GetWaitTime());
	Connect(*result, testing::serverMagicDummyMode, false);
	return result;
}

namespace {

	class TcpClient : public testing::ConnectClient<TestUtil::TcpClient> {
		//...//
	};

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

}
