/**************************************************************************
 *   Created: 2011/07/14 23:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Unique_hpp__1107142308
#define INCLUDED_FILE__TUNNELEX__Unique_hpp__1107142308

#include "LocalAssert.h"

namespace TunnelEx {

	namespace Helpers {

		//! Abstract pointer holder for TunnelEx::Unique.
		class UniqueHolderBase {
		public:
			UniqueHolderBase() throw() {
				//...//
			}
		protected:
			virtual ~UniqueHolderBase() throw() {
				//...//
			}
		private:
			UniqueHolderBase(const UniqueHolderBase &);
			const UniqueHolderBase & operator =(const UniqueHolderBase &);
		public:
			virtual void Release() throw() = 0;
			virtual void Delete() throw() = 0;
		};

		//! Pointer holder implementation for TunnelEx::Unique.
		template<typename ElementT, ElementT defaultElement, typename DtorT>
		class UniqueHolder : public ::TunnelEx::Helpers::UniqueHolderBase {
		public:
			typedef ElementT Element;
			typedef DtorT Dtor;
		public:
			explicit UniqueHolder(Element element, Dtor dtor) throw()
					: m_element(element),
					m_dtor(dtor) {
				//...//
			}
		protected:
			virtual ~UniqueHolder() {
				if (m_element != defaultElement) {
					m_dtor(m_element);
				}
			}
		public:
			virtual void Delete() throw() {
				delete this;
			}
			virtual void Release() throw() {
				assert(m_element != defaultElement);
				m_element = defaultElement;
			}
		private:
			Element m_element;
			Dtor m_dtor;
		};

		//! Proxy reference for TunnelEx::Unique copying.
		/** @sa: TunnelEx::Unique
		*/
		template<typename ElementT, typename DtorT>
		struct UniqueRef {
			typedef ElementT Element;
			typedef DtorT Dtor;
			explicit UniqueRef(
						Element element,
						Dtor dtor,
						::TunnelEx::Helpers::UniqueHolderBase *const holder = 0)
					: m_element(element),
					m_dtor(dtor),
					m_holder(holder) {
				//...//
			}
			Element m_element;
			Dtor m_dtor;
			::TunnelEx::Helpers::UniqueHolderBase *m_holder;
		};

	}

	//! Smart pointer with std::auto_ptr semantic.
	/** Uses in case of work with TunnelEx's core pointers.
	  * @sa TunnelEx::Shared
	  */
	template<
		typename ElementT,
		typename DefaultElementT,
		DefaultElementT defaultElement>
	class Unique {

		template<
			typename Other,
			typename OtherDefault,
			OtherDefault defaultElement>
		friend class Unique;

	public:

		typedef ElementT ElementType;
		typedef DefaultElementT DefaultElementType;
		typedef Unique<ElementType, DefaultElementType, defaultElement> Self;

	public:

		Unique() throw()
				: m_element(defaultElement),
				m_holder(0) {
			//...//
		}

		template<typename Dtor>
		explicit Unique(ElementType element, Dtor dtor)
				: m_element(element) {
			InitHolder(dtor);
		}

	public:

		Unique(Self &rhs) throw()
				: m_element(rhs.m_element),
				m_holder(rhs.m_holder) {
			rhs.m_holder = 0;
			rhs.m_element = defaultElement;
		}

		template<typename Dtor>
		Unique(::TunnelEx::Helpers::UniqueRef<ElementType, Dtor> ref)
				: m_element(ref.element),
				m_holder(ref.holder) {
			if (m_element != defaultElement && m_holder == 0) {
				InitHolder(ref.dtor);
			}
		}

		template<typename Other, typename OtherDefault, OtherDefault otherDefaultElement>
		Unique(Unique<Other, OtherDefault, otherDefaultElement> &rhs) throw()
				: m_element(rhs.m_element),
				m_holder(rhs.m_holder) {
			rhs.m_holder = 0;
			rhs.m_element = defaultElement;
		}

		~Unique() {
			m_holder->Delete();
		}

	public:

		template<typename Other, typename OtherDefault, OtherDefault otherDefaultElement>
		const Self & operator =(Unique<Other, OtherDefault, otherDefaultElement> &rhs) throw() {
			Self(rhs).Swap(*this);
			return *this;
		}

		const Self & operator =(Self &rhs) throw() {
			Self(rhs).Swap(*this);
			return *this;
		}

		template<typename Other, typename Dtor>
		operator ::TunnelEx::Helpers::UniqueRef<Other, Dtor>() throw() {
			const ::TunnelEx::Helpers::UniqueRef<Other, Dtor> result(m_element, m_dtor, m_holder);
			m_holder = 0;
			m_element = defaultElement;
			return result;
		}

		template<typename Other, typename Dtor>
		const Self & operator =(::TunnelEx::Helpers::UniqueRef<Other, Dtor> ref) throw() {
			Self(ref).Swap(*this);
			return *this;
		}

		operator bool() const throw() {
			return m_element != defaultElement;
		}

		bool operator !() const throw() {
			return m_element == defaultElement;
		}

	public:

		ElementType Get() const throw() {
			return m_element;
		}

		void Reset() {
			Self().Swap(*this);
		}

		template<typename Dtor>
		void Reset(ElementType element, Dtor dtor) {
			Self(element, dtor).Swap(*this);
		}

		ElementType Release() throw() {
			ElementType tmpElement = m_element;
			if (m_holder) {
				m_holder->Release();
				Self().Swap(*this);
			}
			return tmpElement;
		}

		void Swap(Self &rhs) throw() {
			ElementType tmpElement = rhs.m_element;
			::TunnelEx::Helpers::UniqueHolderBase *tmpHolder = rhs.m_holder;
			rhs.m_element = m_element;
			rhs.m_holder = m_holder;
			m_element = tmpElement;
			m_holder = tmpHolder;
		}

	private:

		template<typename Dtor>
		void InitHolder(Dtor dtor) {
			assert(m_holder == 0);
			try {
				typedef ::TunnelEx::Helpers::UniqueHolder<
						ElementType,
						defaultElement,
						Dtor>
					Holder;
				m_holder = new Holder(m_element, dtor);
			} catch (...) {
				assert(false);
				dtor(m_element);
				throw;
			}
		}

	private:

		ElementType m_element;
		::TunnelEx::Helpers::UniqueHolderBase *m_holder;

	};

}

////////////////////////////////////////////////////////////////////////////////

template<
	typename T,
	typename TDefault,
	TDefault tDefaultElement,
	typename U,
	typename UDefault,
	UDefault uDefaultElement>
inline bool operator ==(
			const ::TunnelEx::Unique<T, TDefault, tDefaultElement> &a,
			const ::TunnelEx::Unique<U, UDefault, uDefaultElement> &b) {
	assert(&a != &b);
	return a.Get() == b.Get();
}

template<
	typename T,
	typename TDefault,
	TDefault tDefaultElement,
	typename U,
	typename UDefault,
	UDefault uDefaultElement>
inline bool operator !=(
			const ::TunnelEx::Unique<T, TDefault, tDefaultElement> &a,
			const ::TunnelEx::Unique<U, UDefault, uDefaultElement> &b) {
	assert(&a != &b);
	return a.Get() != b.Get();
}

template<
	typename T,
	typename TDefault,
	TDefault tDefaultElement,
	typename U,
	typename UDefault,
	UDefault uDefaultElement>
inline bool operator <(
			const ::TunnelEx::Unique<T, TDefault, tDefaultElement> &a,
			const ::TunnelEx::Unique<U, UDefault, uDefaultElement> &b) {
	assert(&a != &b);
	return a.Get() < b.Get();
}

template<
	typename T,
	typename TDefault,
	TDefault tDefaultElement,
	typename U,
	typename UDefault,
	UDefault uDefaultElement>
inline bool operator >(
			const ::TunnelEx::Unique<T, TDefault, tDefaultElement> &a,
			const ::TunnelEx::Unique<U, UDefault, uDefaultElement> &b) {
	assert(&a != &b);
	return a.Get() > b.Get();
}

////////////////////////////////////////////////////////////////////////////////

#endif // INCLUDED_FILE__TUNNELEX__Unique_hpp__1107142308
