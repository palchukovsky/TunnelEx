/**************************************************************************
 *   Created: 2008/01/12 3:07
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.com
 **************************************************************************/

#include "Prec.h"
#include "Locking.hpp"
#ifdef DEV_VER
//#	include "Log.hpp"
#endif

using namespace TunnelEx;

//////////////////////////////////////////////////////////////////////////

class RecursiveMutex::Implementation : private boost::noncopyable {
public:
	ACE_Recursive_Thread_Mutex mutex;
};

RecursiveMutex::RecursiveMutex()
		: m_pimpl(new Implementation) {
	//...//
}
		
RecursiveMutex::~RecursiveMutex() {
	delete m_pimpl;
}

void RecursiveMutex::Acquire() throw() {
	m_pimpl->mutex.acquire();
}

void RecursiveMutex::Release() throw() {
	m_pimpl->mutex.release();
}

//////////////////////////////////////////////////////////////////////////

void Helpers::TolerantSpinWait::Sleep() {
	::Sleep(0);
}

//////////////////////////////////////////////////////////////////////////

#ifdef DEV_VER
	void Helpers::AggressiveSpinWait::Report() const {
// 		std::ostringstream oss;
// 		oss << "AggressiveSpinWait::m_iterationsCount = " << m_iterationsCount;
// 		Log::GetInstance().AppendWarn(oss.str());
	}
#endif

//////////////////////////////////////////////////////////////////////////

long Interlocked::Increment(long volatile *ptr) throw() {
	return BOOST_INTERLOCKED_INCREMENT(ptr);
}

long Interlocked::Increment(long volatile &ref) throw() {
	return BOOST_INTERLOCKED_INCREMENT(&ref);
}

long Interlocked::Decrement(long volatile *ptr) throw() {
	return BOOST_INTERLOCKED_DECREMENT(ptr);
}

long Interlocked::Decrement(long volatile &ref) throw() {
	return BOOST_INTERLOCKED_DECREMENT(&ref);
}

long Interlocked::Exchange(long volatile &destination, long value) throw() {
	return BOOST_INTERLOCKED_EXCHANGE(&destination, value);
}

long Interlocked::CompareExchange(
			long volatile &destination,
			long exchangeValue,
			long compareValue)
		throw() {
	return BOOST_INTERLOCKED_COMPARE_EXCHANGE(&destination, exchangeValue, compareValue);
}

//////////////////////////////////////////////////////////////////////////
