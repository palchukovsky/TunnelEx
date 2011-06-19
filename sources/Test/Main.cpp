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
			std::cout << "Press Enter to exit." << std::endl;
			getchar();
		}
	};

}

int main(int argc, char **argv) {

	const bool isServer
		= argc < 2
		|| boost::iequals(argv[1], "tcpserver")
		|| boost::iequals(argv[1], "tcp-server");
	const bool isClient
		= argc < 2
		|| boost::iequals(argv[1], "tcpclient")
		|| boost::iequals(argv[1], "tcp-client");
	assert(!isServer || isServer != isClient);

	CloseStopper closeStopper;

	testing::AddGlobalTestEnvironment(new Environment);

	if (!isServer && !isClient) {
		
		testing::AddGlobalTestEnvironment(new LocalEnvironment);
		
		std::string filter = testing::GTEST_FLAG(filter).c_str();
		filter
			+= "-TcpClient.*:TcpServer.*"
				":UdpClient.*:UdpServer.*"
				":PipeClient.*:PipeServer.*";
		testing::GTEST_FLAG(filter) = filter;
	
	} else if (isServer) {
		assert(!isClient);
		
		testing::AddGlobalTestEnvironment(new ServerEnvironment);
		
		if (testing::GTEST_FLAG(filter) == "*") {
			testing::GTEST_FLAG(filter) = "TcpServer.*";
			testing::GTEST_FLAG(repeat) = -1;
		}
	
	} else {
		assert(isClient);
		
		testing::AddGlobalTestEnvironment(new ClientEnvironment);
		
		if (testing::GTEST_FLAG(filter) == "*") {
			testing::GTEST_FLAG(filter) = "TcpClient.*";
#			ifndef DEV_VER 
				testing::GTEST_FLAG(shuffle) = 1;
#			endif
		}

	}

	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();

}
