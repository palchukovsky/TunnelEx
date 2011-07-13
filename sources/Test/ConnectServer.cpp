/**************************************************************************
 *   Created: 2011/07/13 11:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "ConnectServer.hpp"

using namespace testing;

ConnectServer::~ConnectServer() {
	//...//
}
		
void ConnectServer::SetUpTestCase() {
	//...//
}
		
void ConnectServer::TearDownTestCase() {
	//...//
}
		
void ConnectServer::SetUp() {
	assert(!m_server.get());
	std::auto_ptr<TestUtil::Server> server(CreateServer());
	server->SetWaitTime(testing::defaultDataWaitTime);
	const bool waitResult = server->WaitConnect(1, true);
	assert(waitResult);
	UseUnused(waitResult);
	m_server = server;
}
		
void ConnectServer::TearDown() {
	m_server.reset();
}

bool ConnectServer::TestAny() {
	bool result = true;
	DoTest(result);
	return result;
}

bool ConnectServer::TestActiveServer(size_t mainConnection) {
	bool result = false;
	DoTestActiveServer(mainConnection, result);
	return result;
}

bool ConnectServer::TestPassiveServer(size_t mainConnection) {
	bool result = false;
	DoTestPassiveServer(mainConnection, result);
	return result;
}

bool ConnectServer::TestOneWayActiveServer(size_t mainConnection) {
	bool result = false;
	DoTestOneWayActiveServer(mainConnection, result);
	return result;
}

bool ConnectServer::TestOneWayPassiveServer(size_t mainConnection) {
	bool result = false;
	DoTestOneWayPassiveServer(mainConnection, result);
	return result;
}

bool ConnectServer::TestSeveralConnections(size_t mainConnection) {
	bool result = false;
	DoTestSeveralConnections(mainConnection, result);
	return result;
}

void ConnectServer::DoTest(bool &result) {

	ASSERT_GT(m_server->GetNumberOfAcceptedConnections(false), size_t(0));
	const size_t connection = 0;

	ASSERT_TRUE(
		m_server->WaitAndTakeData(connection, testing::clientMagicHello, false));
		
	enum ServerMode {
		SERVER_MODE_ACTIVE = 1,
		SERVER_MODE_PASSIVE = 2,
		SERVER_MODE_ONE_WAY_ACTIVE = 3,
		SERVER_MODE_ONE_WAY_PASSIVE = 4,
		SERVER_MODE_SEVERAL_CONNECTIONS = 5
	} serverMode = SERVER_MODE_ACTIVE;
		
	{
		typedef std::list<const std::string *> Modes;
		Modes modes;
		modes.push_back(&testing::serverMagicActiveMode);
		modes.push_back(&testing::serverMagicPassiveMode);
		modes.push_back(&testing::serverMagicOneWayActiveMode);
		modes.push_back(&testing::serverMagicOneWayPassiveMode);
		modes.push_back(&testing::serverMagicSeveralConnectionsMode);
		int serverModePos = 0;
		ASSERT_NO_THROW( 
			serverModePos = m_server->WaitAndTakeData(connection, modes, true));
		ASSERT_GT(serverModePos, 0);
		ASSERT_LT(serverModePos, 6);
		serverMode = ServerMode(serverModePos);
		Modes::const_iterator serverModeItPos = modes.begin();
		std::advance(serverModeItPos, serverModePos - 1);
		std::cout << **serverModeItPos << " (" << serverModePos << ")";
	}

	ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicHello));

	switch (serverMode) {
		case  SERVER_MODE_ACTIVE:
			std::cout << " - server active mode" << std::endl;
			ASSERT_TRUE(TestActiveServer(connection));
			break;
		case SERVER_MODE_PASSIVE:
			std::cout << " - server passive mode" << std::endl;
			ASSERT_TRUE(TestPassiveServer(connection));
			break;
		case SERVER_MODE_ONE_WAY_ACTIVE:
			std::cout << " - server one-way active mode" << std::endl;
			ASSERT_TRUE(TestOneWayActiveServer(connection));
			break;
		case SERVER_MODE_ONE_WAY_PASSIVE:
			std::cout << " - server one-way passive mode" << std::endl;
			ASSERT_TRUE(TestOneWayPassiveServer(connection));
			break;
		case SERVER_MODE_SEVERAL_CONNECTIONS:
			std::cout << " - server several connections mode" << std::endl;
			ASSERT_TRUE(TestSeveralConnections(connection));
			break;
		default:
			FAIL() << "Doesn't implemented yet.";
			break;
	}

	result = true;

}

void ConnectServer::DoTestActiveServer(size_t connection, bool &result) {

	const testing::PacketsNumber packets = 100;
	ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicBegin));
	ASSERT_NO_THROW(m_server->SendVal(connection, packets));
	ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicEnd));

	for (testing::PacketsNumber i = 0; i < packets; ++i) {
		ASSERT_TRUE(SendTestPacket(connection, 256, 0.5));
		ASSERT_TRUE(
			m_server->WaitAndTakeData(connection, testing::clientMagicOk, false));
		ASSERT_TRUE(ReceiveTestPacket(connection));
		ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicOk));
	}

	ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicBay));
	ASSERT_TRUE(
		m_server->WaitAndTakeData(connection, testing::clientMagicBay, true));
	EXPECT_EQ(0, m_server->GetReceivedSize(connection));
	m_server->CloseConnection(connection);

	result = true;

}

void ConnectServer::DoTestPassiveServer(size_t connection, bool &result) {

	testing::PacketsNumber packets = 0;
	ASSERT_TRUE(
		m_server->WaitAndTakeData(connection, testing::clientMagicBegin, false));
	ASSERT_NO_THROW(
		packets = m_server->WaitAndTakeData<testing::PacketsNumber>(connection, false));
	ASSERT_GT(packets, testing::PacketsNumber(0));
	ASSERT_TRUE(
		m_server->WaitAndTakeData(connection, testing::clientMagicEnd, false));

	for (testing::PacketsNumber i = 0; i < packets; ++i) {
		ASSERT_TRUE(ReceiveTestPacket(connection));
		ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicOk));
		ASSERT_TRUE(SendTestPacket(connection, 512, 0.5));
		ASSERT_TRUE(
			m_server->WaitAndTakeData(connection, testing::clientMagicOk, false));
	}

	ASSERT_TRUE(
		m_server->WaitAndTakeData(connection, testing::clientMagicBay, true));
	ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicBay));
	EXPECT_EQ(0, m_server->GetReceivedSize(connection));
	ASSERT_TRUE(m_server->WaitDisconnect(connection));

	result = true;

}

void ConnectServer::DoTestOneWayActiveServer(size_t connection, bool &result) {

	const testing::PacketsNumber packets = 1024 * 3;
	ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicBegin));
	ASSERT_NO_THROW(m_server->SendVal(connection, packets));
	ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicEnd));

	for (testing::PacketsNumber i = 0; i < packets; ++i) {
		ASSERT_TRUE(SendTestPacket(connection, 1024, 0.95));
	}

	ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicBay));
	const auto waitTime = m_server->GetWaitTime();
	m_server->SetWaitTime(waitTime * 3);
	ASSERT_TRUE(
		m_server->WaitAndTakeData(
			connection,
			testing::clientMagicBay,
			true));
	m_server->SetWaitTime(waitTime);
	m_server->CloseConnection(connection);

	result = true;

}

void ConnectServer::DoTestOneWayPassiveServer(size_t connection, bool &result) {

	testing::PacketsNumber packets = 0;
	ASSERT_TRUE(
		m_server->WaitAndTakeData(connection, testing::clientMagicBegin, false));
	ASSERT_NO_THROW(
		packets = m_server->WaitAndTakeData<testing::PacketsNumber>(connection, false));
	ASSERT_GT(packets, testing::PacketsNumber(0));
	ASSERT_TRUE(
		m_server->WaitAndTakeData(connection, testing::clientMagicEnd, false));

	int lastPersents = 0;
	for (testing::PacketsNumber i = 0; i < packets; ++i) {
		ASSERT_TRUE(ReceiveTestPacket(connection));
		const int persents = (((i + 1) * 100) / packets);
		if (!(persents % 10) && persents > lastPersents) {
			std::cout
				<< "received "
				<< (i + 1) << " from " << packets
				<< " (" << persents << "%)"
				<< std::endl;
			lastPersents = persents;
		}
	}

	ASSERT_TRUE(
		m_server->WaitAndTakeData(connection, testing::clientMagicBay, true));
	ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicBay));
	EXPECT_EQ(0, m_server->GetReceivedSize(connection));
	m_server->CloseConnection(connection);

	result = true;

}

void ConnectServer::DoTestSeveralConnections(size_t mainConnection, bool &result) {

	const testing::PacketSize packetSize = 128;

	testing::ConnectionsNumber connectionsNumber = 0;
	ASSERT_TRUE(
		m_server->WaitAndTakeData(
			mainConnection,
			testing::clientMagicBegin,
			false));
	ASSERT_NO_THROW(
		connectionsNumber = m_server->WaitAndTakeData<testing::ConnectionsNumber>(
			mainConnection,
			false));
	ASSERT_GT(connectionsNumber, testing::ConnectionsNumber(0));
	ASSERT_TRUE(
		m_server->WaitAndTakeData(
			mainConnection,
			testing::clientMagicEnd,
			false));

	for (testing::ConnectionsNumber i = 1; i <= connectionsNumber; ++i) {
		const size_t connection = i + mainConnection;
		ASSERT_TRUE(m_server->WaitConnect(connection + 1, false))
			<< "Failed to accept sub connection #" << i << ".";
		ASSERT_TRUE(
			m_server->WaitAndTakeData(connection, testing::clientMagicHello, false));
		testing::ConnectionsNumber remoteI = 0;
		ASSERT_NO_THROW(
			remoteI = m_server->WaitAndTakeData<testing::ConnectionsNumber>(connection, false));
		ASSERT_EQ(i, remoteI);
		ASSERT_TRUE(
			m_server->WaitAndTakeData(
				connection,
				testing::serverMagicSubConnectionMode,
				true));
		ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicHello));
		ASSERT_NO_THROW(m_server->SendVal(connection, i));
	}

	testing::PacketsNumber packetsNumber = 0;
	ASSERT_TRUE(
		m_server->WaitAndTakeData(
			mainConnection,
			testing::clientMagicBegin,
			false));
	ASSERT_NO_THROW(
		packetsNumber = m_server->WaitAndTakeData<testing::PacketsNumber>(
			mainConnection,
			false));
	ASSERT_TRUE(
		m_server->WaitAndTakeData(
			mainConnection,
			testing::clientMagicEnd,
			false));

	for (size_t i = 0; i < 2; ++i) {
		for (testing::PacketsNumber i = 0; i < packetsNumber; ++i) {
			for (testing::ConnectionsNumber i = 1; i <= connectionsNumber; ++i) {
				const size_t connection = i + mainConnection;
				testing::ConnectionsNumber remoteI = 0;
				ASSERT_NO_THROW(
					remoteI = m_server->WaitAndTakeData<testing::ConnectionsNumber>(connection, false));
				EXPECT_EQ(i, remoteI);
				ASSERT_TRUE(ReceiveTestPacket(connection));
				ASSERT_NO_THROW(
					m_server->Send(
						connection,
						testing::serverMagicOk));
				ASSERT_NO_THROW(
					m_server->SendVal(connection, testing::ConnectionsNumber(i)));
				ASSERT_TRUE(SendTestPacket(connection, packetSize, 0.5));
				ASSERT_TRUE(
					m_server->WaitAndTakeData(
						connection,
						testing::clientMagicOk,
						false));
			}
		}
	}

	for (testing::ConnectionsNumber i = 1;  i <= connectionsNumber; ++i) {
		const size_t connection = i + mainConnection;
		const bool isActiveDisconnect = !(i % 2);
		testing::ConnectionsNumber remoteI = 0;
		if (isActiveDisconnect) {
			ASSERT_NO_THROW(m_server->SendVal(connection, i));
			ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicBay));
			ASSERT_NO_THROW(
				remoteI = m_server->WaitAndTakeData<testing::ConnectionsNumber>(connection, false));
			ASSERT_EQ(i, remoteI);
			ASSERT_TRUE(
				m_server->WaitAndTakeData(
					connection,
					testing::clientMagicBay,
					true));
			EXPECT_EQ(0, m_server->GetReceivedSize(connection));
			ASSERT_NO_THROW(m_server->CloseConnection(connection));
		} else {
			ASSERT_NO_THROW(
				remoteI = m_server->WaitAndTakeData<testing::ConnectionsNumber>(connection, false));
			ASSERT_EQ(i, remoteI);
			ASSERT_TRUE(
				m_server->WaitAndTakeData(
					connection,
					testing::clientMagicBay,
					true));
			ASSERT_NO_THROW(m_server->SendVal(connection, i));
			ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicBay));
			EXPECT_EQ(0, m_server->GetReceivedSize(connection));
 			EXPECT_TRUE(m_server->WaitDisconnect(connection))
 				<< "Failed to receive DISCONNECT event from sub connection #" << i << ".";
		}
	}

	ASSERT_TRUE(
		m_server->WaitAndTakeData(
			mainConnection,
			testing::clientMagicBay,
			true));
	ASSERT_NO_THROW(m_server->Send(mainConnection, testing::serverMagicBay));
	EXPECT_EQ(0, m_server->GetReceivedSize(mainConnection));
	ASSERT_TRUE(m_server->WaitDisconnect(mainConnection));

	result = true;

}

bool ConnectServer::SendTestPacket(
			size_t connection,
			testing::PacketSize size,
			double widthRatio) {
	bool result = false;
	DoSendTestPacket(connection, size, widthRatio, result);
	return result;
}

void ConnectServer::DoSendTestPacket(
			size_t connection,
			testing::PacketSize size,
			double widthRatio,
			bool &result) {

	ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicBegin));

	std::auto_ptr<TestUtil::Buffer> packet(new TestUtil::Buffer);
	boost::crc_32_type crc;
	testing::GeneratePacket(*packet, crc, size - (size * widthRatio), size + (size * widthRatio));

	ASSERT_NO_THROW(
		m_server->SendVal(connection, testing::PacketSize(packet->size())));
	ASSERT_NO_THROW(m_server->Send(connection, packet));
	ASSERT_NO_THROW(m_server->SendVal(connection, crc.checksum()));
	ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicEnd));

	result = true;

}

bool ConnectServer::ReceiveTestPacket(size_t connection) {
	bool result = false;
	DoReceiveTestPacket(connection, result);
	return result;
}

void ConnectServer::DoReceiveTestPacket(size_t connection, bool &result) {
	testing::PacketSize size = 0;
	ASSERT_TRUE(
		m_server->WaitAndTakeData(connection, testing::clientMagicBegin, false));
	ASSERT_NO_THROW(
		size = m_server->WaitAndTakeData<testing::PacketSize>(connection, false));
	EXPECT_GT(size, 0);
	TestUtil::Buffer packet;
	ASSERT_NO_THROW(
		m_server->WaitAndTakeAnyData(connection, size, false, packet));
	boost::crc_32_type realCrc;
	testing::Calc(packet, realCrc);
	boost::crc_32_type::value_type remoteCrc;
	ASSERT_NO_THROW(
		remoteCrc = m_server->WaitAndTakeData<boost::crc_32_type::value_type>(
			connection,
			false));
	EXPECT_EQ(realCrc.checksum(), remoteCrc);
	ASSERT_TRUE(
		m_server->WaitAndTakeData(connection, testing::clientMagicEnd, false));
	result = true;
}
