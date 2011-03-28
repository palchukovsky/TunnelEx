/**************************************************************************
 *   Created: 2008/01/12 3:07
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.com
 * -------------------------------------------------------------------
 *       $Id: Locking.cpp 978 2010-07-09 03:09:22Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "Locking.hpp"

using namespace boost;
using namespace TunnelEx;

//////////////////////////////////////////////////////////////////////////

class RecursiveMutex::Implementation : private noncopyable {

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

class Lock::Implementation : private noncopyable {

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

long Interlocked::Decrement(long volatile *ptr) throw() {
	return BOOST_INTERLOCKED_DECREMENT(ptr);
}

//////////////////////////////////////////////////////////////////////////
