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
			m_client->SetWaitTime(testing::defaultDataWaitTime);
		}
		
		virtual void TearDown() {
			m_client.reset();
		}

	protected:

		void Connect(const std::string &mode) {
			ASSERT_TRUE(m_client->WaitConnect());
			ASSERT_NO_THROW(m_client->Send(testing::clientMagicHello));
			ASSERT_TRUE(m_client->WaitAndTakeData(testing::serverMagicHello, false));
			ASSERT_NO_THROW(m_client->Send(mode));
		}

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

	TEST_F(TcpClient, DataExchangeActive) {

		Connect(testing::serverMagicPassiveMode);

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

		Connect(testing::serverMagicActiveMode);

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

}
