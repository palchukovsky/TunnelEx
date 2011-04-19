/**************************************************************************
 *   Created: 2008/03/20 17:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__SharedPtr_h__0803201737
#define INCLUDED_FILE__TUNNELEX__SharedPtr_h__0803201737

#include "Api.h"
#include "Locking.hpp"

namespace TunnelEx {

	template<class T>
	class UniquePtr;

	namespace Helpers {

		//! Abstract pointer holder for TunnelEx::SharedPtr.
		class TUNNELEX_CORE_API SharedPtrHolderBase {
		public:
			SharedPtrHolderBase() throw()
					: m_refCount(1) {
				//...//
			}
		protected:
			virtual ~SharedPtrHolderBase() throw() {
				//...//
			}
		private:
			SharedPtrHolderBase(const SharedPtrHolderBase &);
			const SharedPtrHolderBase & operator =(const SharedPtrHolderBase &);
		public:
			virtual void Delete() = 0;
			void AddRef() throw() {
				::TunnelEx::Interlocked::Increment(&m_refCount);
			}
			void RemoveRef() {
				if (::TunnelEx::Interlocked::Decrement(&m_refCount) == 0) {
					Delete();
				}
			}
		private:
			long m_refCount;
		};

		//! Pointer holder implementation for TunnelEx::SharedPtr.
		template<class T>
		class SharedPtrHolder : public ::TunnelEx::Helpers::SharedPtrHolderBase {
		public:
			explicit SharedPtrHolder(T *const ptr) throw()
					: m_ptr(ptr) {
				//...//
			}
		protected:
			virtual ~SharedPtrHolder() {
				delete m_ptr;
			}
		public:
			void Delete() {
				delete this;
			}
		private:
			const T *const m_ptr;
		};

	}

	//! Smart pointer with references counting.
	/** Uses in case of work with TunnelEx's core pointers.
	  * @sa TunnelEx::UniquePtr
	  */
	template<class T>
	class SharedPtr {

		template<class Other>
		friend class SharedPtr;

	public:

		typedef T ElementType;

	public:

		SharedPtr() throw()
				: m_ptr(0),
				m_count(0) {
			//...//
		}

		explicit SharedPtr(T *const ptr)
				: m_ptr(ptr) {
			try {
				m_count = new ::TunnelEx::Helpers::SharedPtrHolder<T>(m_ptr);
			} catch (...) {
				delete m_ptr;
				throw;
			}
		}

		template<class Other>
		explicit SharedPtr(::TunnelEx::UniquePtr<Other> &uniquePtr)
				: m_ptr(uniquePtr.Get()) {
			try {
				m_count = new ::TunnelEx::Helpers::SharedPtrHolder<T>(m_ptr);
			} catch (...) {
				throw;
			}
			uniquePtr.Release();
		}

		~SharedPtr() {
			if (m_count) {
				m_count->RemoveRef();
			}
		}

		SharedPtr(const SharedPtr<T> &rhs) throw()
				: m_ptr(rhs.m_ptr),
				m_count(rhs.m_count) {
			if (m_count != 0) {
				m_count->AddRef();
			}
		}

		template<class Other>
		SharedPtr(const SharedPtr<Other> &rhs) throw()
				: m_ptr(rhs.m_ptr),
				m_count(rhs.m_count) {
			if (m_count != 0) {
				m_count->AddRef();
			}
		}

		SharedPtr & operator =(const SharedPtr<T> &rhs)  throw() {
			SharedPtr<T>(rhs).Swap(*this);
			return *this;
		}

		template<class Other>
		SharedPtr & operator =(const SharedPtr<Other> &rhs) throw() {
			SharedPtr<T>(rhs).Swap(*this);
			return *this;
		}

		template<class Other>
		SharedPtr & operator =(::TunnelEx::UniquePtr<Other> &uniquePtr) throw() {
			SharedPtr<T>(uniquePtr).Swap(*this);
			return *this;
		}

		void Reset() throw() {
			SharedPtr<T>().Swap(*this);
		}

		void Reset(T *const ptr) {
			SharedPtr<T>(ptr).Swap(*this);
		}

		T & operator *() const throw() {
			return *m_ptr;
		}

		T * operator ->() const throw() {
			return m_ptr;
		}

		T * Get() const throw() {
			return m_ptr;
		}

		operator bool() const throw() {
			return m_ptr != 0;
		}

		bool operator !() const throw() {
			return m_ptr == 0;
		}

		void Swap(SharedPtr<T> &rhs) throw() {
			T *const tmpPtr = m_ptr;
			::TunnelEx::Helpers::SharedPtrHolderBase *const tmpCount = m_count;
			m_ptr = rhs.m_ptr;
			m_count = rhs.m_count;
			rhs.m_ptr = tmpPtr;
			rhs.m_count = tmpCount;
		}

	private:

		T *m_ptr;
		::TunnelEx::Helpers::SharedPtrHolderBase *m_count;

	};

	template<class T, class U>
	inline bool operator ==(
				::TunnelEx::SharedPtr<T> const &a,
				::TunnelEx::SharedPtr<U> const &b)
			throw() {
		return a.Get() == b.Get();
	}

	template<class T, class U>
	inline bool operator !=(
				const ::TunnelEx::SharedPtr<T> &a,
				const ::TunnelEx::SharedPtr<U> &b)
			throw() {
		return a.Get() != b.Get();
	}

	template<class T, class U>
	inline bool operator <(
				const ::TunnelEx::SharedPtr<T> &a,
				const ::TunnelEx::SharedPtr<U> &b)
			throw() {
		return a.Get() < b.Get();
	}

	template<class T, class U>
	inline bool operator >(
				const ::TunnelEx::SharedPtr<T> &a,
				const ::TunnelEx::SharedPtr<U> &b)
			throw() {
		return a.Get() > b.Get();
	}

}

#endif // INCLUDED_FILE__TUNNELEX__SharedPtr_h__0803201737
