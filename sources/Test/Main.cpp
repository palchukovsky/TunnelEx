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

	enum Mode {
		MODE_COMMON,
		MODE_TCP_SERVER,
		MODE_TCP_CLIENT,
		MODE_UDP_SERVER,
		MODE_UDP_CLIENT,
		MODE_PIPE_SERVER,
		MODE_PIPE_CLIENT
	} mode = MODE_COMMON;

	if (argc >= 2) {
		if (boost::iequals(argv[1], "tcpserver") || boost::iequals(argv[1], "tcp-server")) {
			mode = MODE_TCP_SERVER;
		} else if (boost::iequals(argv[1], "tcpclient") || boost::iequals(argv[1], "tcp-client")) {
			mode = MODE_TCP_CLIENT;
		} else if (boost::iequals(argv[1], "udpserver") || boost::iequals(argv[1], "udp-server")) {
			mode = MODE_UDP_SERVER;
		} else if (boost::iequals(argv[1], "udpclient") || boost::iequals(argv[1], "udp-client")) {
			mode = MODE_UDP_CLIENT;
		} else if (boost::iequals(argv[1], "pipeserver") || boost::iequals(argv[1], "pipe-server")) {
			mode = MODE_PIPE_SERVER;
		} else if (boost::iequals(argv[1], "pipeclient") || boost::iequals(argv[1], "pipe-client")) {
			mode = MODE_PIPE_CLIENT;
		}
	}

	CloseStopper closeStopper;

	testing::AddGlobalTestEnvironment(new Environment);
	testing::InitGoogleTest(&argc, argv);

	switch (mode) {
		case  MODE_COMMON:
			{
				testing::AddGlobalTestEnvironment(new LocalEnvironment);
				std::string filter = testing::GTEST_FLAG(filter).c_str();
				filter
					+= "-TcpClient.*:TcpServer.*"
						":UdpClient.*:UdpServer.*"
						":PipeClient.*:PipeServer.*";
				testing::GTEST_FLAG(filter) = filter;
			}
			break;
		case MODE_TCP_SERVER:
			testing::AddGlobalTestEnvironment(new ServerEnvironment);
			testing::GTEST_FLAG(filter) = "TcpServer.*";
			testing::GTEST_FLAG(repeat) = -1;
			break;
		case MODE_TCP_CLIENT:
			testing::AddGlobalTestEnvironment(new ClientEnvironment);
			if (testing::GTEST_FLAG(filter) == "*") {
				testing::GTEST_FLAG(filter) = "TcpClient.*";
#				ifndef DEV_VER 
					testing::GTEST_FLAG(shuffle) = 1;
#				endif
			}
			break;
		case MODE_UDP_SERVER:
			testing::AddGlobalTestEnvironment(new ServerEnvironment);
			testing::GTEST_FLAG(filter) = "UdpServer.*";
			testing::GTEST_FLAG(repeat) = -1;
			break;
		case MODE_UDP_CLIENT:
			testing::AddGlobalTestEnvironment(new ClientEnvironment);
			if (testing::GTEST_FLAG(filter) == "*") {
				testing::GTEST_FLAG(filter) = "UdpClient.*";
#				ifndef DEV_VER 
					testing::GTEST_FLAG(shuffle) = 1;
#				endif
			}
			break;
		default:
			assert(false);
	}

	return RUN_ALL_TESTS();

}
