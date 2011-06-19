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
			m_client.reset(new TestUtil::TcpClient("localhost", testing::tcpServerPort));
			m_client->SetWaitTime(testing::defaultClientWaitTime);
		}
		
		virtual void TearDown() {
			m_client.reset();
		}

	protected:

		void TestActiveServer() {

			ASSERT_TRUE(m_client->WaitAndTakeData(testing::serverMagicBegin, false));
			testing::PacketsNumber packets = 0;
			ASSERT_NO_THROW(
				packets = m_client->WaitAndTakeData<testing::PacketsNumber>(false));
			ASSERT_TRUE(m_client->WaitAndTakeData(testing::serverMagicEnd, false));
			EXPECT_GT(packets, 0);

			for (testing::PacketsNumber i = 0; i < packets; ++i) {
				ReceiveTestPacket();
				ASSERT_NO_THROW(m_client->Send(testing::clientMagicOk));
				SendTestPacket(128);
				ASSERT_TRUE(m_client->WaitAndTakeData(testing::serverMagicOk, false));
			}

			ASSERT_TRUE(m_client->WaitAndTakeData(testing::serverMagicBay, true));
			ASSERT_NO_THROW(m_client->Send(testing::clientMagicBay));
			EXPECT_EQ(0, m_client->GetReceived().size());
			EXPECT_TRUE(m_client->WaitDisconnect());
			EXPECT_EQ(0, m_client->GetReceived().size());

		}

		void TestPassiveServer() {

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

	private:

		void SendTestPacket(testing::PacketSize size) {
			ASSERT_NO_THROW(m_client->Send(testing::clientMagicBegin));
			ASSERT_NO_THROW(m_client->SendVal(size));
			TestUtil::Buffer packet;
			boost::crc_32_type crc;
			testing::GeneratePacket(size, packet, crc);
			assert(size == packet.size());
			ASSERT_NO_THROW(m_client->Send(packet));
			ASSERT_NO_THROW(m_client->SendVal(crc.checksum()));
			ASSERT_NO_THROW(m_client->Send(testing::clientMagicEnd));
		}

		void ReceiveTestPacket() {
			ASSERT_TRUE(
				m_client->WaitAndTakeData(testing::serverMagicBegin, false));
			testing::PacketSize size = 0;
			ASSERT_NO_THROW(size = m_client->WaitAndTakeData<testing::PacketSize>(false));
			EXPECT_GT(size, 0);
			TestUtil::Buffer packet;
			ASSERT_NO_THROW(packet = m_client->WaitAndTakeAnyData(size, false));
			boost::crc_32_type realCrc;
			testing::Calc(packet, realCrc);
			boost::crc_32_type::value_type remoteCrc;
			ASSERT_NO_THROW(
				remoteCrc = m_client->WaitAndTakeData<boost::crc_32_type::value_type>(false));
			EXPECT_EQ(realCrc.checksum(), remoteCrc);
			ASSERT_TRUE(
				m_client->WaitAndTakeData(testing::serverMagicEnd, true));
		}

	protected:

		std::auto_ptr<TestUtil::Client> m_client;

	};

	////////////////////////////////////////////////////////////////////////////////

	TEST_F(TcpClient, DataExchange) {

		ASSERT_TRUE(m_client->WaitConnect());
		ASSERT_NO_THROW(m_client->Send(testing::clientMagicHello));
		ASSERT_TRUE(m_client->WaitAndTakeData(testing::serverMagicHello, false));
		
		enum ServerMode {
			SERVER_MODE_ACTIVE = 1,
			SERVER_MODE_PASSIVE = 2,
			SERVER_MODE_ONE_WAY_ACTIVE = 3,
			SERVER_MODE_ONE_WAY_PASSIVE = 4
		} serverMode = SERVER_MODE_ACTIVE;
		
		{
			std::list<const std::string *> modes;
			modes.push_back(&testing::serverMagicActiveMode);
			modes.push_back(&testing::serverMagicPassiveMode);
			modes.push_back(&testing::serverMagicOneWayActiveMode);
			modes.push_back(&testing::serverMagicOneWayPassiveMode);
			const int serverModePos = m_client->WaitAndTakeData(modes, false);
			ASSERT_GT(serverModePos, 0);
			ASSERT_LT(serverModePos, 5);
			serverMode = ServerMode(serverModePos);
		}

		switch (serverMode) {
			case  SERVER_MODE_ACTIVE:
				TestActiveServer();
				break;
			case SERVER_MODE_PASSIVE:
				TestPassiveServer();
				break;
			case SERVER_MODE_ONE_WAY_ACTIVE:
			case SERVER_MODE_ONE_WAY_PASSIVE:
				FAIL() << "Doesn't implemented yet.";
				break;
		}

	}

}
