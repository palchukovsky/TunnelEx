/**************************************************************************
 *   Created: 2011/07/13 13:04
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "ConnectServer.hpp"
#include "TestUtils/PipeServer.hpp"

namespace {

	//////////////////////////////////////////////////////////////////////////
		
	class PipeServer : public testing::ConnectServer {
	public:
		virtual ~PipeServer() {
			//...//
		}
	protected:
		virtual std::auto_ptr<TestUtil::Server> CreateServer(
					const boost::posix_time::time_duration &waitTime)
				const {
			std::auto_ptr<TestUtil::Server> result(
				new TestUtil::PipeServer(testing::pipeServerPath, waitTime));
			return result;
		}
	};

	//////////////////////////////////////////////////////////////////////////

	TEST_F(PipeServer, Any) {
		ASSERT_TRUE(TestAny());
	}

	//////////////////////////////////////////////////////////////////////////

}
