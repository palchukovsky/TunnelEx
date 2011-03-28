/**************************************************************************
 *   Created: 2007/09/09 23:38
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: PosixTime.hpp 1046 2010-11-02 12:07:07Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__PosixTime_hpp__0709092339
#define INCLUDED_FILE__TUNNELEX__PosixTime_hpp__0709092339

#ifdef TUNNELEX_CORE
#	include "Time.h"
#else // #ifdef TUNNELEX_CORE
#	include <TunnelEx/Time.h>
#endif // #ifdef TUNNELEX_CORE

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
		using namespace boost::posix_time;
		BOOST_ASSERT(!posixTime.is_special());
		static const ptime timeTEpoch(boost::gregorian::date(1970, 1, 1));
		BOOST_ASSERT(!(posixTime < timeTEpoch));
		if (posixTime < timeTEpoch) {
			return 0;
		}
		const time_duration durationFromTEpoch(posixTime - timeTEpoch);
		return static_cast<TimeT>(durationFromTEpoch.total_seconds());
	}

} }

#endif // #ifndef INCLUDED_FILE__TUNNELEX__PosixTime_hpp__0709092339
