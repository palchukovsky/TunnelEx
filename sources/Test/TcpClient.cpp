/**************************************************************************
 *   Created: 2011/05/30 23:13
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "TestUtils/InetClient.hpp"
#include "Common.hpp"

namespace pt = boost::posix_time;

namespace {

	class TcpClient : public testing::Test  {
	
	public:
	
		virtual ~TcpClient() {
			//...//
		}
	
	public:
		
		static void SetUpTestCase() {
			//...//
		}
		
		static void TearDownTestCase() {
			//...//
		}
		
		virtual void SetUp() {
			m_client = CreateConnection();
		}
		
		virtual void TearDown() {
			m_client.reset();
		}

	protected:

		std::auto_ptr<TestUtil::Client> CreateConnection() const {
			std::auto_ptr<TestUtil::Client> result(
				new TestUtil::TcpClient("localhost", testing::tcpServerPort));
			result->SetWaitTime(testing::defaultDataWaitTime);
			return result;
		}

		void Connect(const std::string &mode, bool infiniteTimeout) {
			Connect(*m_client, infiniteTimeout);
			ASSERT_NO_THROW(m_client->Send(mode));
		}

		void Connect(
					TestUtil::Client &client,
					bool infiniteTimeout)
				const {
			ASSERT_TRUE(client.WaitConnect(infiniteTimeout));
			ASSERT_NO_THROW(client.Send(testing::clientMagicHello));
		}

		void SendTestPacket(testing::PacketSize size) {
			SendTestPacket(*m_client, size);
		}

		void SendTestPacket(
					TestUtil::Client &client,
					testing::PacketSize size)
				const  {
			
			ASSERT_NO_THROW(client.Send(testing::clientMagicBegin));
			
			TestUtil::Buffer packet;
			boost::crc_32_type crc;
			testing::GeneratePacket(packet, crc, size * 0.5, size * 1.5);
			
			ASSERT_NO_THROW(client.SendVal(testing::PacketSize(packet.size())));
			ASSERT_NO_THROW(client.Send(packet));
			ASSERT_NO_THROW(client.SendVal(crc.checksum()));
			ASSERT_NO_THROW(client.Send(testing::clientMagicEnd));

		}

		void ReceiveTestPacket() {
			ReceiveTestPacket(*m_client);
		}

		void ReceiveTestPacket(TestUtil::Client &client) const {
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
		}

	protected:

		std::auto_ptr<TestUtil::Client> m_client;

	};

	////////////////////////////////////////////////////////////////////////////////

	TEST_F(TcpClient, DataExchangeActive) {

		Connect(testing::serverMagicPassiveMode, false);
		ASSERT_TRUE(m_client->WaitAndTakeData(testing::serverMagicHello, true));

		const testing::PacketsNumber packets = 100;
		ASSERT_NO_THROW(m_client->Send(testing::clientMagicBegin));
		ASSERT_NO_THROW(m_client->SendVal(packets));
		ASSERT_NO_THROW(m_client->Send(testing::clientMagicEnd));

		for (testing::PacketsNumber i = 0; i < packets; ++i) {
			SendTestPacket(128);
			ASSERT_TRUE(m_client->WaitAndTakeData(testing::serverMagicOk, false));
			ReceiveTestPacket();
			ASSERT_NO_THROW(m_client->Send(testing::clientMagicOk));
		}

		ASSERT_NO_THROW(m_client->Send(testing::clientMagicBay));
		ASSERT_TRUE(m_client->WaitAndTakeData(testing::serverMagicBay, true));
		EXPECT_EQ(0, m_client->GetReceivedSize());
		EXPECT_TRUE(m_client->IsConnected());
		ASSERT_NO_THROW(m_client->Disconnect());

	}

	TEST_F(TcpClient, DataExchangePassive) {

		Connect(testing::serverMagicActiveMode, false);
		ASSERT_TRUE(m_client->WaitAndTakeData(testing::serverMagicHello, false));

		ASSERT_TRUE(m_client->WaitAndTakeData(testing::serverMagicBegin, false));
		testing::PacketsNumber packets = 0;
		ASSERT_NO_THROW(
			packets = m_client->WaitAndTakeData<testing::PacketsNumber>(false));
		ASSERT_TRUE(m_client->WaitAndTakeData(testing::serverMagicEnd, false));
		EXPECT_GT(packets, testing::PacketsNumber(0));

		for (testing::PacketsNumber i = 0; i < packets; ++i) {
			ReceiveTestPacket();
			ASSERT_NO_THROW(m_client->Send(testing::clientMagicOk));
			SendTestPacket(128);
			ASSERT_TRUE(m_client->WaitAndTakeData(testing::serverMagicOk, false));
		}

		ASSERT_TRUE(m_client->WaitAndTakeData(testing::serverMagicBay, true));
		ASSERT_NO_THROW(m_client->Send(testing::clientMagicBay));
		EXPECT_EQ(0, m_client->GetReceivedSize());
		EXPECT_TRUE(m_client->WaitDisconnect());
		EXPECT_EQ(0, m_client->GetReceivedSize());

	}

	TEST_F(TcpClient, DataExchangeOneWayActive) {

		const testing::PacketsNumber packets = 1024 * 3;
		const testing::PacketSize packetSize = 1024;

		Connect(testing::serverMagicOneWayPassiveMode, false);
		ASSERT_TRUE(m_client->WaitAndTakeData(testing::serverMagicHello, true));

		ASSERT_NO_THROW(m_client->Send(testing::clientMagicBegin));
		ASSERT_NO_THROW(m_client->SendVal(packets));
		ASSERT_NO_THROW(m_client->Send(testing::clientMagicEnd));

		for (testing::PacketsNumber i = 0; i < packets; ++i) {
			SendTestPacket(packetSize);
		}

		ASSERT_NO_THROW(m_client->Send(testing::clientMagicBay));
		const auto waitTime = m_client->GetWaitTime();
		m_client->SetWaitTime(waitTime * 3);
		ASSERT_TRUE(m_client->WaitAndTakeData(testing::serverMagicBay, true));
		m_client->SetWaitTime(waitTime);
		ASSERT_NO_THROW(m_client->Disconnect());

	}

	TEST_F(TcpClient, DataOneWayExchangePassive) {

		Connect(testing::serverMagicOneWayActiveMode, false);
		ASSERT_TRUE(m_client->WaitAndTakeData(testing::serverMagicHello, false));

		ASSERT_TRUE(m_client->WaitAndTakeData(testing::serverMagicBegin, false));
		testing::PacketsNumber packets = 0;
		ASSERT_NO_THROW(
			packets = m_client->WaitAndTakeData<testing::PacketsNumber>(false));
		ASSERT_TRUE(m_client->WaitAndTakeData(testing::serverMagicEnd, false));
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

		ASSERT_TRUE(m_client->WaitAndTakeData(testing::serverMagicBay, true));
		ASSERT_NO_THROW(m_client->Send(testing::clientMagicBay));
		ASSERT_NO_THROW(m_client->Disconnect());

	}

	TEST_F(TcpClient, SeveralConnetions) {

		const testing::ConnectionsNumber connectionsNumber = 100;
		const testing::PacketsNumber packetsNumber = 10;
		const testing::PacketSize packetSize = 128;
		
		Connect(testing::serverMagicSeveralConnectionsMode, false);
		ASSERT_TRUE(m_client->WaitAndTakeData(testing::serverMagicHello, true));

		ASSERT_NO_THROW(m_client->Send(testing::clientMagicBegin));
		ASSERT_NO_THROW(m_client->SendVal(connectionsNumber));
		ASSERT_NO_THROW(m_client->Send(testing::clientMagicEnd));

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

		ASSERT_NO_THROW(m_client->Send(testing::clientMagicBegin));
		ASSERT_NO_THROW(m_client->SendVal(packetsNumber));
		ASSERT_NO_THROW(m_client->Send(testing::clientMagicEnd));

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

		ASSERT_NO_THROW(m_client->Send(testing::clientMagicBay));
		ASSERT_TRUE(m_client->WaitAndTakeData(testing::serverMagicBay, true));
		EXPECT_TRUE(m_client->IsConnected());
		ASSERT_NO_THROW(m_client->Disconnect());

	}

}
