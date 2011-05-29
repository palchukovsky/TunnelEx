/**************************************************************************
 *   Created: 2007/12/15 15:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.com
 **************************************************************************/

#include "Prec.h"
#include "Environment.hpp"

namespace {

	struct CloseStopper {
		~CloseStopper() {
			getchar();
		}
	};

}

int main(int argc, char **argv) {

	const bool isServer = argc < 2 || boost::iequals(argv[1], "server");
	const bool isClient = argc < 2 || boost::iequals(argv[1], "client");
	assert(!isServer || isServer != isClient);

	CloseStopper closeStopper;

	testing::AddGlobalTestEnvironment(new Test::Environment);

	if (!isServer && !isClient) {
		testing::AddGlobalTestEnvironment(new Test::LocalEnvironment);
	} else if (isServer) {
		assert(!isServer);
		testing::AddGlobalTestEnvironment(new Test::ServerEnvironment);
		testing::GTEST_FLAG(filter) = "Server.*";
	} else {
		assert(isClient);
		testing::AddGlobalTestEnvironment(new Test::ClientEnvironment);
		testing::GTEST_FLAG(filter) = "Client.*";
	}

	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();

}
