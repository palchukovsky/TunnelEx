/**************************************************************************
 *   Created: 2007/06/12 15:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__NetworkEndPoint_h__0706121520
#define INCLUDED_FILE__NetworkEndPoint_h__0706121520

#include "String.hpp"
#include "Time.h"
#include "Collection.hpp"
#include "Exceptions.hpp"
#include "SmartPtr.hpp"
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
				::TunnelEx::AutoPtr<::TunnelEx::EndpointAddress> combinedAddress,
				bool isAcceptor);

		explicit Endpoint(
				::TunnelEx::AutoPtr<::TunnelEx::EndpointAddress> readAddress,
				::TunnelEx::AutoPtr<::TunnelEx::EndpointAddress> writeAddress,
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
		/** @throw TunnelEx::EndpointAddressTypeMismatchException
		  */
		const ::TunnelEx::WString & GetCombinedResourceIdentifier() const;

		//! Returns read resource identifier.
		/** @throw TunnelEx::EndpointAddressTypeMismatchException
		  */
		const ::TunnelEx::WString & GetReadResourceIdentifier() const;
			
		//! Returns write resource identifier.
		/** @throw TunnelEx::EndpointAddressTypeMismatchException
		  */
		const ::TunnelEx::WString & GetWriteResourceIdentifier() const;

		//! Sets combined address implementation and changes endpoint type to "combined".
		void SetCombinedAddress(
				::TunnelEx::AutoPtr<::TunnelEx::EndpointAddress>,
				bool isAcceptor);

		//! Sets read and write address implementations identifiers and changes endpoint type to "split".
		void SetReadWriteAddresses(
				::TunnelEx::AutoPtr<::TunnelEx::EndpointAddress> readAddress,
				::TunnelEx::AutoPtr<::TunnelEx::EndpointAddress> writeAddress,
				::TunnelEx::Endpoint::Acceptor acceptor);


		//! Returns combined address implementation.
		/** Creates address object if it does not created yet.
		  * @throw TunnelEx::InvalidLinkException
		  * @throw TunnelEx::EndpointAddressTypeMismatchException
		  * @return	nil if address does not std::set.
		  */
		::TunnelEx::SharedPtr<const ::TunnelEx::EndpointAddress> GetCombinedAddress() const;
		::TunnelEx::SharedPtr<::TunnelEx::EndpointAddress> GetCombinedAddress();

		//! Returns read addresses implementation.
		/** Creates address object if it does not created yet.
		  * @throw TunnelEx::InvalidLinkException
		  * @throw TunnelEx::EndpointAddressTypeMismatchException
		  * @return	nil if address does not std::set.
		  */
		::TunnelEx::SharedPtr<const ::TunnelEx::EndpointAddress> GetReadAddress() const;
		::TunnelEx::SharedPtr<::TunnelEx::EndpointAddress> GetReadAddress();

		//! Returns write addresses implementation.
		/** Creates address object if it does not created yet.
		  * @throw TunnelEx::InvalidLinkException
		  * @throw TunnelEx::EndpointAddressTypeMismatchException
		  * @return	nil if address does not std::set.
		  */
		::TunnelEx::SharedPtr<const ::TunnelEx::EndpointAddress> GetWriteAddress() const;
		::TunnelEx::SharedPtr<::TunnelEx::EndpointAddress> GetWriteAddress();


		//! Returns combined typed address.
		/** @throw TunnelEx::InvalidLinkException
		  * @throw TunnelEx::EndpointAddressTypeMismatchException
		  */
		template<class T>
		const T & GetCombinedTypedAddress() const {
			return (const_cast<Endpoint *>(this))->GetCombinedTypedAddress<typename T>();
		}
		template<class T>
		T & GetCombinedTypedAddress() {
			T *const result = dynamic_cast<T *>(GetCombinedAddress().Get());
			if (result == 0) {
				throw ::TunnelEx::EndpointAddressTypeMismatchException(
					L"Endpoint address type mismatch");
			}
			return *result;
		}

		//! Returns read typed address.
		/** @throw TunnelEx::InvalidLinkException
		  * @throw TunnelEx::EndpointAddressTypeMismatchException
		  */
		template<class T>
		const T & GetReadTypedAddress() const {
			return (const_cast<Endpoint *>(this))->GetReadTypedAddress<typename T>();
		}
		template<class T>
		T & GetReadTypedAddress() {
			T *const result = dynamic_cast<T *>(GetReadAddress().Get());
			if (result == 0) {
				throw ::TunnelEx::EndpointAddressTypeMismatchException(
					L"Endpoint address type mismatch");
			}
			return *result;
		}

		//! Returns write typed address.
		/** @throw TunnelEx::InvalidLinkException
		  * @throw TunnelEx::EndpointAddressTypeMismatchException
		  */
		template<class T>
		const T & GetWriteTypedAddress() const {
			return (const_cast<Endpoint *>(this))->GetWriteTypedAddress<typename T>();
		}
		template<class T>
		T & GetWriteTypedAddress() {
			T *const result = dynamic_cast<T *>(GetWriteAddress().Get());
			if (result == 0) {
				throw ::TunnelEx::EndpointAddressTypeMismatchException(
					L"Endpoint address type mismatch");
			}
			return *result;
		}

		/** @throw TunnelEx::EndpointAddressTypeMismatchException
		  */
		template<class T>
		bool CheckCombinedAddressType() const {
			return dynamic_cast<const T *>(GetCombinedAddress().Get()) != 0;
		}
		template<class T>
		bool CheckReadAddressType() const {
			return dynamic_cast<const T *>(GetReadAddress().Get()) != 0;
		}
		template<class T>
		bool CheckWriteAddressType() const {
			return dynamic_cast<const T *>(GetWriteAddress().Get()) != 0;
		}

		/** @throw TunnelEx::EndpointAddressTypeMismatchException
		  */
		bool IsCombinedAcceptor() const;

		/** @throw TunnelEx::EndpointAddressTypeMismatchException
		  */
		::TunnelEx::Endpoint::Acceptor GetReadWriteAcceptor() const;

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
				const ::TunnelEx::AutoPtr<::TunnelEx::EndpointAddress> address,
				bool isAcceptor,
				const ::TunnelEx::WString *uuid = NULL);
		explicit RuleEndpoint(
				const ::TunnelEx::WString &readResourceIdentifier,
				const ::TunnelEx::WString &writeResourceIdentifier,
				::TunnelEx::Endpoint::Acceptor acceptor,
				const ::TunnelEx::WString *uuid = NULL);
		explicit RuleEndpoint(
				::TunnelEx::AutoPtr<::TunnelEx::EndpointAddress> readAddress,
				::TunnelEx::AutoPtr<::TunnelEx::EndpointAddress> writeAddress,
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
