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

		typedef testing::Client Base;
	
	public:
	
		virtual ~UdpClient() {
			//...//
		}
	
	protected:

		virtual void TearDown() {
			const auto waitTime = GetClient().GetWaitTime();
			Base::TearDown();
			boost::thread::sleep(boost::get_system_time() + waitTime);
		}

		virtual std::auto_ptr<TestUtil::Client> CreateClient(
					const boost::posix_time::time_duration &waitTime)
				const {
			std::auto_ptr<TestUtil::Client> result(
				new TestUtil::UdpClient(
					testing::udpServerHost,
					testing::udpServerPort,
					waitTime));
			return result;
		}

		virtual void DoConnect(
					TestUtil::Client &client,
					const std::string &mode,
					bool infiniteTimeout,
					bool &result)
				const {
			ASSERT_TRUE(Connect(client, infiniteTimeout));
			ASSERT_TRUE(client.WaitAndTakeData(testing::serverMagicHello, true));
			ASSERT_NO_THROW(client.Send(mode));
			result = true;
		}

		bool SendTestPacket(testing::PacketSize size, bool answers, double widthRatio) {
			return SendTestPacket(GetClient(), size, answers, widthRatio);
		}

		bool SendTestPacket(
					TestUtil::Client &client,
					testing::PacketSize size,
					bool answers,
					double widthRatio)
				const  {
			bool result = false;
			DoSendTestPacket(client, size, answers, widthRatio, result);
			return result;
		}

		void DoSendTestPacket(
					TestUtil::Client &client,
					testing::PacketSize size,
					bool answers,
					double widthRatio,
					bool &result)
				const  {
			
			std::auto_ptr<TestUtil::Buffer> packet(new TestUtil::Buffer);
			boost::crc_32_type crc;
			testing::GeneratePacket(*packet, crc, size - (size * widthRatio), size + (size * widthRatio));
			
			ASSERT_NO_THROW(client.SendVal(testing::PacketSize(packet->size())));
			if (answers) {
				ASSERT_TRUE(client.WaitAndTakeData(testing::serverMagicOk, true));
			}
			ASSERT_NO_THROW(client.Send(packet));
			if (answers) {
				ASSERT_TRUE(client.WaitAndTakeData(testing::serverMagicOk, true));
			}
			ASSERT_NO_THROW(client.SendVal(crc.checksum()));

			result = true;

		}

		bool ReceiveTestPacket(bool answers) {
			return ReceiveTestPacket(GetClient(), answers);
		}

		bool ReceiveTestPacket(TestUtil::Client &client, bool answers) const {
			bool result = false;
			DoReceiveTestPacket(client, answers, result);
			return result;
		}

		void DoReceiveTestPacket(TestUtil::Client &client, bool answers, bool &result) const {
			testing::PacketSize size = 0;
			ASSERT_NO_THROW(size = client.WaitAndTakeData<testing::PacketSize>(answers));
			EXPECT_GT(size, 0);
			if (answers) {
				ASSERT_NO_THROW(client.Send(testing::clientMagicOk));
			}
			TestUtil::Buffer packet;
			ASSERT_NO_THROW(client.WaitAndTakeAnyData(size, answers, packet));
			boost::crc_32_type realCrc;
			testing::Calc(packet, realCrc);
			boost::crc_32_type::value_type remoteCrc;
			if (answers) {
				ASSERT_NO_THROW(client.Send(testing::clientMagicOk));
			}
			ASSERT_NO_THROW(
				remoteCrc = client.WaitAndTakeData<boost::crc_32_type::value_type>(answers));
			EXPECT_EQ(realCrc.checksum(), remoteCrc);
			result = true;
		}

	};

	////////////////////////////////////////////////////////////////////////////////

	TEST_F(UdpClient, DataExchangePassive) {

		ASSERT_TRUE(Connect(testing::serverMagicActiveMode, false));

		testing::PacketsNumber packets = 0;
		ASSERT_NO_THROW(
			packets = GetClient().WaitAndTakeData<testing::PacketsNumber>(true));
		EXPECT_GT(packets, testing::PacketsNumber(0));
		ASSERT_NO_THROW(GetClient().Send(testing::clientMagicOk));

		for (testing::PacketsNumber i = 0; i < packets; ++i) {
			ASSERT_TRUE(ReceiveTestPacket(true));
			ASSERT_NO_THROW(GetClient().Send(testing::clientMagicOk));
			ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicOk, true));
			ASSERT_TRUE(SendTestPacket(128, true, .5));
			ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicOk, true));
			ASSERT_NO_THROW(GetClient().Send(testing::clientMagicOk));
		}

		ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicBay, true));
		ASSERT_NO_THROW(GetClient().Send(testing::clientMagicBay));
		EXPECT_EQ(0, GetClient().GetReceivedSize());

	}

	TEST_F(UdpClient, DataOneWayExchangePassive) {

		ASSERT_TRUE(Connect(testing::serverMagicOneWayActiveMode, false));

		testing::PacketsNumber packets = 0;
		ASSERT_NO_THROW(
			packets = GetClient().WaitAndTakeData<testing::PacketsNumber>(false));
		EXPECT_GT(packets, testing::PacketsNumber(0));

		for (testing::PacketsNumber i = 0; i < packets; ++i) {
			ASSERT_TRUE(ReceiveTestPacket(false));
			if (!(i % 25)) {
				ASSERT_NO_THROW(GetClient().Send(testing::clientMagicOk))
					<< " at packet " << i;
			}
		}

		ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicBay, true));
		EXPECT_EQ(0, GetClient().GetReceivedSize());

	}

	TEST_F(UdpClient, DataOneWayExchangeActive) {

		testing::PacketsNumber packets = 5000;

		ASSERT_TRUE(Connect(testing::serverMagicOneWayPassiveMode, false));
		ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicOk, true));

		ASSERT_NO_THROW(GetClient().SendVal(packets));

		for (testing::PacketsNumber i = 0; i < packets; ++i) {
			ASSERT_TRUE(SendTestPacket(128, false, .95));
			if (!(i % 25)) {
				ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicOk, true))
					<< " at packet  " << i;
			}
		}

		EXPECT_NO_THROW(GetClient().Send(testing::clientMagicBay));
		EXPECT_EQ(0, GetClient().GetReceivedSize());

	}

	TEST_F(UdpClient, SeveralConnetions) {

#		ifdef DEV_VER
			const testing::ConnectionsNumber connectionsNumber = 10; // see TEX-692
#		else
			const testing::ConnectionsNumber connectionsNumber = 200;
#		endif
		const testing::PacketsNumber packetsNumber = 10;
		const testing::PacketSize packetSize = 100;

		ASSERT_TRUE(Connect(testing::serverMagicSeveralConnectionsMode, false));
		ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicOk, true));

		ASSERT_NO_THROW(GetClient().SendVal(connectionsNumber));
		ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicOk, true));

		typedef std::list<boost::shared_ptr<TestUtil::Client>> Connections;
		Connections connections;
		for (testing::ConnectionsNumber i = 1; i <= connectionsNumber; ++i) {
			boost::shared_ptr<TestUtil::Client> connection(CreateConnection());
			ASSERT_TRUE(Connect(*connection, testing::serverMagicSubConnectionMode, false));
			connections.push_back(connection);
		}
		
		foreach (auto &connection, connections) {
			ASSERT_TRUE(connection->WaitAndTakeData(testing::serverMagicOk, true));
		}

		{
			testing::ConnectionsNumber i = 1;
			foreach (auto &connection, connections) {
				ASSERT_NO_THROW(connection->SendVal(i));
				testing::ConnectionsNumber remoteI = 0;
				ASSERT_NO_THROW(remoteI = connection->WaitAndTakeData<testing::ConnectionsNumber>(true));
				EXPECT_EQ(i, remoteI);
				++i;
			}
		}

		ASSERT_NO_THROW(GetClient().SendVal(packetsNumber));

		for (size_t i = 0; i < 2; ++i) {
			
			for (testing::PacketsNumber i = 0; i < packetsNumber; ++i) {
				
				{
					testing::ConnectionsNumber connectionNumber = 1;
					foreach (auto &connection, connections) {
						ASSERT_NO_THROW(connection->SendVal(connectionNumber));
						testing::ConnectionsNumber remoteConnectionIndex = 0;
						ASSERT_NO_THROW(
							remoteConnectionIndex
							= connection->WaitAndTakeData<testing::ConnectionsNumber>(true));
						EXPECT_EQ(connectionNumber, remoteConnectionIndex);
						++connectionNumber;
					}
				}

				foreach (auto &connection, connections) {
					ASSERT_TRUE(SendTestPacket(*connection, packetSize, true, .1));	
					ASSERT_TRUE(connection->WaitAndTakeData(testing::serverMagicOk, true));
					ASSERT_NO_THROW(connection->Send(testing::clientMagicOk));
				}

				foreach (auto &connection, connections) {
					ASSERT_TRUE(ReceiveTestPacket(*connection, true));
					ASSERT_NO_THROW(connection->Send(testing::clientMagicOk));
					ASSERT_TRUE(connection->WaitAndTakeData(testing::serverMagicOk, true));
				}

			
			}

		}

		ASSERT_NO_THROW(GetClient().Send(testing::clientMagicBay));
		ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicBay, true));

	}

}

#endif // INCLUDED_FILE__TUNNELEX__UdpClientTest_cpp__1106270010
