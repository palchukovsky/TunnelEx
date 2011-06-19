/**************************************************************************
 *   Created: 2011/05/30 22:43
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "Common.hpp"
#include "TestUtils/InetServer.hpp"

namespace pt = boost::posix_time;

namespace {

	class TcpServer : public testing::Test  {
	
	public:
	
		virtual ~TcpServer() {
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
			m_server.reset(new TestUtil::TcpServer(testing::tcpServerPort));
			m_server->SetWaitTime(testing::defaultClientWaitTime);
			const bool waitResult = m_server->WaitConnect(1, true);
			assert(waitResult);
			UseUnused(waitResult);
		}
		
		virtual void TearDown() {
			m_server.reset();
		}

	protected:

		void SendTestPacket(size_t connection, testing::PacketSize size) {
			ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicBegin));
			ASSERT_NO_THROW(m_server->SendVal(connection, size));
			TestUtil::Buffer packet;
			boost::crc_32_type crc;
			testing::GeneratePacket(size, packet, crc);
			ASSERT_NO_THROW(m_server->Send(connection, packet));
			ASSERT_NO_THROW(m_server->SendVal(connection, crc.checksum()));
			ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicEnd));
		}

		void ReceiveTestPacket(size_t connection) {
			testing::PacketSize size = 0;
			ASSERT_TRUE(
				m_server->WaitAndTakeData(connection, testing::clientMagicBegin, false));
			ASSERT_NO_THROW(
				size = m_server->WaitAndTakeData<testing::PacketSize>(connection, false));
			EXPECT_GT(size, 0);
			TestUtil::Buffer packet;
			ASSERT_NO_THROW(
				packet = m_server->WaitAndTakeAnyData(connection, size, false));
			boost::crc_32_type realCrc;
			testing::Calc(packet, realCrc);
			boost::crc_32_type::value_type remoteCrc;
			ASSERT_NO_THROW(
				remoteCrc = m_server->WaitAndTakeData<boost::crc_32_type::value_type>(connection, false));
			EXPECT_EQ(realCrc.checksum(), remoteCrc);
			ASSERT_TRUE(
				m_server->WaitAndTakeData(connection, testing::clientMagicEnd, true));
		}

	protected:

		std::auto_ptr<TestUtil::Server> m_server;

	};

	////////////////////////////////////////////////////////////////////////////////

	TEST_F(TcpServer, DataExchangeActive) {

		ASSERT_EQ(size_t(1), m_server->GetNumberOfAcceptedConnections(false));
		const size_t connection = 0;
		
		ASSERT_TRUE(
			m_server->WaitAndTakeData(connection, testing::clientMagicHello, true));
		ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicHello));
		ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicActiveMode));

		const testing::PacketsNumber packets = 100;
		ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicBegin));
		ASSERT_NO_THROW(m_server->SendVal(connection, packets));
		ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicEnd));

		for (testing::PacketsNumber i = 0; i < packets; ++i) {
			SendTestPacket(connection, 128);
			ASSERT_TRUE(
				m_server->WaitAndTakeData(connection, testing::clientMagicOk, false));
			ReceiveTestPacket(connection);
			ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicOk));
		}

		ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicBay));
		ASSERT_TRUE(
			m_server->WaitAndTakeData(connection, testing::clientMagicBay, true));
		EXPECT_EQ(0, m_server->GetReceivedSize(connection));
		m_server->CloseConnection(connection);

	}

	TEST_F(TcpServer, DataExchangePassive) {

		ASSERT_EQ(size_t(1), m_server->GetNumberOfAcceptedConnections(false));
		const size_t connection = 0;
		
		ASSERT_TRUE(
			m_server->WaitAndTakeData(connection, testing::clientMagicHello, true));
		ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicHello));
		ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicPassiveMode));

		testing::PacketsNumber packets = 0;
		ASSERT_TRUE(
			m_server->WaitAndTakeData(connection, testing::clientMagicBegin, false));
		ASSERT_NO_THROW(
			packets = m_server->WaitAndTakeData<testing::PacketsNumber>(connection, false));
		ASSERT_GT(packets, 0);
		ASSERT_TRUE(
			m_server->WaitAndTakeData(connection, testing::clientMagicEnd, false));

		for (testing::PacketsNumber i = 0; i < packets; ++i) {
			ReceiveTestPacket(connection);
			ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicOk));
			SendTestPacket(connection, 128);
			ASSERT_TRUE(
				m_server->WaitAndTakeData(connection, testing::clientMagicOk, false));
		}

		ASSERT_TRUE(
			m_server->WaitAndTakeData(connection, testing::clientMagicBay, true));
		ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicBay));
		EXPECT_EQ(0, m_server->GetReceivedSize(connection));
		ASSERT_TRUE(m_server->WaitDisconnect(connection));

	}

}
