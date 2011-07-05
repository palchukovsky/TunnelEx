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

void Client::Connect(bool infiniteTimeout) {
	Connect(*m_client, infiniteTimeout);
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

void Client::ReceiveTestPacket() {
	ReceiveTestPacket(*m_client);
}
