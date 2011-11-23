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

namespace {

	class UdpServer : public testing::Test  {
	
	public:
	
		virtual ~UdpServer() {
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
			m_server.reset(
				new TestUtil::UdpServer(
					testing::udpServerPort,
					testing::defaultDataWaitTime));
			const bool waitResult = m_server->WaitConnect(1, true);
			assert(waitResult);
			UseUnused(waitResult);
		}
		
		virtual void TearDown() {
			m_server.reset();
		}

	protected:

		bool TestActiveServer(size_t connection) {
			bool result = true;
			DoTestActiveServer(connection, result);
			return result;
		}

		bool TestOneWayActiveServer(size_t connection) {
			bool result = true;
			DoTestOneWayActiveServer(connection, result);
			return result;
		}
		
		bool TestOneWayPassiveServer(size_t connection) {
			bool result = true;
			DoTestOneWayPassiveServer(connection, result);
			return result;
		}
		
		bool TestSeveralConnections(size_t connection) {
			bool result = true;
			DoTestSeveralConnections(connection, result);
			return result;
		}

		void DoTestActiveServer(size_t connection, bool &result) {

			const testing::PacketsNumber packets = 1000;

			ASSERT_NO_THROW(m_server->SendVal(connection, packets));
			ASSERT_TRUE(
					m_server->WaitAndTakeData(connection, testing::clientMagicOk, true));

			for (testing::PacketsNumber i = 0; i < packets; ++i) {
				ASSERT_TRUE(SendTestPacket(connection, 128, true, .5));
				ASSERT_TRUE(
					m_server->WaitAndTakeData(connection, testing::clientMagicOk, true));
				ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicOk));
				ASSERT_TRUE(ReceiveTestPacket(connection, true))
					<< "Failed to receive test packet #" << (i + 1) << ".";
				ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicOk));
				ASSERT_TRUE(
					m_server->WaitAndTakeData(connection, testing::clientMagicOk, true));
			}

			ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicBay));
			ASSERT_TRUE(
				m_server->WaitAndTakeData(connection, testing::clientMagicBay, true));
			EXPECT_EQ(0, m_server->GetReceivedSize(connection));

			result = true;

		}

		void DoTestOneWayActiveServer(size_t connection, bool &result) {

			const testing::PacketsNumber packets = 5000;

			ASSERT_NO_THROW(m_server->SendVal(connection, packets));

			for (testing::PacketsNumber i = 0; i < packets; ++i) {
				ASSERT_TRUE(SendTestPacket(connection, 128, false, 0.95));
				if (!(i % 25)) {
					ASSERT_TRUE(
						m_server->WaitAndTakeData(connection, testing::clientMagicOk, true))
							<< " at packet " << i;
				}
			}

			EXPECT_NO_THROW(m_server->Send(connection, testing::serverMagicBay));
			EXPECT_EQ(0, m_server->GetReceivedSize(connection));

			result = true;

		}

		void DoTestOneWayPassiveServer(size_t connection, bool &result) {

			ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicOk));

			testing::PacketsNumber packets = 0;
			ASSERT_NO_THROW(
				packets = m_server->WaitAndTakeData<testing::PacketsNumber>(connection, false));
			EXPECT_GT(packets, testing::PacketsNumber(0));

			for (testing::PacketsNumber i = 0; i < packets; ++i) {
				ASSERT_TRUE(ReceiveTestPacket(connection, false));
				if (!(i % 25)) {
					ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicOk))
						<< " at packet " << i;
				}
			}

			EXPECT_TRUE(m_server->WaitAndTakeData(connection, testing::clientMagicBay, true));
			EXPECT_EQ(0, m_server->GetReceivedSize(connection));

			result = true;

		}

		void DoTestSeveralConnections(size_t mainConnection, bool &result) {

			const testing::PacketSize packetSize = 128;

			ASSERT_NO_THROW(m_server->Send(mainConnection, testing::serverMagicOk));

			testing::ConnectionsNumber connectionsNumber = 0;
			ASSERT_NO_THROW(
				connectionsNumber = m_server->WaitAndTakeData<testing::ConnectionsNumber>(
					mainConnection,
					true));
			EXPECT_GT(connectionsNumber, testing::ConnectionsNumber(0));
			ASSERT_NO_THROW(m_server->Send(mainConnection, testing::serverMagicOk));

			
			for (testing::ConnectionsNumber i = 1; i <= connectionsNumber; ++i) {
				const size_t connection = i + mainConnection;
				ASSERT_TRUE(m_server->WaitConnect(connection + 1, false))
					<< "Failed to accept sub connection #" << i << ".";
				ASSERT_TRUE(
					m_server->WaitAndTakeData(connection, testing::clientMagicHello, true));
				ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicHello));
				ASSERT_TRUE(
					m_server->WaitAndTakeData(
						connection,
						testing::serverMagicSubConnectionMode,
						true));
			}

			for (testing::ConnectionsNumber i = 1; i <= connectionsNumber; ++i) {
				ASSERT_NO_THROW(m_server->Send(i + mainConnection, testing::serverMagicOk));
			}

			for (testing::ConnectionsNumber i = 1; i <= connectionsNumber; ++i) {
				const size_t connection = i + mainConnection;
				ASSERT_EQ(
					i,
					m_server->WaitAndTakeData<testing::ConnectionsNumber>(connection, true));
				ASSERT_NO_THROW(m_server->SendVal(connection, testing::ConnectionsNumber(i)));
			}

			testing::PacketsNumber packetsNumber = 0;
			ASSERT_NO_THROW(
				packetsNumber = m_server->WaitAndTakeData<testing::PacketsNumber>(
					mainConnection,
					true));

			for (size_t i = 0; i < 2; ++i) {
				
				for (testing::PacketsNumber i = 0; i < packetsNumber; ++i) {

					
					for (testing::ConnectionsNumber i = 1; i <= connectionsNumber; ++i) {
						const size_t connection = i + mainConnection;
						EXPECT_EQ(
							i,
							m_server->WaitAndTakeData<testing::ConnectionsNumber>(connection, true));
						ASSERT_NO_THROW(
							m_server->SendVal(connection, testing::ConnectionsNumber(i)));
					}

					for (testing::ConnectionsNumber i = 1; i <= connectionsNumber; ++i) {
						const size_t connection = i + mainConnection;
						ASSERT_TRUE(ReceiveTestPacket(connection, true));
						ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicOk));
						ASSERT_TRUE(
							m_server->WaitAndTakeData(connection, testing::clientMagicOk, true));
					}

					for (testing::ConnectionsNumber i = 1; i <= connectionsNumber; ++i) {
						const size_t connection = i + mainConnection;
						ASSERT_TRUE(SendTestPacket(connection, 128, true, .5));
						ASSERT_TRUE(
							m_server->WaitAndTakeData(connection, testing::clientMagicOk, true));
						ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicOk));
					}

				}
			}

			ASSERT_TRUE(
				m_server->WaitAndTakeData(
					mainConnection,
					testing::clientMagicBay,
					true));
			ASSERT_NO_THROW(m_server->Send(mainConnection, testing::serverMagicBay));
			EXPECT_EQ(0, m_server->GetReceivedSize(mainConnection));

			result = true;

		}

	private:

		bool SendTestPacket(
					size_t connection,
					testing::PacketSize size,
					bool answers,
					double widthRatio)
				const  {
			bool result = true;
			DoSendTestPacket(connection, size, answers, widthRatio, result);
			return result;
		}

		void DoSendTestPacket(
					size_t connection,
					testing::PacketSize size,
					bool answers,
					double widthRatio,
					bool &result)
				const  {
			
			std::auto_ptr<TestUtil::Buffer> packet(new TestUtil::Buffer);
			boost::crc_32_type crc;
			testing::GeneratePacket(*packet, crc, size - (size * widthRatio), size + (size * widthRatio));
			
			ASSERT_NO_THROW(m_server->SendVal(connection, testing::PacketSize(packet->size())));
			if (answers) {
				ASSERT_TRUE(m_server->WaitAndTakeData(connection, testing::clientMagicOk, true));
			}
			ASSERT_NO_THROW(m_server->Send(connection, packet));
			if (answers) {
				ASSERT_TRUE(m_server->WaitAndTakeData(connection, testing::clientMagicOk, true));
			}
			ASSERT_NO_THROW(m_server->SendVal(connection, crc.checksum()));

			result = true;

		}

		bool ReceiveTestPacket(size_t connection, bool answers) const {
			bool result = false;
			DoReceiveTestPacket(connection, answers, result);
			return result;
		}

		void DoReceiveTestPacket(size_t connection, bool answers, bool &result) const {
			testing::PacketSize size = 0;
			ASSERT_NO_THROW(
				size = m_server->WaitAndTakeData<testing::PacketSize>(connection, answers));
			EXPECT_GT(size, 0);
			if (answers) {
				ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicOk));
			}
			TestUtil::Buffer packet;
			ASSERT_NO_THROW(m_server->WaitAndTakeAnyData(connection, size, answers, packet));
			boost::crc_32_type realCrc;
			testing::Calc(packet, realCrc);
			boost::crc_32_type::value_type remoteCrc;
			if (answers) {
				ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicOk));
			}
			ASSERT_NO_THROW(
				remoteCrc = m_server->WaitAndTakeData<boost::crc_32_type::value_type>(connection, answers));
			EXPECT_EQ(realCrc.checksum(), remoteCrc) << " at connection " << connection;
			result = true;
		}

	protected:

		std::auto_ptr<TestUtil::Server> m_server;

	};

	////////////////////////////////////////////////////////////////////////////////

	TEST_F(UdpServer, Any) {

		ASSERT_EQ(size_t(1), m_server->GetNumberOfAcceptedConnections(false));
		const size_t connection = 0;

		ASSERT_TRUE(
			m_server->WaitAndTakeData(connection, testing::clientMagicHello, true));
		ASSERT_NO_THROW(m_server->Send(connection, testing::serverMagicHello));
		
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
			ASSERT_NO_THROW(serverModePos = m_server->WaitAndTakeData(connection, modes, true));
			ASSERT_GT(serverModePos, 0);
			ASSERT_LT(serverModePos, 6);
			serverMode = ServerMode(serverModePos);
			Modes::const_iterator serverModeItPos = modes.begin();
			std::advance(serverModeItPos, serverModePos - 1);
			std::cout << "\t" << **serverModeItPos << " (" << serverModePos << ")";
		}

		switch (serverMode) {
			case  SERVER_MODE_ACTIVE:
				std::cout << " - server active mode" << std::endl;
				ASSERT_TRUE(TestActiveServer(connection));
				break;
			case SERVER_MODE_PASSIVE:
				std::cout << " - server passive mode" << std::endl;
				FAIL() << "Doesn't implemented yet.";
				// TestPassiveServer(connection);
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
				TestSeveralConnections(connection);
				break;
			default:
				FAIL() << "Doesn't implemented yet.";
				break;
		}

	}

}
