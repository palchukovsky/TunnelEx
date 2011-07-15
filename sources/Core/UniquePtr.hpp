/**************************************************************************
 *    Created: 2007/08/16 1:54
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__UniquePtr_h__0708160154
#define INCLUDED_FILE__UniquePtr_h__0708160154

#include "Unique.hpp"
#include "LocalAssert.h"

namespace TunnelEx {

	namespace Helpers {

		template<typename T>
		struct UniquePtrTrait {
		
			template<typename T>
			struct RemoveConst {
				typedef T Type;
			};
			template<typename T>
			struct RemoveConst<const T> {
				typedef T Type;
			};

			typedef T * Ptr;
			typedef typename RemoveConst<T>::Type * MutablePtr;

			static inline void Delete(MutablePtr ptr) {
				typedef char TypeMustBeComplete[sizeof(T)? 1: -1];
				sizeof(TypeMustBeComplete);
				delete ptr;
			}

			struct Ref : public ::TunnelEx::Helpers::UniqueRef<Ptr, void(MutablePtr)> {
				typedef UniqueRef<Ptr, void(MutablePtr)> Base;
				typedef typename Base::Dtor Dtor;
				explicit Ref(
							Element element,
							Dtor dtor,
							::TunnelEx::Helpers::UniqueHolderBase *const holder = 0)
						: Base(element, dtor, holder) {
					//...//
				}
				explicit Ref(const Base &base)
						: Base(base) {
					//...//
				}
			};
		
		};


	}

	//! Smart pointer with std::auto_ptr semantic.
	/** Uses in case of work with TunnelEx's core pointers.
	  * @sa TunnelEx::SharedPtr
	  */
	template<typename T>
	class UniquePtr
		: public ::TunnelEx::Unique<
			typename Helpers::UniquePtrTrait<T>::Ptr,
			typename Helpers::UniquePtrTrait<T>::MutablePtr,
			0> {

		template<typename Other>
		friend class UniquePtr;

	public:

		typedef T ElementType;
		typedef UniquePtr<ElementType> Self;
		typedef Helpers::UniquePtrTrait<ElementType> Trait;
		typedef typename Trait::Ref Ref;
		typedef Unique<ElementType *, typename Trait::MutablePtr, 0> Base;

	public:

		UniquePtr() throw() {
			//...//
		}

		explicit UniquePtr(ElementType *const ptr)
				: Base(ptr, &Trait::Delete) {
			//...//
		}

	public:

		UniquePtr(Self &rhs) throw()
				: Base(rhs) {
			//...//
		}

		template<typename Other>
		UniquePtr(typename ::TunnelEx::UniquePtr<Other>::Ref ref)
				: Base(ref) {
			//...//
		}

		template<typename Other>
		UniquePtr(UniquePtr<Other> &rhs) throw()
				: Base(rhs) {
			//...//
		}

	public:

		template<typename Other>
		const Self & operator =(UniquePtr<Other> &rhs) throw() {
			Base::operator =(rhs);
			return *this;
		}

		const Self & operator =(Self &rhs) throw() {
			Base::operator =(rhs);
			return *this;
		}

		template<typename Other>
		operator typename ::TunnelEx::UniquePtr<Other>::Ref() throw() {
			typedef ::TunnelEx::UniquePtr<Other>::Ref Ref;
			return Ref(Base::operator ::TunnelEx::Helpers::UniqueRef<Other *, Ref::Dtor>());
		}

		ElementType & operator *() const throw() {
			assert(Get() != 0);
			return *Get();
		}

		ElementType * operator ->() const throw() {
			return Get();
		}

		operator bool() const throw() {
			return Base::operator bool();
		}

		bool operator !() const throw() {
			return Base::operator !();
		}

		ElementType * Get() const throw() {
			return Base::Get();
		}

		void Reset() {
			Base::Reset();
		}

		void Reset(ElementType * ptr) {
			Self(ptr).Swap(*this);
		}

		ElementType * Release() throw() {
			return Base::Release();
		}

		void Swap(Self &rhs) throw() {
			Base::Swap(rhs);
		}

	};

}

template<typename T, typename U>
inline bool operator ==(
			::TunnelEx::UniquePtr<T> const &a,
			::TunnelEx::UniquePtr<U> const &b) {
	assert(&a != &b);
	return a.Get() == b.Get();
}

template<typename T, typename U>
inline bool operator !=(
			const ::TunnelEx::UniquePtr<T> &a,
			const ::TunnelEx::UniquePtr<U> &b) {
	assert(&a != &b);
	return a.Get() != b.Get();
}

template<typename T, typename U>
inline bool operator <(
			const ::TunnelEx::UniquePtr<T> &a,
			const ::TunnelEx::UniquePtr<U> &b) {
	assert(&a != &b);
	return a.Get() < b.Get();
}

template<typename T, typename U>
inline bool operator >(
			const ::TunnelEx::UniquePtr<T> &a,
			const ::TunnelEx::UniquePtr<U> &b) {
	assert(&a != &b);
	return a.Get() > b.Get();
}

#endif // INCLUDED_FILE__UniquePtr_h__0708160154
