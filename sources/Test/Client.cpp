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

bool Client::Connect(const std::string &mode, bool infiniteTimeout) {
	bool result = false;
	DoConnect(GetClient(), mode, infiniteTimeout, result);
	return result;
}

bool Client::Connect(
			TestUtil::Client &client,
			const std::string &mode,
			bool infiniteTimeout)
		const {
	bool result = false;
	DoConnect(client, mode, infiniteTimeout, result);
	return result;
}

bool Client::Connect(bool infiniteTimeout) {
	bool result = false;
	DoConnect(*m_client, infiniteTimeout, result);
	return result;
}

bool Client::Connect(
			TestUtil::Client &client,
			bool infiniteTimeout)
		const {
	bool result	= false;
	DoConnect(client, infiniteTimeout, result);
	return result;
}

void Client::DoConnect(
			TestUtil::Client &client,
			bool infiniteTimeout,
			bool &result)
		const {
	ASSERT_TRUE(client.WaitConnect(infiniteTimeout));
	ASSERT_NO_THROW(client.Send(testing::clientMagicHello));
	result = true;
}
