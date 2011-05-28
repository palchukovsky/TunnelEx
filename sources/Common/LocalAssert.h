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

#ifndef INCLUDED_FILE__TUNNELEX__LocalAssert_h__1105130132
#define INCLUDED_FILE__TUNNELEX__LocalAssert_h__1105130132

template<typename Mutex>
inline void AssertLocked(const Mutex &mutex) {
	assert(const_cast<Mutex &>(mutex).get_nesting_level() > 0);
}

template<typename Mutex>
inline void AssertLockedByMyThread(const Mutex &mutex) {
	assert(const_cast<Mutex &>(mutex).get_nesting_level() > 0);
	assert(ACE_OS::thr_self() == const_cast<Mutex &>(mutex).get_thread_id());
}

template<typename Mutex>
inline void AssertNotLocked(const Mutex &mutex) {
	assert(const_cast<Mutex &>(mutex).get_nesting_level() < 1);
}

template<typename Mutex>
inline void AssertNotLockedByMyThread(const Mutex &mutex) {
	assert(
		const_cast<Mutex &>(mutex).get_nesting_level() < 1
		|| ACE_OS::thr_self() != const_cast<Mutex &>(mutex).get_thread_id());
}

#endif
