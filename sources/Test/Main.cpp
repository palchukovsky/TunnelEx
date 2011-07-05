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

	const bool isTcpServer
		= argc < 2
		|| boost::iequals(argv[1], "tcpserver")
		|| boost::iequals(argv[1], "tcp-server");
	const bool isTcpClient
		= argc < 2
		|| boost::iequals(argv[1], "tcpclient")
		|| boost::iequals(argv[1], "tcp-client");
	const bool isUdpServer
		= argc < 2
		|| boost::iequals(argv[1], "udpserver")
		|| boost::iequals(argv[1], "udp-server");
	const bool isUdpClient
		= argc < 2
		|| boost::iequals(argv[1], "udpclient")
		|| boost::iequals(argv[1], "udp-client");
	assert(!isTcpServer || isTcpServer != isTcpClient);
	assert(!isUdpServer || isUdpServer != isUdpClient);

	CloseStopper closeStopper;

	testing::AddGlobalTestEnvironment(new Environment);
	testing::InitGoogleTest(&argc, argv);

	if (!isTcpServer && !isTcpClient && !isUdpServer && !isUdpClient) {
		
		testing::AddGlobalTestEnvironment(new LocalEnvironment);
		
		std::string filter = testing::GTEST_FLAG(filter).c_str();
		filter
			+= "-TcpClient.*:TcpServer.*"
				":UdpClient.*:UdpServer.*"
				":PipeClient.*:PipeServer.*";
		testing::GTEST_FLAG(filter) = filter;
	
	} else if (isTcpServer) {
		assert(!isTcpClient && !isUdpClient && !isUdpServer);
		
		testing::AddGlobalTestEnvironment(new ServerEnvironment);
		
		testing::GTEST_FLAG(filter) = "TcpServer.*";
		testing::GTEST_FLAG(repeat) = -1;
	
	} else if (isTcpClient) {
		assert(!isUdpClient && !isUdpServer);
		
		testing::AddGlobalTestEnvironment(new ClientEnvironment);
		
		if (testing::GTEST_FLAG(filter) == "*") {
			testing::GTEST_FLAG(filter) = "TcpClient.*";
#			ifndef DEV_VER 
				testing::GTEST_FLAG(shuffle) = 1;
#			endif
		}

	} else if (isUdpServer) {
		assert(!isUdpClient && !isTcpClient && !isTcpServer);
		
		testing::AddGlobalTestEnvironment(new ServerEnvironment);
	
		testing::GTEST_FLAG(filter) = "UdpServer.*";
		testing::GTEST_FLAG(repeat) = -1;
	
	} else if (isUdpClient) {
		assert(!isTcpClient && !isTcpServer);
		
		testing::AddGlobalTestEnvironment(new ClientEnvironment);
		
		if (testing::GTEST_FLAG(filter) == "*") {
			testing::GTEST_FLAG(filter) = "UdpClient.*";
#			ifndef DEV_VER 
				testing::GTEST_FLAG(shuffle) = 1;
#			endif
		}

	}

	return RUN_ALL_TESTS();

}
