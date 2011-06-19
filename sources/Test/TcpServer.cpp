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
			m_server->SetWaitTime(testing::defaultDataWaitTime);
			const bool waitResult = m_server->WaitConnect(1, true);
			assert(waitResult);
			UseUnused(waitResult);
		}
		
		virtual void TearDown() {
			m_server.reset();
		}

	protected:

		void TestActiveServer(size_t connection) {

			const testing::PacketsNumber packets = 100;
			ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicBegin));
			ASSERT_NO_THROW(m_server->SendVal(connection, packets));
			ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicEnd));

			for (testing::PacketsNumber i = 0; i < packets; ++i) {
				SendTestPacket(connection, 512);
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

		void TestPassiveServer(size_t connection) {

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
				SendTestPacket(connection, 512);
				ASSERT_TRUE(
					m_server->WaitAndTakeData(connection, testing::clientMagicOk, false));
			}

			ASSERT_TRUE(
				m_server->WaitAndTakeData(connection, testing::clientMagicBay, true));
			ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicBay));
			EXPECT_EQ(0, m_server->GetReceivedSize(connection));
			ASSERT_TRUE(m_server->WaitDisconnect(connection));

		}

	private:

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

	TEST_F(TcpServer, DataExchange) {

		ASSERT_EQ(size_t(1), m_server->GetNumberOfAcceptedConnections(false));
		const size_t connection = 0;

		ASSERT_TRUE(
			m_server->WaitAndTakeData(connection, testing::clientMagicHello, true));
		ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicHello));
		
		enum ServerMode {
			SERVER_MODE_ACTIVE = 1,
			SERVER_MODE_PASSIVE = 2,
			SERVER_MODE_ONE_WAY_ACTIVE = 3,
			SERVER_MODE_ONE_WAY_PASSIVE = 4
		} serverMode = SERVER_MODE_ACTIVE;
		
		{
			typedef std::list<const std::string *> Modes;
			Modes modes;
			modes.push_back(&testing::serverMagicActiveMode);
			modes.push_back(&testing::serverMagicPassiveMode);
			modes.push_back(&testing::serverMagicOneWayActiveMode);
			modes.push_back(&testing::serverMagicOneWayPassiveMode);
			const int serverModePos
				= m_server->WaitAndTakeData(connection, modes, false);
			ASSERT_GT(serverModePos, 0);
			ASSERT_LT(serverModePos, 5);
			serverMode = ServerMode(serverModePos);
			Modes::const_iterator serverModeItPos = modes.begin();
			std::advance(serverModeItPos, serverModePos - 1);
			std::cout << **serverModeItPos << " (" << serverModePos << ")";
		}

		switch (serverMode) {
			case  SERVER_MODE_ACTIVE:
				std::cout << " - server active mode" << std::endl;
				ASSERT_EQ(0, m_server->GetReceivedSize(connection));
				TestActiveServer(connection);
				break;
			case SERVER_MODE_PASSIVE:
				std::cout << " - server passive mode" << std::endl;
				TestPassiveServer(connection);
				break;
			case SERVER_MODE_ONE_WAY_ACTIVE:
			case SERVER_MODE_ONE_WAY_PASSIVE:
				FAIL() << "Doesn't implemented yet.";
				break;
		}

	}

}
