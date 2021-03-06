/**************************************************************************
 *   Created: 2011/07/13 11:49
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "ConnectClient.hpp"
#include "TestUtils/ClientServer.hpp"

using namespace testing;

ConnectClient::~ConnectClient() {
	//...//
}

bool ConnectClient::TestDataExchangeActive() {
	bool result = false;
	DoDataExchangeActiveTest(result);
	return result;
}

bool ConnectClient::TestDataExchangePassive() {
	bool result = false;
	DoDataExchangePassiveTest(result);
	return result;
}

bool ConnectClient::TestDataExchangeOneWayActive() {
	bool result = false;
	DoDataExchangeOneWayActiveTest(result);
	return result;
}

bool ConnectClient::TestDataOneWayExchangePassive() {
	bool result = false;
	DoDataOneWayExchangePassiveTest(result);
	return result;
}

bool ConnectClient::TestSeveralConnetions() {
	bool result = false;
	DoSeveralConnetionsTest(result);
	return result;
}

std::auto_ptr<TestUtil::Client> ConnectClient::CreateCloseControlConnection() const {
	std::auto_ptr<TestUtil::Client> result(CreateClient(GetClient().GetWaitTime()));
	Connect(*result, testing::serverMagicDummyMode, false);
	return result;
}

void ConnectClient::DoConnect(
			TestUtil::Client &client,
			const std::string &mode,
			bool infiniteTimeout,
			bool &result)
		const {
	ASSERT_TRUE(Connect(client, infiniteTimeout));
	ASSERT_NO_THROW(client.Send(mode));
	result = true;
}

bool ConnectClient::SendTestPacket(testing::PacketSize size, double widthRatio) {
	return SendTestPacket(GetClient(), size, widthRatio);
}

bool ConnectClient::SendTestPacket(
			TestUtil::Client &client,
			testing::PacketSize size,
			double widthRatio)
		const  {
	bool result = false;
	DoSendTestPacket(client, size, widthRatio, result);
	return result;
}

bool ConnectClient::ReceiveTestPacket() {
	return ReceiveTestPacket(GetClient());
}

bool ConnectClient::ReceiveTestPacket(TestUtil::Client &client) const {
	bool result = false;
	DoReceiveTestPacket(client, result);
	return result;
}

void ConnectClient::DoSendTestPacket(
			TestUtil::Client &client,
			testing::PacketSize size,
			double widthRatio,
			bool &result)
		const  {
			
	ASSERT_NO_THROW(client.Send(testing::clientMagicBegin));
			
	std::auto_ptr<TestUtil::Buffer> packet(new TestUtil::Buffer);
	boost::crc_32_type crc;
	testing::GeneratePacket(*packet, crc, size - (size * widthRatio), size + (size * widthRatio));
			
	ASSERT_NO_THROW(client.SendVal(testing::PacketSize(packet->size())));
	ASSERT_NO_THROW(client.Send(packet));
	ASSERT_NO_THROW(client.SendVal(crc.checksum()));
	ASSERT_NO_THROW(client.Send(testing::clientMagicEnd));

	result = true;

}

void ConnectClient::DoReceiveTestPacket(TestUtil::Client &client, bool &result) const {
	ASSERT_TRUE(
		client.WaitAndTakeData(testing::serverMagicBegin, false));
	testing::PacketSize size = 0;
	ASSERT_NO_THROW(size = client.WaitAndTakeData<testing::PacketSize>(false));
	EXPECT_GT(size, 0);
	TestUtil::Buffer packet;
	ASSERT_NO_THROW(client.WaitAndTakeAnyData(size, false, packet));
	boost::crc_32_type realCrc;
	testing::Calc(packet, realCrc);
	boost::crc_32_type::value_type remoteCrc;
	ASSERT_NO_THROW(
		remoteCrc = client.WaitAndTakeData<boost::crc_32_type::value_type>(false));
	EXPECT_EQ(realCrc.checksum(), remoteCrc);
	ASSERT_TRUE(
		client.WaitAndTakeData(testing::serverMagicEnd, false));
	result = true;
}

void ConnectClient::DoDataExchangeActiveTest(bool &result) {

	ASSERT_TRUE(Connect(testing::serverMagicPassiveMode, false));
	ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicHello, true));

	const std::auto_ptr<const TestUtil::Client> closeControlConnection(
		CreateCloseControlConnection());

	const testing::PacketsNumber packets = 100;
	ASSERT_NO_THROW(GetClient().Send(testing::clientMagicBegin));
	ASSERT_NO_THROW(GetClient().SendVal(packets));
	ASSERT_NO_THROW(GetClient().Send(testing::clientMagicEnd));

	for (testing::PacketsNumber i = 0; i < packets; ++i) {
		ASSERT_TRUE(SendTestPacket(128, 0.5));
		ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicOk, false));
		ASSERT_TRUE(ReceiveTestPacket());
		ASSERT_NO_THROW(GetClient().Send(testing::clientMagicOk));
	}

	ASSERT_NO_THROW(GetClient().Send(testing::clientMagicBay));
	ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicBay, true));
	EXPECT_EQ(0, GetClient().GetReceivedSize());
	EXPECT_TRUE(GetClient().IsConnected());
	ASSERT_NO_THROW(GetClient().Disconnect());

	EXPECT_TRUE(closeControlConnection->WaitDisconnect());

	result = true;

}

void ConnectClient::DoDataExchangePassiveTest(bool &result) {

	ASSERT_TRUE(Connect(testing::serverMagicActiveMode, false));
	ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicHello, false));

	const std::auto_ptr<const TestUtil::Client> closeControlConnection(
		CreateCloseControlConnection());

	ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicBegin, false));
	testing::PacketsNumber packets = 0;
	ASSERT_NO_THROW(
		packets = GetClient().WaitAndTakeData<testing::PacketsNumber>(false));
	ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicEnd, false));
	EXPECT_GT(packets, testing::PacketsNumber(0));

	for (testing::PacketsNumber i = 0; i < packets; ++i) {
		ASSERT_TRUE(ReceiveTestPacket());
		ASSERT_NO_THROW(GetClient().Send(testing::clientMagicOk));
		ASSERT_TRUE(SendTestPacket(128, 0.5));
		ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicOk, false));
	}

	ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicBay, true));
	ASSERT_NO_THROW(GetClient().Send(testing::clientMagicBay));
	EXPECT_EQ(0, GetClient().GetReceivedSize());
	EXPECT_TRUE(GetClient().WaitDisconnect());
	EXPECT_EQ(0, GetClient().GetReceivedSize());

	EXPECT_TRUE(closeControlConnection->WaitDisconnect());

	result = true;

}

void ConnectClient::DoDataExchangeOneWayActiveTest(bool &result) {

	const testing::PacketsNumber packets = 1024 * 3;
	const testing::PacketSize packetSize = 1024;

	ASSERT_TRUE(Connect(testing::serverMagicOneWayPassiveMode, false));
	ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicHello, true));

	const std::auto_ptr<const TestUtil::Client> closeControlConnection(
		CreateCloseControlConnection());

	ASSERT_NO_THROW(GetClient().Send(testing::clientMagicBegin));
	ASSERT_NO_THROW(GetClient().SendVal(packets));
	ASSERT_NO_THROW(GetClient().Send(testing::clientMagicEnd));

	for (testing::PacketsNumber i = 0; i < packets; ++i) {
		ASSERT_TRUE(SendTestPacket(packetSize, .95));
	}

	ASSERT_NO_THROW(GetClient().Send(testing::clientMagicBay));
	const auto waitTime = GetClient().GetWaitTime();
	GetClient().SetWaitTime(waitTime * 3);
	ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicBay, true));
	GetClient().SetWaitTime(waitTime);
	ASSERT_NO_THROW(GetClient().Disconnect());

	EXPECT_TRUE(closeControlConnection->WaitDisconnect());

	result = true;

}

void ConnectClient::DoDataOneWayExchangePassiveTest(bool &result) {

	ASSERT_TRUE(Connect(testing::serverMagicOneWayActiveMode, false));
	ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicHello, false));

	const std::auto_ptr<const TestUtil::Client> closeControlConnection(
		CreateCloseControlConnection());

	ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicBegin, false));
	testing::PacketsNumber packets = 0;
	ASSERT_NO_THROW(
		packets = GetClient().WaitAndTakeData<testing::PacketsNumber>(false));
	ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicEnd, false));
	EXPECT_GT(packets, testing::PacketsNumber(0));

	int lastPersents = 0;
	for (testing::PacketsNumber i = 0; i < packets; ++i) {
		ASSERT_TRUE(ReceiveTestPacket());
		const int persents = (((i + 1) * 100) / packets);
		if (!(persents % 10) && persents > lastPersents) {
			std::cout
				<< "\treceived "
				<< (i + 1) << " of the " << packets
				<< " (" << persents << "%)"
				<< std::endl;
			lastPersents = persents;
		}
	}

	ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicBay, true));
	ASSERT_NO_THROW(GetClient().Send(testing::clientMagicBay));
	ASSERT_NO_THROW(GetClient().Disconnect());

	EXPECT_TRUE(closeControlConnection->WaitDisconnect());

	result = true;

}

void ConnectClient::DoSeveralConnetionsTest(bool &result) {

	const testing::ConnectionsNumber connectionsNumber = 400;
	const testing::PacketsNumber packetsNumber = 10;
	const testing::PacketSize packetSize = 128;
		
	ASSERT_TRUE(Connect(testing::serverMagicSeveralConnectionsMode, false));
	ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicHello, true));

	ASSERT_NO_THROW(GetClient().Send(testing::clientMagicBegin));
	ASSERT_NO_THROW(GetClient().SendVal(connectionsNumber));
	ASSERT_NO_THROW(GetClient().Send(testing::clientMagicEnd));

	typedef std::list<boost::shared_ptr<TestUtil::Client>> Connections;
	Connections connections;

	auto lastPersents = 0;
	for (testing::ConnectionsNumber i = 1; i <= connectionsNumber; ++i) {

		boost::shared_ptr<TestUtil::Client> connection(CreateConnection());
		ASSERT_TRUE(Connect(*connection, false));
		ASSERT_NO_THROW(connection->SendVal(i));
		ASSERT_NO_THROW(connection->Send(testing::serverMagicSubConnectionMode));
		ASSERT_TRUE(connection->WaitAndTakeData(testing::serverMagicHello, false))
			<< "Failed to receive HELLO for sub connection #" << i << ".";
		testing::ConnectionsNumber remoteI = 0;
		ASSERT_NO_THROW(
				remoteI = connection->WaitAndTakeData<testing::ConnectionsNumber>(true))
			<< "Failed to receive sub connection number for sub connection #" << i << ".";
		ASSERT_EQ(i, remoteI);
		connections.push_back(connection);

		const auto persents = (((i + 1) * 100) / connectionsNumber);
		if (!(persents % 10) && persents > lastPersents) {
			std::cout
				<< "\taccepted "
				<< (i + 1) << " of the " << connectionsNumber
				<< " (" << persents << "%)"
				<< std::endl;
			lastPersents = persents;
		}

	}

	const std::auto_ptr<const TestUtil::Client> closeControlConnection(
		CreateCloseControlConnection());

	ASSERT_NO_THROW(GetClient().Send(testing::clientMagicBegin));
	ASSERT_NO_THROW(GetClient().SendVal(packetsNumber));
	ASSERT_NO_THROW(GetClient().Send(testing::clientMagicEnd));

	for (size_t i = 0; i < 2; ++i) {
		for (testing::PacketsNumber i = 0; i < packetsNumber; ++i) {
			testing::ConnectionsNumber connectionNumber = 1;
			foreach (auto &connection, connections) {
				ASSERT_NO_THROW(connection->SendVal(connectionNumber));
				ASSERT_TRUE(SendTestPacket(*connection, packetSize, .5));
				ASSERT_TRUE(
					connection->WaitAndTakeData(
						testing::serverMagicOk,
						false));
				testing::ConnectionsNumber remoteI = 0;
				ASSERT_NO_THROW(
					remoteI = connection->WaitAndTakeData<testing::ConnectionsNumber>(false));
				ASSERT_EQ(connectionNumber, remoteI);
				ASSERT_TRUE(ReceiveTestPacket(*connection));
				ASSERT_NO_THROW(connection->Send(testing::clientMagicOk));
				++connectionNumber;
			}
		}
	}

	{
		testing::ConnectionsNumber i = 1;
		foreach (auto &connection, connections) {
			const bool isActiveDisconnect = !!(i % 2);
			if (isActiveDisconnect) {
				ASSERT_NO_THROW(connection->SendVal(i));
				ASSERT_NO_THROW(connection->Send(testing::clientMagicBay));
				ASSERT_EQ(
						i,
						connection->WaitAndTakeData<testing::ConnectionsNumber>(false))
					<< "Failed to receive server sub connection number from sub connection #" << i << " (active disconnect).";
				ASSERT_TRUE(connection->WaitAndTakeData(testing::serverMagicBay, true))
					<< "Failed to receive server BAY from sub connection #" << i << " (active disconnect).";
				EXPECT_TRUE(connection->IsConnected());
				ASSERT_NO_THROW(connection->Disconnect());
			} else {
				ASSERT_EQ(
						i,
						connection->WaitAndTakeData<testing::ConnectionsNumber>(false))
					<< "Failed to receive server sub connection number from sub connection #" << i << " (passive disconnect).";
				ASSERT_TRUE(connection->WaitAndTakeData(testing::serverMagicBay, true))
					<< "Failed to receive server BAY from sub connection #" << i << " (passive disconnect).";
				ASSERT_NO_THROW(connection->SendVal(i));
				ASSERT_NO_THROW(connection->Send(testing::clientMagicBay));
				EXPECT_TRUE(connection->WaitDisconnect());
			}
			++i;
		}
		connections.clear();
	}

	ASSERT_NO_THROW(GetClient().Send(testing::clientMagicBay));
	ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicBay, true));
	EXPECT_TRUE(GetClient().IsConnected());
	ASSERT_NO_THROW(GetClient().Disconnect());

	EXPECT_TRUE(closeControlConnection->WaitDisconnect());

	result = true;

}
