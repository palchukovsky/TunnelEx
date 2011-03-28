/**************************************************************************
 *   Created: 2007/02/18 3:11
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 * -------------------------------------------------------------------
 *       $Id: Singleton.hpp 1084 2010-12-05 18:57:31Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__Singleton_h__0702180311
#define INCLUDED_FILE__Singleton_h__0702180311

#include "Api.h"

namespace TunnelEx {

//////////////////////////////////////////////////////////////////////////

	namespace Singletons {

		//! PhoenixLifetime policy for SingletonHolder.
		/*	Implementation of the LifetimePolicy used by SingletonHolder.
			Schedules an object's destruction as per C++ rules, and it allows
			object  recreation by not throwing an exception from
			OnDeadReference. */
		template <class T>
		class PhoenixLifetime;

		//////////////////////////////////////////////////////////////////////////

		//! DefaultLifetime policy for SingletonHolder.
		/*	Implementation of the LifetimePolicy used by SingletonHolder.
			Schedules an object's destruction as per C++ rules Forwards to
			std::atexit. */
		template <class T>
		class DefaultLifetime;


		//////////////////////////////////////////////////////////////////////////

		//! Multi-thread policy.
		template<typename T>
		class MultiThreadingModel {
		private:
			MultiThreadingModel();
		public:
			typedef volatile T VolatileType;
			class Lock;
		};

		//////////////////////////////////////////////////////////////////////////

		//! Singleton holder.
		template<
			typename T,
			template<class> class LifetimePolicy,
			template<class> class ThreadingModelPolicy = MultiThreadingModel>
		class TUNNELEX_CORE_API Holder {

		public:
		
			typedef typename T Type;
			typedef Type & Ref;
			typedef const Type & ConstRef;
			typedef Type * Ptr;
			typedef const Type * ConstPtr;
			
			static Ref GetInstance();

		private:

			typedef typename ThreadingModelPolicy<Ptr> ThreadingModel;
			typedef typename ThreadingModel::VolatileType InstancePtr;

			Holder();

			static bool m_isDestroyed;
			static InstancePtr m_instancePtr;

			static void MakeInstance();
			static void DestroyInstance();

		};

	}

}

#endif // ifndef INCLUDED_FILE__Singleton_h__0702180311
