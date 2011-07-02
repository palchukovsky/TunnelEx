/**************************************************************************
 *   Created: 2011/06/26 23:58
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "Client.hpp"
#include "TestUtils/ClientServer.hpp"

using namespace testing;

Client::~Client() {
	//...//
}
	
void Client::SetUpTestCase() {
	//...//
}
		
void Client::TearDownTestCase() {
	//...//
}
		
void Client::SetUp() {
	m_client = CreateConnection();
}
		
void Client::TearDown() {
	m_client.reset();
}

TestUtil::Client & Client::GetClient() {
	return *m_client;
}
const TestUtil::Client & Client::GetClient() const {
	return const_cast<Client *>(this)->GetClient();
}

std::auto_ptr<TestUtil::Client> Client::CreateConnection() const {
	std::auto_ptr<TestUtil::Client> result = CreateClient();
	result->SetWaitTime(testing::defaultDataWaitTime);
	return result;
}

void Client::Connect(const std::string &mode, bool infiniteTimeout) {
	Connect(*m_client, infiniteTimeout);
	ASSERT_NO_THROW(m_client->Send(mode));
}

void Client::Connect(
			TestUtil::Client &client,
			bool infiniteTimeout)
		const {
	ASSERT_TRUE(client.WaitConnect(infiniteTimeout));
	ASSERT_NO_THROW(client.Send(testing::clientMagicHello));
}

void Client::SendTestPacket(testing::PacketSize size) {
	SendTestPacket(*m_client, size);
}

void Client::SendTestPacket(
			TestUtil::Client &client,
			testing::PacketSize size)
		const  {
			
	ASSERT_NO_THROW(client.Send(testing::clientMagicBegin));
			
	std::auto_ptr<TestUtil::Buffer> packet(new TestUtil::Buffer);
	boost::crc_32_type crc;
	testing::GeneratePacket(*packet, crc, size * 0.5, size * 1.5);
			
	ASSERT_NO_THROW(client.SendVal(testing::PacketSize(packet->size())));
	ASSERT_NO_THROW(client.Send(packet));
	ASSERT_NO_THROW(client.SendVal(crc.checksum()));
	ASSERT_NO_THROW(client.Send(testing::clientMagicEnd));

}

void Client::ReceiveTestPacket() {
	ReceiveTestPacket(*m_client);
}

void Client::ReceiveTestPacket(TestUtil::Client &client) const {
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
