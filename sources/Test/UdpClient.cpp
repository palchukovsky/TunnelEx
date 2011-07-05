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

		using Client::Connect;

		virtual void Connect(const std::string &mode, bool infiniteTimeout) {
			Connect(infiniteTimeout);
			ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicHello, true));
			ASSERT_NO_THROW(GetClient().Send(mode));
			ASSERT_NO_THROW(GetClient().Send(mode));
			ASSERT_NO_THROW(GetClient().Send(mode));
			ASSERT_NO_THROW(GetClient().Send(mode));
		}

		using Client::SendTestPacket;

		virtual void SendTestPacket(
					TestUtil::Client &client,
					testing::PacketSize size)
				const  {
			
			std::auto_ptr<TestUtil::Buffer> packet(new TestUtil::Buffer);
			boost::crc_32_type crc;
			testing::GeneratePacket(*packet, crc, size * 0.5, size * 1.5);
			
			ASSERT_NO_THROW(client.SendVal(testing::PacketSize(packet->size())));
			ASSERT_TRUE(client.WaitAndTakeData(testing::serverMagicOk, true));
			ASSERT_NO_THROW(client.Send(packet));
			ASSERT_TRUE(client.WaitAndTakeData(testing::serverMagicOk, true));
			ASSERT_NO_THROW(client.SendVal(crc.checksum()));
			ASSERT_TRUE(client.WaitAndTakeData(testing::serverMagicOk, true));

		}

		using Client::ReceiveTestPacket;

		virtual void ReceiveTestPacket(TestUtil::Client &client) const {
			testing::PacketSize size = 0;
			ASSERT_NO_THROW(size = client.WaitAndTakeData<testing::PacketSize>(true));
			EXPECT_GT(size, 0);
			ASSERT_NO_THROW(client.Send(testing::clientMagicOk));
			TestUtil::Buffer packet;
			ASSERT_NO_THROW(client.WaitAndTakeAnyData(size, true, packet));
			boost::crc_32_type realCrc;
			testing::Calc(packet, realCrc);
			boost::crc_32_type::value_type remoteCrc;
			ASSERT_NO_THROW(client.Send(testing::clientMagicOk));
			ASSERT_NO_THROW(
				remoteCrc = client.WaitAndTakeData<boost::crc_32_type::value_type>(true));
			EXPECT_EQ(realCrc.checksum(), remoteCrc);
			ASSERT_NO_THROW(client.Send(testing::clientMagicOk));
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
			SendTestPacket(16);
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

	TEST_F(UdpClient,  Simple) {
		Connect(false);
		for (testing::PacketsNumber i = 0; i < 1000; ++i) {
			std::ostringstream oss;
			oss << std::setfill('#') << std::setw(100) << i << '|';
			ASSERT_NO_THROW(GetClient().Send(oss.str()))
				<< "Failed send on #" << i << ".";
			Sleep(2000);
		}
	}

	TEST_F(UdpClient, DataExchangePassive) {

		Connect(testing::serverMagicActiveMode, false);

		testing::PacketsNumber packets = 0;
		ASSERT_NO_THROW(
			packets = GetClient().WaitAndTakeData<testing::PacketsNumber>(true));
		EXPECT_GT(packets, testing::PacketsNumber(0));
		ASSERT_NO_THROW(GetClient().Send(testing::clientMagicOk));

		for (testing::PacketsNumber i = 0; i < packets; ++i) {
			ReceiveTestPacket();
			SendTestPacket(4096);
		}

		ASSERT_TRUE(GetClient().WaitAndTakeData(testing::serverMagicBay, true));
		ASSERT_NO_THROW(GetClient().Send(testing::clientMagicBay));
		EXPECT_EQ(0, GetClient().GetReceivedSize());

	}

}

#endif // INCLUDED_FILE__TUNNELEX__UdpClientTest_cpp__1106270010
