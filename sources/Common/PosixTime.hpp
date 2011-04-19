/**************************************************************************
 *   Created: 2007/09/09 23:38
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__PosixTime_hpp__0709092339
#define INCLUDED_FILE__TUNNELEX__PosixTime_hpp__0709092339

#include "Core/Time.h"

#include "CompileWarningsBoost.h"
#	include <boost/date_time/posix_time/posix_time.hpp>
#	include <boost/assert.hpp>
#include "CompileWarningsBoost.h"

#ifdef _WINDOWS
#	ifdef Yield
#		undef Yield
#	endif // Yield
#endif // _WINDOWS

namespace TunnelEx { namespace Helpers {

	//! @todo: check function in new version of boost
	inline TunnelEx::TimeT ConvertPosixTimeToTimeT(
				const boost::posix_time::ptime &posixTime) {
		namespace pt = boost::posix_time;
		BOOST_ASSERT(!posixTime.is_special());
		static const pt::ptime timeTEpoch(boost::gregorian::date(1970, 1, 1));
		BOOST_ASSERT(!(posixTime < timeTEpoch));
		if (posixTime < timeTEpoch) {
			return 0;
		}
		const pt::time_duration durationFromTEpoch(posixTime - timeTEpoch);
		return static_cast<TimeT>(durationFromTEpoch.total_seconds());
	}

} }

#endif // #ifndef INCLUDED_FILE__TUNNELEX__PosixTime_hpp__0709092339
