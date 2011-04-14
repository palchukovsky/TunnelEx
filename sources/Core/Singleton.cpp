/**************************************************************************
 *   Created: 2007/03/11 1:03
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Singleton.cpp 959 2010-06-22 11:36:04Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#ifndef INCLUDED_FILE__Singleton_cpp__0703110153
#define INCLUDED_FILE__Singleton_cpp__0703110153

#include "Singleton.hpp"

using namespace TunnelEx;
using namespace TunnelEx::Singletons;

//////////////////////////////////////////////////////////////////////////

template <class T>
class Singletons::PhoenixLifetime {

private:
	PhoenixLifetime();

public:

	static void ScheduleDestruction(void(*killFuncPtr)()) {
#		if ATEXIT_FIXED == 0
			if (!m_isDestroyedOnce)  {
				atexit(killFuncPtr);
			}
#		else // ATEXIT_FIXED == 0
			atexit(killFuncPtr);
#		endif // ATEXIT_FIXED != 0
	}

	static void PhoenixLifetime::OnDeadReference() {
#		if ATEXIT_FIXED == 0
			m_isDestroyedOnce = true;
#		endif // ATEXIT_FIXED == 0
	}

#if ATEXIT_FIXED == 0
	private:
		static bool m_isDestroyedOnce;
#endif // ATEXIT_FIXED == 0

};

#if ATEXIT_FIXED == 0
	template<typename T>
	bool PhoenixLifetime<T>::m_isDestroyedOnce = false;
#endif // ATEXIT_FIXED == 0

//////////////////////////////////////////////////////////////////////////

template <class T>
class Singletons::DefaultLifetime {
private:
	DefaultLifetime();
public:
	static void DefaultLifetime::ScheduleDestruction(void(*killFuncPtr)()) {
		atexit(killFuncPtr);
	}
	static void DefaultLifetime::OnDeadReference() {
		throw std::exception("Singleton dead reference detected."); 
	}
};
//////////////////////////////////////////////////////////////////////////

template<typename T>
class TunnelEx::Singletons::MultiThreadingModel<T>::Lock : private boost::noncopyable {
public:
	Lock()
			: m_lock(m_mutex) {
		//...//
	};
	~Lock() {
		//...//
	}

private:
	static ACE_Thread_Mutex m_mutex;
	ACE_Guard<ACE_Thread_Mutex> m_lock;
};

template<typename T>
ACE_Thread_Mutex MultiThreadingModel<T>::Lock::m_mutex;

//////////////////////////////////////////////////////////////////////////

template<typename T, template<class> class L, template<class> class Th>
typename Holder<T, L, Th>::Ref Holder<T, L, Th>::GetInstance() {
	if (!m_instancePtr) {
		MakeInstance();
	}
	return *m_instancePtr;
}

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable: 4702)
#endif // defined(_MSC_VER)
template<
	typename T,
	template<class> class LifetimePolicy,
	template<typename> class ThreadingModel>
void Holder<T, LifetimePolicy, ThreadingModel>::MakeInstance() {
	ThreadingModel::Lock lock;
	if (!m_instancePtr) {
		if (m_isDestroyed) {
			LifetimePolicy<T>::OnDeadReference();
			m_isDestroyed = false;
		}
		InstancePtr instancePtr = static_cast<T *>(malloc(sizeof T));
		if (!instancePtr) {
			throw std::bad_alloc(
				"New singleton object creation is failed"
				" (insufficient memory available)");
		}
		try {
			new(instancePtr)T;
		} catch (...) {
			free(instancePtr);
			throw;
		}
		m_instancePtr = instancePtr;
		LifetimePolicy<T>::ScheduleDestruction(&DestroyInstance);
	}
}
#if defined(_MSC_VER)
#	pragma warning(pop)
#endif // defined(_MSC_VER)

template<typename T, template<class> class L, template<class> class Th>
void Holder<T, L, Th>::DestroyInstance() {
	BOOST_ASSERT(!m_isDestroyed);
	BOOST_ASSERT(m_instancePtr);
	delete m_instancePtr;
	m_instancePtr = 0;
	m_isDestroyed = true;
}

template<typename T, template<class> class L, template<class> class Th>
typename Holder<T, L, Th>::InstancePtr Holder<T, L, Th>::m_instancePtr = NULL;

template<typename T, template<class> class L, template<class> class Th>
bool Holder<T, L, Th>::m_isDestroyed = false;

//////////////////////////////////////////////////////////////////////////

#endif // INCLUDED_FILE__Singleton_cpp__0703110153
