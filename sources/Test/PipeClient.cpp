/**************************************************************************
 *   Created: 2011/07/13 0:03
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "ConnectClient.hpp"
#include "TestUtils/PipeClient.hpp"

namespace {

	//////////////////////////////////////////////////////////////////////////

	class PipeClient : public testing::ConnectClient {
	public:
		virtual ~PipeClient() {
			//...//
		}
	protected:
		virtual std::auto_ptr<TestUtil::Client> CreateClient() const {
			std::auto_ptr<TestUtil::Client> result(
				new TestUtil::PipeClient(testing::pipeServerPath));
			return result;
		}
	};

	//////////////////////////////////////////////////////////////////////////

	TEST_F(PipeClient, DataExchangeActive) {
		ASSERT_TRUE(TestDataExchangeActive());
	}

	TEST_F(PipeClient, DataExchangePassive) {
		ASSERT_TRUE(TestDataExchangePassive());
	}

	TEST_F(PipeClient, DataExchangeOneWayActive) {
		ASSERT_TRUE(TestDataExchangeOneWayActive());
	}

	TEST_F(PipeClient, DataOneWayExchangePassive) {
		ASSERT_TRUE(TestDataOneWayExchangePassive());
	}

	TEST_F(PipeClient, SeveralConnetions) {
		ASSERT_TRUE(TestSeveralConnetions());
	}

	//////////////////////////////////////////////////////////////////////////

}
