/**************************************************************************
 *   Created: 2011/06/01 21:46
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Constants_hpp__1106012146
#define INCLUDED_FILE__TUNNELEX__Constants_hpp__1106012146

namespace testing {

	typedef boost::uint32_t PacketsNumber;
	typedef boost::uint16_t PacketSize;
	typedef boost::uint16_t ConnectionsNumber;

	extern std::string tcpServerHost;
	extern unsigned short tcpServerPort;
	extern std::string udpServerHost;
	extern unsigned short udpServerPort;
	extern std::string pipeServerPath;
	
	extern const std::string clientMagicHello;
	extern const std::string clientMagicBay;
	extern const std::string clientMagicBegin;
	extern const std::string clientMagicEnd;
	extern const std::string clientMagicOk;

	extern const std::string serverMagicHello;
	extern const std::string serverMagicBay;
	extern const std::string serverMagicBegin;
	extern const std::string serverMagicEnd;
	extern const std::string serverMagicOk;

	extern const std::string serverMagicActiveMode;
	extern const std::string serverMagicPassiveMode;
	extern const std::string serverMagicOneWayActiveMode;
	extern const std::string serverMagicOneWayPassiveMode;
	extern const std::string serverMagicSeveralConnectionsMode;
	
	extern const std::string serverMagicSubConnectionMode;

	extern const std::string serverMagicDummyMode;

	extern const boost::posix_time::time_duration defaultDataWaitTime;

	extern boost::mt19937 generator;

	template<typename Buffer>
	void GeneratePacket(
				Buffer &result,
				boost::crc_32_type &crc,
				size_t minSize,
				size_t maxSize) {

		assert(minSize != maxSize);
		assert(minSize < maxSize);
		testing::PacketSize size = 0;
		{
			const boost::uniform_int<> distance(minSize, maxSize);
			boost::variate_generator<boost::mt19937 &, boost::uniform_int<>> die(
				generator,
				distance);
			size = die();
		}

		const boost::uniform_int<> distance(
			std::numeric_limits<Buffer::value_type>::min(),
			std::numeric_limits<Buffer::value_type>::max());
		boost::variate_generator<boost::mt19937 &, boost::uniform_int<>> die(
			generator,
			distance);

		result.resize(size);
		foreach (Buffer::value_type &i, result) {
			i = die();
			crc.process_byte(static_cast<unsigned char>(i));
		}

	}

	template<typename Buffer>
	void Calc(const Buffer &buffer, boost::crc_32_type &crc) {
		foreach (const Buffer::value_type &i, buffer) {
			crc.process_byte(static_cast<unsigned char>(i));
		}
	}

}

#endif // INCLUDED_FILE__TUNNELEX__Constants_hpp__1106012146
