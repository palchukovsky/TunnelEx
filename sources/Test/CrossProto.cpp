/**************************************************************************
 *   Created: 2009/04/12 14:26
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 Eugene V. Palchukovsky
 * -------------------------------------------------------------------
 *       $Id: CrossProto.cpp 665 2009-06-01 11:38:39Z palchukovsky $
 **************************************************************************/

#include "PipeServer.hpp"
#include "InetServer.hpp"
#include "Wait.h"

#include <TunnelEx/Rule.hpp>
#include <TunnelEx/Server.hpp>

#pragma warning(push, 3)
#	include <boost/test/unit_test.hpp>
#	include <boost/shared_ptr.hpp>
#	include <boost/format.hpp>
#pragma warning(pop)
#include <string>
#include <windows.h>

using namespace std;
namespace ut = boost::unit_test;
using namespace boost;
using namespace Test;
namespace tex = TunnelEx;

BOOST_AUTO_TEST_SUITE(CrossProto)

//! For TEX-430
BOOST_AUTO_TEST_CASE(UdpToPipe) {

	const unsigned short udpPort = 10000;
	const wchar_t *const pipe = L"tunnelex/test_pipe";

	shared_ptr<PipeServer> server;
	BOOST_REQUIRE_NO_THROW(server.reset(new PipeServer(pipe)));

	wformat xml(
		L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		L"<RuleCollection Version=\"1.1\">"
		L"	<Rule Name=\"First rule\" Uuid=\"AC115577-E705-480a-BAE9-D16D522DB726\">"
		L"		<FilterCollection />"
		L"		<InputCollection>"
		L"			<Endpoint ResourceIdentifier=\"udp://localhost:%1%\" Uuid=\"A1055577-E705-481a-BAE9-D16D5221B127\" />"
		L"		</InputCollection>"
		L"		<DestinationCollection>"
		L"			<Endpoint ResourceIdentifier=\"pipe://%2%\" Uuid=\"A2055577-E705-481a-BAE9-D16D5221B126\" />"
		L"		</DestinationCollection>"
		L"	</Rule>"
		L"</RuleCollection>");
	xml % udpPort % pipe;
	tex::Server::GetInstance().Start(tex::RuleCollection(xml.str().c_str()));

	TEST_WAIT_AND_REQUIRE_MESSAGE(
		server->GetNumberOfAcceptedConnections() == 1,
		10,
		250,
		"Wrong accepted pipe connections number: " << server->GetNumberOfAcceptedConnections() << ".");

	shared_ptr<UdpClient> client;
	BOOST_REQUIRE_NO_THROW(client.reset(new UdpClient(udpPort)));

	string testMessage = "Client send test...";
	BOOST_REQUIRE_NO_THROW(client->Send(testMessage));
	BOOST_REQUIRE_NO_THROW(string(&server->Receive(0)[0]) == testMessage);

	//! @todo: uncoment after will be implemented "Reverse Endpoint" (https://wiki.palchukovsky.com/display/tex/Reverse+Endpoint)
	/* testMessage = "Server send test...";
	BOOST_REQUIRE_NO_THROW(server->Send(0, testMessage));
	BOOST_REQUIRE_NO_THROW(string(&client->Receive()[0]) == testMessage); */
	
	tex::Server::GetInstance().Stop();

}

BOOST_AUTO_TEST_SUITE_END()