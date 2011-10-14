/**************************************************************************
 *   Created: 2011/10/15 0:30
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__SmartPtr_hpp__1110150030
#define INCLUDED_FILE__TUNNELEX__SmartPtr_hpp__1110150030

#include "Api.h"
#include "Locking.hpp"
#include "LocalAssert.h"

////////////////////////////////////////////////////////////////////////////////

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
	class AutoPtr {

		template<class Other>
		friend class AutoPtr;

	public:

		typedef T ElementType;

	public:

		AutoPtr() throw()
				: m_ptr(0),
				m_holder(0) {
			//...//
		}

		explicit AutoPtr(T *const ptr)
				: m_ptr(ptr) {
			InitHolder();
		}

		AutoPtr(AutoPtr<T> &rhs) throw()
				: m_ptr(rhs.m_ptr),
				m_holder(rhs.m_holder) {
			rhs.m_holder = 0;
			rhs.m_ptr = 0;
		}

		AutoPtr(::TunnelEx::Helpers::UniquePtrRef<T> rhs)
				: m_ptr(rhs.m_ptr),
				m_holder(rhs.m_holder) {
			if (m_ptr != 0 && m_holder == 0) {
				InitHolder();
			}
		}

		template<class Other>
		AutoPtr(AutoPtr<Other> &rhs) throw()
				: m_ptr(rhs.m_ptr),
				m_holder(rhs.m_holder) {
			rhs.m_holder = 0;
			rhs.m_ptr = 0;
		}

		~AutoPtr() {
			if (m_holder) {
				m_holder->Delete();
			}
		}

	public:

		template<class Other>
		const AutoPtr<T> & operator =(AutoPtr<Other> &rhs) throw() {
			AutoPtr<T>(rhs).Swap(*this);
			return *this;
		}

		const AutoPtr<T> & operator =(AutoPtr<T> &rhs) throw() {
			AutoPtr<T>(rhs).Swap(*this);
			return *this;
		}

		template<class Other>
		operator ::TunnelEx::Helpers::UniquePtrRef<Other>() throw() {
			const ::TunnelEx::Helpers::UniquePtrRef<Other> result(m_ptr, m_holder);
			m_holder = 0;
			m_ptr = 0;
			return result;
		}

		const AutoPtr<T> & operator =(
					::TunnelEx::Helpers::UniquePtrRef<T> rhs)
				throw() {
			AutoPtr<T>(rhs).Swap(*this);
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
			AutoPtr<T>().Swap(*this);
		}

		void Reset(T *const ptr) {
			AutoPtr<T>(ptr).Swap(*this);
		}

		T * Release() throw() {
			T *const tmpPtr = m_ptr;
			if (m_holder) {
				m_holder->Release();
				AutoPtr().Swap(*this);
			}
			return tmpPtr;
		}

		void Swap(AutoPtr<T> &rhs) throw() {
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
			::TunnelEx::AutoPtr<T> const &a,
			::TunnelEx::AutoPtr<U> const &b) {
	assert(&a != &b);
	return a.Get() == b.Get();
}

template<typename T, typename U>
inline bool operator !=(
			const ::TunnelEx::AutoPtr<T> &a,
			const ::TunnelEx::AutoPtr<U> &b) {
	assert(&a != &b);
	return a.Get() != b.Get();
}

template<typename T, typename U>
inline bool operator <(
			const ::TunnelEx::AutoPtr<T> &a,
			const ::TunnelEx::AutoPtr<U> &b) {
	assert(&a != &b);
	return a.Get() < b.Get();
}

template<typename T, typename U>
inline bool operator >(
			const ::TunnelEx::AutoPtr<T> &a,
			const ::TunnelEx::AutoPtr<U> &b) {
	assert(&a != &b);
	return a.Get() > b.Get();
}

////////////////////////////////////////////////////////////////////////////////

namespace TunnelEx {

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
			virtual void Delete() {
				delete this;
			}
		private:
			const T *const m_ptr;
		};

	}

	//! Smart pointer with references counting.
	/** Uses in case of work with TunnelEx's core pointers.
	  * @sa TunnelEx::AutoPtr
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
		explicit SharedPtr(::TunnelEx::AutoPtr<Other> &autoPtr)
				: m_ptr(autoPtr.Get()) {
			try {
				m_count = new ::TunnelEx::Helpers::SharedPtrHolder<T>(m_ptr);
			} catch (...) {
				throw;
			}
			autoPtr.Release();
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
		SharedPtr & operator =(::TunnelEx::AutoPtr<Other> &autoPtr) throw() {
			SharedPtr<T>(autoPtr).Swap(*this);
			return *this;
		}

		void Reset() throw() {
			SharedPtr<T>().Swap(*this);
		}

		void Reset(T *const ptr) {
			SharedPtr<T>(ptr).Swap(*this);
		}

		T & operator *() const throw() {
			assert(m_ptr != 0);
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

}

template<class T, class U>
inline bool operator ==(
			::TunnelEx::SharedPtr<T> const &a,
			::TunnelEx::SharedPtr<U> const &b)
		throw() {
	assert(&a != &b);
	return a.Get() == b.Get();
}

template<class T, class U>
inline bool operator !=(
			const ::TunnelEx::SharedPtr<T> &a,
			const ::TunnelEx::SharedPtr<U> &b)
		throw() {
	assert(&a != &b);
	return a.Get() != b.Get();
}

template<class T, class U>
inline bool operator <(
			const ::TunnelEx::SharedPtr<T> &a,
			const ::TunnelEx::SharedPtr<U> &b)
		throw() {
	assert(&a != &b);
	return a.Get() < b.Get();
}

template<class T, class U>
inline bool operator >(
			const ::TunnelEx::SharedPtr<T> &a,
			const ::TunnelEx::SharedPtr<U> &b)
		throw() {
	assert(&a != &b);
	return a.Get() > b.Get();
}

////////////////////////////////////////////////////////////////////////////////

#endif // INCLUDED_FILE__TUNNELEX__SmartPtr_hpp__1110150030
