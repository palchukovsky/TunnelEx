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

//////////////////////////////////////////////////////////////////////////

class Lock::Implementation : private boost::noncopyable {

public:

	Implementation(ACE_Recursive_Thread_Mutex& mutex)
			: m_lock(mutex) {
		//...//
	}

private:

	ACE_Guard<ACE_Recursive_Thread_Mutex> m_lock;

};

Lock::Lock(RecursiveMutex& mutex)
		: m_pimpl(new Implementation(mutex.m_pimpl->mutex)) {
	//...//
}

Lock::~Lock() {
	delete m_pimpl;
}

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
