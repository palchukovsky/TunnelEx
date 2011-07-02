/**************************************************************************
 *   Created: 2011/06/26 22:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Client_hpp__1106262228
#define INCLUDED_FILE__TUNNELEX__Client_hpp__1106262228

#include "Common.hpp"

namespace TestUtil {

	class Client;

}

namespace testing {

	class Client : public testing::Test  {

	public:
	
		virtual ~Client();
	
	public:
		
		static void SetUpTestCase();
		static void TearDownTestCase();
		
		virtual void SetUp();
		virtual void TearDown();

	protected:

		TestUtil::Client & GetClient();
		const TestUtil::Client & GetClient() const;

		virtual std::auto_ptr<TestUtil::Client> CreateClient() const = 0;

		std::auto_ptr<TestUtil::Client> CreateConnection() const;

		void Connect(const std::string &mode, bool infiniteTimeout);
		void Connect(
					TestUtil::Client &client,
					bool infiniteTimeout)
				const;

		void SendTestPacket(testing::PacketSize size);
		void SendTestPacket(
					TestUtil::Client &client,
					testing::PacketSize size)
				const;
			
		void ReceiveTestPacket();
		void ReceiveTestPacket(TestUtil::Client &client) const;

	private:

		std::auto_ptr<TestUtil::Client> m_client;

	};

	////////////////////////////////////////////////////////////////////////////////

}

#endif // INCLUDED_FILE__TUNNELEX__Client_hpp__1106262228
