/**************************************************************************
 *   Created: 2011/07/13 10:50
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__ConnectServer_hpp__1107131050
#define INCLUDED_FILE__TUNNELEX__ConnectServer_hpp__1107131050

#include "Common.hpp"
#include "TestUtils/ClientServer.hpp"

namespace testing {

	class ConnectServer : public testing::Test  {
	
	public:
	
		virtual ~ConnectServer();
	
	public:
		
		static void SetUpTestCase();
		static void TearDownTestCase();
		
		virtual void SetUp();
		virtual void TearDown();

	public:

		bool TestAny();

	protected:
		
		virtual std::auto_ptr<TestUtil::Server> CreateServer() const = 0;

	private:

		bool TestActiveServer(size_t mainConnection);
		bool TestPassiveServer(size_t mainConnection);
		bool TestOneWayActiveServer(size_t mainConnection);
		bool TestOneWayPassiveServer(size_t mainConnection);
		bool TestSeveralConnections(size_t mainConnection);

	private:

		void DoTest(bool &result);
		void DoTestActiveServer(size_t connection, bool &result);
		void DoTestPassiveServer(size_t connection, bool &result);
		void DoTestOneWayActiveServer(size_t connection, bool &result);
		void DoTestOneWayPassiveServer(size_t connection, bool &result);
		void DoTestSeveralConnections(size_t mainConnection, bool &result);

	private:

		bool SendTestPacket(
					size_t connection,
					testing::PacketSize size,
					double widthRatio);
		void DoSendTestPacket(
					size_t connection,
					testing::PacketSize size,
					double widthRatio,
					bool &result);

		bool ReceiveTestPacket(size_t connection);
		void DoReceiveTestPacket(size_t connection, bool &result);

	private:

		std::auto_ptr<TestUtil::Server> m_server;

	};

}

#endif
