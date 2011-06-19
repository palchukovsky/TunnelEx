/**************************************************************************
 *   Created: 2011/06/01 21:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "Common.hpp"

namespace testing {

	const unsigned short tcpServerPort = 12332;

	const std::string clientMagicHello("{CLENT HELLO 3DE5F96B-6BDD-4B8E-B7A2-68537DF874B7}");
	const std::string clientMagicBay("{CLENT BAY 2F200986-C0CF-496E-A70B-EB567970ADC9}");
	const std::string clientMagicBegin(">>>CLIENT DATA BEGIN>>>");
	const std::string clientMagicEnd("<<<CLIENT DATA END<<<");
	const std::string clientMagicOk("{CLIENT OK 9834D7F7-90A7-49F6-9E0E-E3D592F8BBDC}");

	const std::string serverMagicHello("{SERVER HELLO 69C66C6A-BED9-4FE4-961E-08B292AA92B3}");
	const std::string serverMagicBay("{SERVER BAY 6A8EB838-34D2-4305-89C1-180BC9293D91}");
	const std::string serverMagicBegin(">>>SERVER DATA BEGIN>>>");
	const std::string serverMagicEnd("<<<SERVER DATA END<<<");
	const std::string serverMagicOk("{SERVER OK 86935147-0D01-4FCB-A32D-A80F93FDC189}");

	const std::string serverMagicActiveMode("{SERVER ACTIVE MODE}");
	const std::string serverMagicPassiveMode("{SERVER PASSIVE MODE}");
	const std::string serverMagicOneWayActiveMode("{SERVER ONE WAY ACTIVE MODE}");
	const std::string serverMagicOneWayPassiveMode("{SERVER ONE WAY PASSIVE MODE}");

	const boost::posix_time::time_duration defaultDataWaitTime(0, 0, 3);

	boost::mt19937 generator;

}
