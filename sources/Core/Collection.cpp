/**************************************************************************
 *   Created: 2007/09/19 23:21
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Collection.cpp 1127 2011-02-22 17:23:32Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "Collection.hpp"
#include "Exceptions.hpp"

using namespace std;
using namespace boost;
using namespace TunnelEx;

//////////////////////////////////////////////////////////////////////////

template<class Item>
class TunnelEx::Collection<Item>::Implementation {

public:

	Implementation() {
		//...//
	}

	Implementation(size_t reserve) {
		Reserve(reserve);
	}
	
	Implementation(const Implementation &rhs)
			: m_collection(rhs.m_collection.clone()) { //! @todo: check result, it must be copies of objects [2008/08/16 23:16]
		//...//
	}

	~Implementation() {
		//...//
	}

private:

	const Implementation & operator =(const Implementation &);

public:
	
	Item & GetAt(size_t index) {
		if (m_collection.size() <= index) {
			throw LogicalException(
				L"Could not extract item from the collection "
					L"(index is not in the array bound).");
		}
		return m_collection[index];
	}

	size_t GetSize() const {
		return m_collection.size();
	}

	void SetSize(size_t size) {
		m_collection.resize(size);
	}

	void Append(const Item &item) {
		m_collection.push_back(new Item(item));
	}

	void Delete(size_t index) {
		m_collection.erase(m_collection.begin() + index);
	}

	void Reserve(size_t reserve) {
		m_collection.reserve(reserve);
	}

private:

	ptr_vector<Item> m_collection;

};


//////////////////////////////////////////////////////////////////////////

template<class Item>
Collection<Item>::Collection()
		: m_pimpl(new Implementation) {
	//...//
}

template<class Item>
Collection<Item>::Collection(size_t reserve)
		: m_pimpl(new Implementation(reserve)) {
	//...//
}

template<class Item>
Collection<Item>::Collection(const Collection &rhs)
		: m_pimpl(new Implementation(*rhs.m_pimpl)) {
	//...//
} 

template<class Item>
Collection<Item>::~Collection() throw() {
	delete m_pimpl;
}

template<class Item>
void Collection<Item>::Reserve(size_t reserve) {
	m_pimpl->Reserve(reserve);
}

template<class Item>
const Collection<Item> &
Collection<Item>::operator =(const Collection &rhs) {
	Collection<typename Item> tmp(rhs);
	Swap(tmp);
	return *this;
}

template<class Item>
void Collection<Item>::Swap(Collection<Item> &rho) throw() {
	Implementation *const oldImpl(m_pimpl);
	m_pimpl = rho.m_pimpl;
	rho.m_pimpl = oldImpl;
}

template<class Item>
const Item & Collection<Item>::operator [](size_t index) const {
	return const_cast<Collection *>(this)->operator [](index);
}

template<class Item>
Item& Collection<Item>::operator [](size_t index) {
	return m_pimpl->GetAt(index);
}

template<class Item>
size_t Collection<Item>::GetSize() const {
	return m_pimpl->GetSize();
}

template<class Item>
void Collection<Item>::SetSize(size_t size) {
	m_pimpl->SetSize(size);
}

template<class Item>
void Collection<Item>::Append(const Item &item) {
	m_pimpl->Append(item);
}

template<class Item>
void Collection<Item>::Remove(size_t intdex) {
	m_pimpl->Delete(intdex);
}

//////////////////////////////////////////////////////////////////////////

#if TEMPLATES_REQUIRE_SOURCE != 0
	//! Only for template instantiation.
	namespace TunnelEx { namespace Helpers {
		template<class Collection>
		void MakeCollectionTemplateInstantiation() {
			Collection collection;
			Collection(static_cast<unsigned int>(0));
			const Collection &constRef = collection;
			Collection collectionCopy(collection);
			collection[0];
			constRef[0];
			collection = collectionCopy;
			collection.Swap(collectionCopy);
			collection.Append(Collection::ItemType());
			collection.Remove(0);
			collection.GetSize();
			collection.SetSize(0);
			collection.Reserve(0);
		}
	} }
#endif // TEMPLATES_REQUIRE_SOURCE
