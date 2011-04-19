/**************************************************************************
 *   Created: 2007/09/19 23:16
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 Eugene V. Palchukovsky
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Collection_h__0709192316
#define INCLUDED_FILE__TUNNELEX__Collection_h__0709192316

#include "Api.h"

namespace TunnelEx {

	//! Template for collection's API.
	template<class Item>
	class TUNNELEX_CORE_API Collection {

	public:

		typedef typename Item ItemType;

	public:

		Collection();
		//! Reserves a memory for items at creation for quickly adding.
		/*	Just reserves, doesn't create items. */
		explicit Collection(size_t reserve);
		Collection(const Collection &);
		~Collection() throw();

		const Collection & operator =(const Collection &);

		void Swap(Collection &) throw();
		//! Reserves a memory for items for quickly adding.
		/*	Just reserves, doesn't create items. */
		void Reserve(size_t);
	
		const Item & operator [](size_t) const;
		Item & operator [](size_t);

		void Append(const Item &);
		void Remove(size_t intdex);

		size_t GetSize() const;
		void SetSize(size_t);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}

#endif // INCLUDED_FILE__TUNNELEX__Collection_h__0709192316