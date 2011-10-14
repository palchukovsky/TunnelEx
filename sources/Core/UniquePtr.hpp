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

#include "LocalAssert.h"

namespace TunnelEx {

	namespace Helpers {

		//! Abstract pointer holder for TunnelEx::UniquePtr.
		class UniquePtrHolderBase {
		public:
			UniquePtrHolderBase() throw() {
				//...//
			}
		protected:
			virtual ~UniquePtrHolderBase() throw() {
				//...//
			}
		private:
			UniquePtrHolderBase(const UniquePtrHolderBase &);
			const UniquePtrHolderBase & operator =(const UniquePtrHolderBase &);
		public:
			virtual void Release() throw() = 0;
			virtual void Delete() throw() = 0;
		};

		//! Pointer holder implementation for TunnelEx::UniquePtr.
		template<class T>
		class UniquePtrHolder : public ::TunnelEx::Helpers::UniquePtrHolderBase {
		public:
			explicit UniquePtrHolder(T *const ptr) throw()
					: m_ptr(ptr) {
				//...//
			}
		protected:
			virtual ~UniquePtrHolder() {
				delete m_ptr;
			}
		public:
			virtual void Release() throw() {
				m_ptr = 0;
			}
			virtual void Delete() throw() {
				delete this;
			}
		private:
			T *m_ptr;
		};

		//! Proxy reference for TunnelEx::UniquePtr copying.
		/** @sa: TunnelEx::UniquePtr
		*/
		template<class Y>
		struct UniquePtrRef {	
			explicit UniquePtrRef(
						Y *ptr,
						::TunnelEx::Helpers::UniquePtrHolderBase *const holder = 0)
					: m_ptr(ptr),
					m_holder(holder) {
				//...//
			}
			Y *m_ptr;
			::TunnelEx::Helpers::UniquePtrHolderBase *m_holder;
		};

	}

	//! Smart pointer with std::auto_ptr semantic.
	/** Uses in case of work with TunnelEx's core pointers.
	  * @sa TunnelEx::SharedPtr
	  */
	template<class T>
	class UniquePtr {

		template<class Other>
		friend class UniquePtr;

	public:

		typedef T ElementType;

	public:

		UniquePtr() throw()
				: m_ptr(0),
				m_holder(0) {
			//...//
		}

		explicit UniquePtr(T *const ptr)
				: m_ptr(ptr) {
			InitHolder();
		}

		UniquePtr(UniquePtr<T> &rhs) throw()
				: m_ptr(rhs.m_ptr),
				m_holder(rhs.m_holder) {
			rhs.m_holder = 0;
			rhs.m_ptr = 0;
		}

		UniquePtr(::TunnelEx::Helpers::UniquePtrRef<T> rhs)
				: m_ptr(rhs.m_ptr),
				m_holder(rhs.m_holder) {
			if (m_ptr != 0 && m_holder == 0) {
				InitHolder();
			}
		}

		template<class Other>
		UniquePtr(UniquePtr<Other> &rhs) throw()
				: m_ptr(rhs.m_ptr),
				m_holder(rhs.m_holder) {
			rhs.m_holder = 0;
			rhs.m_ptr = 0;
		}

		~UniquePtr() {
			if (m_holder) {
				m_holder->Delete();
			}
		}

	public:

		template<class Other>
		const UniquePtr<T> & operator =(UniquePtr<Other> &rhs) throw() {
			UniquePtr<T>(rhs).Swap(*this);
			return *this;
		}

		const UniquePtr<T> & operator =(UniquePtr<T> &rhs) throw() {
			UniquePtr<T>(rhs).Swap(*this);
			return *this;
		}

		template<class Other>
		operator ::TunnelEx::Helpers::UniquePtrRef<Other>() throw() {
			const ::TunnelEx::Helpers::UniquePtrRef<Other> result(m_ptr, m_holder);
			m_holder = 0;
			m_ptr = 0;
			return result;
		}

		const UniquePtr<T> & operator =(
					::TunnelEx::Helpers::UniquePtrRef<T> rhs)
				throw() {
			UniquePtr<T>(rhs).Swap(*this);
			return *this;
		}

		T & operator *() const throw() {
			assert(m_ptr != 0);
			return *m_ptr;
		}

		T * operator ->() const throw() {
			return m_ptr;
		}

		operator bool() const throw() {
			return m_ptr != 0;
		}

		bool operator !() const throw() {
			return m_ptr == 0;
		}

		T * Get() const throw() {
			return m_ptr;
		}

		void Reset() {
			UniquePtr<T>().Swap(*this);
		}

		void Reset(T *const ptr) {
			UniquePtr<T>(ptr).Swap(*this);
		}

		T * Release() throw() {
			T *const tmpPtr = m_ptr;
			if (m_holder) {
				m_holder->Release();
				UniquePtr().Swap(*this);
			}
			return tmpPtr;
		}

		void Swap(UniquePtr<T> &rhs) throw() {
			T *const tmpPtr = rhs.m_ptr;
			::TunnelEx::Helpers::UniquePtrHolderBase *tmpHolder = rhs.m_holder;
			rhs.m_ptr = m_ptr;
			rhs.m_holder = m_holder;
			m_ptr = tmpPtr;
			m_holder = tmpHolder;
		}

	private:

		void InitHolder() {
			try {
				m_holder = new ::TunnelEx::Helpers::UniquePtrHolder<T>(m_ptr);
			} catch (...) {
				delete m_ptr;
				throw;
			}
		}

	private:

		T *m_ptr;
		::TunnelEx::Helpers::UniquePtrHolderBase *m_holder;

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
