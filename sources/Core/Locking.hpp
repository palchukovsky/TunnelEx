/**************************************************************************
 *   Created: 2008/01/12 3:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Locking_h__0801120308
#define INCLUDED_FILE__TUNNELEX__Locking_h__0801120308

#include "Api.h"

namespace TunnelEx {

	class Lock;

	//////////////////////////////////////////////////////////////////////////

	//! Recursive mutex.
	class TUNNELEX_CORE_API RecursiveMutex {

		friend class ::TunnelEx::Lock;
		
	public:

		RecursiveMutex();
		~RecursiveMutex();

	private:

		RecursiveMutex(const RecursiveMutex &);
		const RecursiveMutex & operator =(const RecursiveMutex &);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

	//////////////////////////////////////////////////////////////////////////

	class TUNNELEX_CORE_API Lock {

	public:

		Lock(::TunnelEx::RecursiveMutex &);
		~Lock();

	private:

		Lock(const Lock &);
		const Lock & operator=(const Lock &);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

	//////////////////////////////////////////////////////////////////////////

	struct TUNNELEX_CORE_API Interlocked {

		static long Increment(long volatile *) throw();

		static long Decrement(long volatile *) throw();

		static long CompareExchange(
				long volatile &destination,
				long exchangeValue,
				long compareValue)
			throw();

	};


}

#endif // INCLUDED_FILE__TUNNELEX__Locking_h__0801120308
