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
#include "Common.hpp"

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

	for (auto i = 1; i < argc; ++i) {
		if (boost::iequals(argv[i], "tcpserver") || boost::iequals(argv[i], "tcp-server")) {
			mode = MODE_TCP_SERVER;
		} else if (boost::iequals(argv[i], "tcpclient") || boost::iequals(argv[i], "tcp-client")) {
			mode = MODE_TCP_CLIENT;
		} else if (boost::iequals(argv[i], "udpserver") || boost::iequals(argv[i], "udp-server")) {
			mode = MODE_UDP_SERVER;
		} else if (boost::iequals(argv[i], "udpclient") || boost::iequals(argv[i], "udp-client")) {
			mode = MODE_UDP_CLIENT;
		} else if (boost::iequals(argv[i], "pipeserver") || boost::iequals(argv[i], "pipe-server")) {
			mode = MODE_PIPE_SERVER;
		} else if (boost::iequals(argv[i], "pipeclient") || boost::iequals(argv[i], "pipe-client")) {
			mode = MODE_PIPE_CLIENT;
		} else if (boost::istarts_with(argv[i], "point=") || boost::istarts_with(argv[i], "endpoint=")) {
			std::string point = argv[i];
			point = point.substr(point.find('=') + 1);
			testing::pipeServerPath = point;
			if (point.find(':') != std::string::npos) {
				testing::tcpServerHost
					= testing::udpServerHost
					= point.substr(0, point.find(':'));
				testing::tcpServerPort
					= testing::udpServerPort
					= boost::lexical_cast<unsigned short>(point.substr(point.find(':') + 1));
			} else {
				testing::tcpServerHost
					= testing::udpServerHost
					= point;
			}
			
		}
	}

	CloseStopper closeStopper;

	testing::AddGlobalTestEnvironment(new Environment);
	testing::InitGoogleTest(&argc, argv);

	switch (mode) {
		case  MODE_COMMON:
			{
				testing::AddGlobalTestEnvironment(new LocalEnvironment);
				const char *const noServerTestsFilter
					=	"-TcpClient.*:TcpServer.*"
						":UdpClient.*:UdpServer.*"
						":PipeClient.*:PipeServer.*";
				std::string newFilter = testing::GTEST_FLAG(filter);
				if (newFilter != "*") {
					newFilter += std::string(":") + noServerTestsFilter;
				} else {
					newFilter = noServerTestsFilter;
				}
				testing::GTEST_FLAG(filter) = newFilter;
			}
			break;
		case MODE_TCP_SERVER:
			testing::AddGlobalTestEnvironment(new ServerEnvironment);
			testing::GTEST_FLAG(filter) = "TcpServer.*";
			testing::GTEST_FLAG(repeat) = -1;
			std::cout
				<< "Using endpoint: \"*:" << testing::tcpServerPort << "\"..."
				<< std::endl;
			break;
		case MODE_TCP_CLIENT:
			testing::AddGlobalTestEnvironment(new ClientEnvironment);
			if (testing::GTEST_FLAG(filter) == "*") {
				testing::GTEST_FLAG(filter) = "TcpClient.*";
#				ifndef DEV_VER 
					testing::GTEST_FLAG(shuffle) = 1;
#				endif
			} else {
				testing::GTEST_FLAG(filter)
					= std::string("TcpClient.") + testing::GTEST_FLAG(filter).c_str();
			}
			testing::GTEST_FLAG(break_on_failure) = 1;
			std::cout
				<< "Using endpoint: \""
				<< testing::tcpServerHost << ":" << testing::tcpServerPort << "\"..."
				<< std::endl;
			break;
		case MODE_UDP_SERVER:
			testing::AddGlobalTestEnvironment(new ServerEnvironment);
			testing::GTEST_FLAG(filter) = "UdpServer.*";
			testing::GTEST_FLAG(repeat) = -1;
			std::cout
				<< "Using endpoint: \"*:" << testing::udpServerPort << "\"..."
				<< std::endl;
			break;
		case MODE_UDP_CLIENT:
			testing::AddGlobalTestEnvironment(new ClientEnvironment);
			if (testing::GTEST_FLAG(filter) == "*") {
				testing::GTEST_FLAG(filter) = "UdpClient.*";
#				ifndef DEV_VER 
					testing::GTEST_FLAG(shuffle) = 1;
#				endif
			} else {
				testing::GTEST_FLAG(filter)
					= std::string("UdpClient.") + testing::GTEST_FLAG(filter).c_str();
			}
			testing::GTEST_FLAG(break_on_failure) = 1;
			std::cout
				<< "Using endpoint: \""
				<< testing::udpServerHost << ":" << testing::udpServerPort << "\"..."
				<< std::endl;
			break;
		case MODE_PIPE_SERVER:
			testing::AddGlobalTestEnvironment(new ServerEnvironment);
			testing::GTEST_FLAG(filter) = "PipeServer.*";
			testing::GTEST_FLAG(repeat) = -1;
			std::cout << "Using endpoint: \"" << testing::pipeServerPath << "\"..." << std::endl;
			break;
		case MODE_PIPE_CLIENT:
			testing::AddGlobalTestEnvironment(new ClientEnvironment);
			if (testing::GTEST_FLAG(filter) == "*") {
				testing::GTEST_FLAG(filter) = "PipeClient.*";
#				ifndef DEV_VER 
					testing::GTEST_FLAG(shuffle) = 1;
#				endif
			} else {
				testing::GTEST_FLAG(filter)
					= std::string("PipeClient.") + testing::GTEST_FLAG(filter).c_str();
			}
			testing::GTEST_FLAG(break_on_failure) = 1;
			std::cout << "Using endpoint: \"" << testing::pipeServerPath << "\"..." << std::endl;
			break;
		default:
			assert(false);
	}

	return RUN_ALL_TESTS();

}
