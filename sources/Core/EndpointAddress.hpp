/**************************************************************************
 *   Created: 2008/05/22 21:13
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__EndpointAddress_h__0805222113
#define INCLUDED_FILE__TUNNELEX__EndpointAddress_h__0805222113

#include "String.hpp"
#include "SmartPtr.hpp"
#include "Api.h"

namespace TunnelEx {

	class Acceptor;
	class RuleEndpoint;
	class Connection;
	class ConnectionOpeningException;
	class EndpointHasNotMultiClientsTypeException;

	class TUNNELEX_CORE_API EndpointAddress {

	public:
		
		EndpointAddress();
		EndpointAddress(const EndpointAddress &);
		virtual ~EndpointAddress() throw();

		EndpointAddress & operator =(const EndpointAddress &);

		void Swap(EndpointAddress &) throw();

	public:

		virtual const ::TunnelEx::WString & GetResourceIdentifier() const = 0;
		
		//! Returns true if endpoint has a Multi-Clients type.
		/** If endpoint has a Multi-Clients type the client can creates acceptors
		  * for it, listen it, accepts incoming connection and works with
		  * multiple connections at the same time. Otherwise the client can
		  * only open local connection and work with one connection at the
		  * same time.
		  * @sa OpenForIncomingConnections
		  * @sa CreateLocalConnection
		  */
		virtual bool IsHasMultiClientsType() const = 0;

		//! Creates connection acceptor.
		/** @sa IsHasMultiClientsType
		 *  @sa CreateLocalConnection
		 *	@throw TunnelEx::ConnectionOpeningException
		 *	@throw TunnelEx::EndpointHasNotMultiClientsTypeException
		 */
		virtual ::TunnelEx::AutoPtr<::TunnelEx::Acceptor> OpenForIncomingConnections(
				const ::TunnelEx::RuleEndpoint &ruleEndpoint,
				::TunnelEx::SharedPtr<const ::TunnelEx::EndpointAddress> ruleEndpointAddress)
			const
			= 0;

		//! Creates remote connection to this address.
		/** Endpoint connection timeout value must be used.
		  * @sa CreateLocalConnection
		  * @sa RuleEndpoint::GetOpenTimeout
		  * @throw TunnelEx::ConnectionOpeningException
		  */
		virtual ::TunnelEx::AutoPtr<::TunnelEx::Connection> CreateRemoteConnection(
				const ::TunnelEx::RuleEndpoint &ruleEndpoint,
				::TunnelEx::SharedPtr<const ::TunnelEx::EndpointAddress> ruleEndpointAddress) 
			const
			= 0;

		virtual bool IsReadyToRecreateLocalConnection() const = 0;

		virtual bool IsReadyToRecreateRemoteConnection() const = 0;
			
		//! Creates local connection to this address.
		/** @sa IsHasMultiClientsType
		 *  @sa OpenForIncomingConnections
		 *  @sa CreateRemoteConnection
		 *  @throw TunnelEx::ConnectionOpeningException
		 */
		virtual ::TunnelEx::AutoPtr<::TunnelEx::Connection> CreateLocalConnection(
				const ::TunnelEx::RuleEndpoint &ruleEndpoint,
				::TunnelEx::SharedPtr<const ::TunnelEx::EndpointAddress> ruleEndpointAddress) 
			const
			= 0;

		virtual ::TunnelEx::AutoPtr<EndpointAddress> Clone() const = 0;

	public:

		virtual void StatConnectionSetupCompleting() const throw();

		virtual void StatConnectionSetupCanceling() const throw();
		virtual void StatConnectionSetupCanceling(
				const ::TunnelEx::WString &reason)
			const
			throw();

	};

}

#endif // INCLUDED_FILE__TUNNELEX__EndpointAddress_h__0805222113
