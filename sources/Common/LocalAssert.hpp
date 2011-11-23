/**************************************************************************
 *   Created: 2011/04/19 21:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include <assert.h>
#include "CompileWarningsBoost.h"
#	include <boost/assert.hpp>
#include "CompileWarningsBoost.h"

#ifdef assert
#	undef assert
#endif
#define assert(expr) BOOST_ASSERT(expr)

#define verify(expr) BOOST_VERIFY(expr)

#ifndef INCLUDED_FILE__TUNNELEX__LocalAssert_h__1105130132
#define INCLUDED_FILE__TUNNELEX__LocalAssert_h__1105130132

namespace TunnelEx { namespace Helpers { namespace Asserts {

	template<typename Mutex>
	inline bool IsLocked(const Mutex &mutex) {
		return const_cast<Mutex &>(mutex).get_nesting_level() > 0;
	}

	template<typename Mutex>
	inline bool	IsLockedByMyThread(const Mutex &mutex) {
		return
			const_cast<Mutex &>(mutex).get_nesting_level() > 0
			&& ACE_OS::thr_self() == const_cast<Mutex &>(mutex).get_thread_id();
	}

	template<typename Mutex>
	inline bool IsNotLocked(const Mutex &mutex) {
		return const_cast<Mutex &>(mutex).get_nesting_level() < 1;
	}

	template<typename Mutex>
	inline bool IsNotLockedOrLockedByMyThread(const Mutex &mutex) {
		return
			const_cast<Mutex &>(mutex).get_nesting_level() < 1
			|| ACE_OS::thr_self() == const_cast<Mutex &>(mutex).get_thread_id();
	}

	template<typename Mutex>
	inline bool IsNotLockedByMyThread(const Mutex &mutex) {
		return
			const_cast<Mutex &>(mutex).get_nesting_level() < 1
			|| ACE_OS::thr_self() != const_cast<Mutex &>(mutex).get_thread_id();
	}

} } }

#endif
