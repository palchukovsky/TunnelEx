/**************************************************************************
 *   Created: 2011/07/12 21:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__ConnectClient_hpp__1107122152
#define INCLUDED_FILE__TUNNELEX__ConnectClient_hpp__1107122152

#include "Client.hpp"

namespace testing {

	class ConnectClient : public testing::Client {

	public:

		typedef ConnectClient Self;
		typedef testing::Client Base;
	
	public:
	
		virtual ~ConnectClient();

	public:

		bool TestDataExchangeActive();
		bool TestDataExchangePassive();
		bool TestDataExchangeOneWayActive();
		bool TestDataOneWayExchangePassive();
		bool TestSeveralConnetions();

	protected:

		virtual std::auto_ptr<TestUtil::Client> CreateClient(
					const boost::posix_time::time_duration &waitTime)
				const
				= 0;

	private:

		std::auto_ptr<TestUtil::Client> CreateCloseControlConnection() const;
		virtual void DoConnect(
					TestUtil::Client &client,
					const std::string &mode,
					bool infiniteTimeout,
					bool &result)
				const;

		bool SendTestPacket(testing::PacketSize size, double widthRatio);
		bool SendTestPacket(
					TestUtil::Client &client,
					testing::PacketSize size,
					double widthRatio)
				const;
		bool ReceiveTestPacket();
		bool ReceiveTestPacket(TestUtil::Client &client) const;

	private:

		void DoSendTestPacket(
					TestUtil::Client &client,
					testing::PacketSize size,
					double widthRatio,
					bool &result)
				const;
		void DoReceiveTestPacket(TestUtil::Client &client, bool &result) const;

	private:

		void DoDataExchangeActiveTest(bool &result);
		void DoDataExchangePassiveTest(bool &result);
		void DoDataExchangeOneWayActiveTest(bool &result);
		void DoDataOneWayExchangePassiveTest(bool &result);
		void DoSeveralConnetionsTest(bool &result);

	};

}

#endif // INCLUDED_FILE__TUNNELEX__ConnectClient_hpp__1107122152
