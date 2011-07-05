/**************************************************************************
 *   Created: 2011/06/27 0:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__UdpClientTest_cpp__1106270010
#define INCLUDED_FILE__TUNNELEX__UdpClientTest_cpp__1106270010

#include "Prec.h"
#include "Client.hpp"
#include "TestUtils/InetClient.hpp"

namespace {

	class UdpClient : public testing::Client {
	
	public:
	
		virtual ~UdpClient() {
			//...//
		}
	
	protected:

		virtual std::auto_ptr<TestUtil::Client> CreateClient() const {
			std::auto_ptr<TestUtil::Client> result(
				new TestUtil::UdpClient("localhost", testing::udpServerPort));
			return result;
		}

	};

	////////////////////////////////////////////////////////////////////////////////

	TEST_F(UdpClient, DataExchangeActive) {

		Connect(testing::serverMagicPassiveMode, false);
		ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicHello, true));

		const testing::PacketsNumber packets = 100;
		ASSERT_NO_THROW(GetClient().Send(testing::clientMagicBegin));
		ASSERT_NO_THROW(GetClient().SendVal(packets));
		ASSERT_NO_THROW(GetClient().Send(testing::clientMagicEnd));

		for (testing::PacketsNumber i = 0; i < packets; ++i) {
			SendTestPacket(128);
			ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicOk, false));
			ReceiveTestPacket();
			ASSERT_NO_THROW(GetClient().Send(testing::clientMagicOk));
		}

		ASSERT_NO_THROW(GetClient().Send(testing::clientMagicBay));
		ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicBay, true));
		EXPECT_EQ(0, GetClient().GetReceivedSize());
		EXPECT_TRUE(GetClient().IsConnected());
		ASSERT_NO_THROW(GetClient().Disconnect());

	}

	TEST_F(UdpClient, DataExchangePassive) {

		Connect(testing::serverMagicActiveMode, false);
		ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicHello, false));

		ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicBegin, false));
		testing::PacketsNumber packets = 0;
		ASSERT_NO_THROW(
			packets = GetClient().WaitAndTakeData<testing::PacketsNumber>(false));
		ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicEnd, false));
		EXPECT_GT(packets, testing::PacketsNumber(0));

		for (testing::PacketsNumber i = 0; i < packets; ++i) {
			ReceiveTestPacket();
			ASSERT_NO_THROW(GetClient().Send(testing::clientMagicOk));
			SendTestPacket(128);
			ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicOk, false));
		}

		ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicBay, true));
		ASSERT_NO_THROW(GetClient().Send(testing::clientMagicBay));
		EXPECT_EQ(0, GetClient().GetReceivedSize());
		EXPECT_TRUE(GetClient().WaitDisconnect());
		EXPECT_EQ(0, GetClient().GetReceivedSize());

	}

	TEST_F(UdpClient, DISABLED_DataExchangeOneWayActive) {

		const testing::PacketsNumber packets = 1024 * 3;
		const testing::PacketSize packetSize = 1024;

		Connect(testing::serverMagicOneWayPassiveMode, false);
		ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicHello, true));

		ASSERT_NO_THROW(GetClient().Send(testing::clientMagicBegin));
		ASSERT_NO_THROW(GetClient().SendVal(packets));
		ASSERT_NO_THROW(GetClient().Send(testing::clientMagicEnd));

		for (testing::PacketsNumber i = 0; i < packets; ++i) {
			SendTestPacket(packetSize);
		}

		ASSERT_NO_THROW(GetClient().Send(testing::clientMagicBay));
		const auto waitTime = GetClient().GetWaitTime();
		GetClient().SetWaitTime(waitTime * 3);
		ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicBay, true));
		GetClient().SetWaitTime(waitTime);
		ASSERT_NO_THROW(GetClient().Disconnect());

	}

	TEST_F(UdpClient, DISABLED_DataOneWayExchangePassive) {

		Connect(testing::serverMagicOneWayActiveMode, false);
		ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicHello, false));

		ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicBegin, false));
		testing::PacketsNumber packets = 0;
		ASSERT_NO_THROW(
			packets = GetClient().WaitAndTakeData<testing::PacketsNumber>(false));
		ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicEnd, false));
		EXPECT_GT(packets, testing::PacketsNumber(0));

		int lastPersents = 0;
		for (testing::PacketsNumber i = 0; i < packets; ++i) {
			ReceiveTestPacket();
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

		ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicBay, true));
		ASSERT_NO_THROW(GetClient().Send(testing::clientMagicBay));
		ASSERT_NO_THROW(GetClient().Disconnect());

	}

	TEST_F(UdpClient, DISABLED_SeveralConnetions) {

		const testing::ConnectionsNumber connectionsNumber = 100;
		const testing::PacketsNumber packetsNumber = 10;
		const testing::PacketSize packetSize = 128;
		
		Connect(testing::serverMagicSeveralConnectionsMode, false);
		ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicHello, true));

		ASSERT_NO_THROW(GetClient().Send(testing::clientMagicBegin));
		ASSERT_NO_THROW(GetClient().SendVal(connectionsNumber));
		ASSERT_NO_THROW(GetClient().Send(testing::clientMagicEnd));

		typedef std::list<boost::shared_ptr<TestUtil::Client>> Connections;
		Connections connections;
		for (size_t i = 0; i < connectionsNumber; ++i) {
			boost::shared_ptr<TestUtil::Client> connection(CreateConnection());
			Connect(*connection, false);
			ASSERT_NO_THROW(connection->Send(testing::serverMagicSubConnectionMode));
			ASSERT_TRUE(connection->WaitAndTakeData(testing::serverMagicHello, true))
				<< "Failed to receive HELLO for connection #" << (i + 1) << ".";
			connections.push_back(connection);
		}

		ASSERT_NO_THROW(GetClient().Send(testing::clientMagicBegin));
		ASSERT_NO_THROW(GetClient().SendVal(packetsNumber));
		ASSERT_NO_THROW(GetClient().Send(testing::clientMagicEnd));

		for (size_t i = 0; i < 2; ++i) {
			for (testing::PacketsNumber i = 0; i < packetsNumber; ++i) {
				foreach (auto &connection, connections) {
					SendTestPacket(*connection, packetSize);
					ASSERT_TRUE(
						connection->WaitAndTakeData(
							testing::serverMagicOk,
							false));
					ReceiveTestPacket(*connection);
					ASSERT_NO_THROW(connection->Send(testing::clientMagicOk));
				}
			}
		}

		foreach (auto &connection, connections) {
			ASSERT_NO_THROW(connection->Send(testing::clientMagicBay));
			ASSERT_TRUE(connection->WaitAndTakeData(testing::serverMagicBay, true));
			EXPECT_TRUE(connection->IsConnected());
			ASSERT_NO_THROW(connection->Disconnect());
		}
		connections.clear();

		ASSERT_NO_THROW(GetClient().Send(testing::clientMagicBay));
		ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicBay, true));
		EXPECT_TRUE(GetClient().IsConnected());
		ASSERT_NO_THROW(GetClient().Disconnect());

	}

}

#endif // INCLUDED_FILE__TUNNELEX__UdpClientTest_cpp__1106270010
