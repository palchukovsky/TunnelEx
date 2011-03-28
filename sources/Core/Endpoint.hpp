/**************************************************************************
 *   Created: 2007/06/12 15:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Endpoint.hpp 949 2010-06-06 05:56:16Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__NetworkEndPoint_h__0706121520
#define INCLUDED_FILE__NetworkEndPoint_h__0706121520

#include "String.hpp"
#include "SharedPtr.hpp"
#include "Time.h"
#include "Collection.hpp"
#include "Exceptions.hpp"
#include "UniquePtr.hpp"
#include "Api.h"

namespace TunnelEx {

	class EndpointAddress;
	class Endpoint;
	class RuleEndpoint;

	//////////////////////////////////////////////////////////////////////////

	//! Collection of endpoints.
	typedef ::TunnelEx::Collection<::TunnelEx::Endpoint> EndpointCollection;
	//! Collection of rule-endpoints.
	typedef ::TunnelEx::Collection<::TunnelEx::RuleEndpoint> RuleEndpointCollection;

	//////////////////////////////////////////////////////////////////////////

	//! Tunnel endpoint.
	/** Stores information about endpoint. */
	class TUNNELEX_CORE_API Endpoint {

	public:

		enum Acceptor {
			ACCEPTOR_NONE,
			ACCEPTOR_READER,
			ACCEPTOR_WRITER
		};

	public:
		
		Endpoint();
		
		explicit Endpoint(
				const ::TunnelEx::WString &combinedResourceIdentifier,
				bool isAcceptor);
		
		explicit Endpoint(
				const ::TunnelEx::WString &readResourceIdentifier,
				const ::TunnelEx::WString &writeResourceIdentifier,
				::TunnelEx::Endpoint::Acceptor acceptor);

		explicit Endpoint(
				::TunnelEx::UniquePtr<::TunnelEx::EndpointAddress> combinedAddress,
				bool isAcceptor);

		explicit Endpoint(
				::TunnelEx::UniquePtr<::TunnelEx::EndpointAddress> readAddress,
				::TunnelEx::UniquePtr<::TunnelEx::EndpointAddress> writeAddress,
				::TunnelEx::Endpoint::Acceptor acceptor);

		~Endpoint() throw();

		Endpoint(const Endpoint &);

		const Endpoint & operator =(const Endpoint &);

		void Swap(Endpoint &) throw();
		
	public:

		bool IsCombined() const;

		//! Sets combined resource identifier and changes endpoint type to "combined".
		void SetCombinedResourceIdentifier(
				const ::TunnelEx::WString &,
				bool isAcceptor);
	
		//! Sets read and write resource identifiers and changes endpoint type to "split".
		void SetReadWriteResourceIdentifiers(
				const ::TunnelEx::WString &readResourceIdentifier,
				const ::TunnelEx::WString &writeResourceIdentifier,
				::TunnelEx::Endpoint::Acceptor acceptor);


		//! Returns combined resource identifier.
		const ::TunnelEx::WString & GetCombinedResourceIdentifier()
			const
			throw(::TunnelEx::EndpointAddressTypeMismatchException);

		//! Returns read resource identifier.
		const ::TunnelEx::WString & GetReadResourceIdentifier()
			const
			throw(::TunnelEx::EndpointAddressTypeMismatchException);
		//! Returns write resource identifier.
		const ::TunnelEx::WString & GetWriteResourceIdentifier()
			const
			throw(::TunnelEx::EndpointAddressTypeMismatchException);

		//! Sets combined address implementation and changes endpoint type to "combined".
		void SetCombinedAddress(
				::TunnelEx::UniquePtr<::TunnelEx::EndpointAddress>,
				bool isAcceptor);

		//! Sets read and write address implementations identifiers and changes endpoint type to "split".
		void SetReadWriteAddresses(
				::TunnelEx::UniquePtr<::TunnelEx::EndpointAddress> readAddress,
				::TunnelEx::UniquePtr<::TunnelEx::EndpointAddress> writeAddress,
				::TunnelEx::Endpoint::Acceptor acceptor);


		//! Returns combined address implementation.
		/** Creates address object if it does not created yet.
		  * @return	nil if address does not set.
		  */
		::TunnelEx::SharedPtr<const ::TunnelEx::EndpointAddress> GetCombinedAddress()
			const
			throw(
				::TunnelEx::InvalidLinkException,
				::TunnelEx::EndpointAddressTypeMismatchException);
		::TunnelEx::SharedPtr<::TunnelEx::EndpointAddress> GetCombinedAddress()
			throw(
				::TunnelEx::InvalidLinkException,
				::TunnelEx::EndpointAddressTypeMismatchException);

		//! Returns read addresses implementation.
		/** Creates address object if it does not created yet.
		  * @return	nil if address does not set.
		  */
		::TunnelEx::SharedPtr<const ::TunnelEx::EndpointAddress> GetReadAddress()
			const
			throw(
				::TunnelEx::InvalidLinkException,
				::TunnelEx::EndpointAddressTypeMismatchException);
		::TunnelEx::SharedPtr<::TunnelEx::EndpointAddress> GetReadAddress()
			throw(
				::TunnelEx::InvalidLinkException,
				::TunnelEx::EndpointAddressTypeMismatchException);

		//! Returns write addresses implementation.
		/** Creates address object if it does not created yet.
		  * @return	nil if address does not set.
		  */
		::TunnelEx::SharedPtr<const ::TunnelEx::EndpointAddress> GetWriteAddress() const
			throw(
				::TunnelEx::InvalidLinkException,
				::TunnelEx::EndpointAddressTypeMismatchException);
		::TunnelEx::SharedPtr<::TunnelEx::EndpointAddress> GetWriteAddress()
			throw(
				::TunnelEx::InvalidLinkException,
				::TunnelEx::EndpointAddressTypeMismatchException);


		//! Returns combined typed address.
		template<class T>
		const T & GetCombinedTypedAddress()
				const
				throw(
					::TunnelEx::InvalidLinkException,
					::TunnelEx::EndpointAddressTypeMismatchException) {
			return (const_cast<Endpoint *>(this))->GetCombinedTypedAddress<typename T>();
		}
		template<class T>
		T & GetCombinedTypedAddress()
				throw(
					::TunnelEx::InvalidLinkException,
					::TunnelEx::EndpointAddressTypeMismatchException) {
			T *const result = dynamic_cast<T *>(GetCombinedAddress().Get());
			if (result == 0) {
				throw ::TunnelEx::EndpointAddressTypeMismatchException(
					L"Endpoint address type mismatch");
			}
			return *result;
		}

		//! Returns read typed address.
		template<class T>
		const T & GetReadTypedAddress()
				const
				throw(
					::TunnelEx::InvalidLinkException,
					::TunnelEx::EndpointAddressTypeMismatchException) {
			return (const_cast<Endpoint *>(this))->GetReadTypedAddress<typename T>();
		}
		template<class T>
		T & GetReadTypedAddress()
				throw(
					::TunnelEx::InvalidLinkException,
					::TunnelEx::EndpointAddressTypeMismatchException) {
			T *const result = dynamic_cast<T *>(GetReadAddress().Get());
			if (result == 0) {
				throw ::TunnelEx::EndpointAddressTypeMismatchException(
					L"Endpoint address type mismatch");
			}
			return *result;
		}

		//! Returns write typed address.
		template<class T>
		const T & GetWriteTypedAddress()
				const
				throw(
					::TunnelEx::InvalidLinkException,
					::TunnelEx::EndpointAddressTypeMismatchException) {
			return (const_cast<Endpoint *>(this))->GetWriteTypedAddress<typename T>();
		}
		template<class T>
		T & GetWriteTypedAddress()
				throw(
					::TunnelEx::InvalidLinkException,
					::TunnelEx::EndpointAddressTypeMismatchException) {
			T *const result = dynamic_cast<T *>(GetWriteAddress().Get());
			if (result == 0) {
				throw ::TunnelEx::EndpointAddressTypeMismatchException(
					L"Endpoint address type mismatch");
			}
			return *result;
		}

		template<class T>
		bool CheckCombinedAddressType()
				const
				throw(::TunnelEx::EndpointAddressTypeMismatchException) {
			return dynamic_cast<const T *>(GetCombinedAddress().Get()) != 0;
		}
		template<class T>
		bool CheckReadAddressType()
				const
				throw(::TunnelEx::EndpointAddressTypeMismatchException) {
			return dynamic_cast<const T *>(GetReadAddress().Get()) != 0;
		}
		template<class T>
		bool CheckWriteAddressType()
				const
				throw(::TunnelEx::EndpointAddressTypeMismatchException) {
			return dynamic_cast<const T *>(GetWriteAddress().Get()) != 0;
		}

		bool IsCombinedAcceptor()
			const
			throw(::TunnelEx::EndpointAddressTypeMismatchException);

		::TunnelEx::Endpoint::Acceptor GetReadWriteAcceptor()
			const 
			throw(::TunnelEx::EndpointAddressTypeMismatchException);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

	//////////////////////////////////////////////////////////////////////////

	class TUNNELEX_CORE_API RuleEndpoint : public ::TunnelEx::Endpoint {

	public:

		struct ListenerInfo {
			::TunnelEx::WString name;
			::TunnelEx::WString param;
		};

		typedef ::TunnelEx::Collection<ListenerInfo> Listeners;

	public:
		
		explicit RuleEndpoint(const ::TunnelEx::WString *uuid = NULL);
		explicit RuleEndpoint(
				const ::TunnelEx::WString &resourceIdentifier,
				bool isAcceptor,
				const ::TunnelEx::WString *uuid = NULL);
		explicit RuleEndpoint(
				const ::TunnelEx::UniquePtr<::TunnelEx::EndpointAddress> address,
				bool isAcceptor,
				const ::TunnelEx::WString *uuid = NULL);
		explicit RuleEndpoint(
				const ::TunnelEx::WString &readResourceIdentifier,
				const ::TunnelEx::WString &writeResourceIdentifier,
				::TunnelEx::Endpoint::Acceptor acceptor,
				const ::TunnelEx::WString *uuid = NULL);
		explicit RuleEndpoint(
				::TunnelEx::UniquePtr<::TunnelEx::EndpointAddress> readAddress,
				::TunnelEx::UniquePtr<::TunnelEx::EndpointAddress> writeAddress,
				::TunnelEx::Endpoint::Acceptor acceptor,
				const ::TunnelEx::WString *uuid = NULL);
		virtual ~RuleEndpoint() throw();

		RuleEndpoint(const RuleEndpoint &);
		const RuleEndpoint & operator =(const RuleEndpoint &);

		const Listeners & GetPreListeners() const;
		Listeners & GetPreListeners();
		const Listeners & GetPostListeners() const;
		Listeners & GetPostListeners();

		::TunnelEx::TimeSeconds GetOpenTimeout() const;

		const ::TunnelEx::WString & GetUuid() const;
		
		void Swap(RuleEndpoint &) throw();

		RuleEndpoint MakeCopy() const;

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}

#endif // INCLUDED_FILE__NetworkEndPoint_h__0706121520
