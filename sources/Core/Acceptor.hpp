/**************************************************************************
 *   Created: 2008/07/12 10:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Acceptor_hpp__0807121047
#define INCLUDED_FILE__TUNNELEX__Acceptor_hpp__0807121047

#include "IoHandle.h"
#include "SharedPtr.hpp"
#include "UniquePtr.hpp"
#include "Instance.hpp"
#include "Api.h"

namespace TunnelEx {

	class Connection;
	class RuleEndpoint;
	class EndpointAddress;
	class ConnectionOpeningException;

	class TUNNELEX_CORE_API Acceptor : public ::TunnelEx::Instance {

	public:

		explicit Acceptor(
				const ::TunnelEx::RuleEndpoint &ruleEndpoint,
				::TunnelEx::SharedPtr<const ::TunnelEx::EndpointAddress> ruleEndpointAddress);
		virtual ~Acceptor() throw();

	private:

		Acceptor(const Acceptor &);
		const Acceptor & operator =(const Acceptor &);

	public:

		//! Accepts incoming connection and creates object for it.
		/** Endpoint open timeout value must be used.
		  * @sa RuleEndpoint::GetOpenTimeout
		  * @throw TunnelEx::ConnectionOpeningException
		  * @return accepted connection object;
		  */
		virtual ::TunnelEx::UniquePtr<::TunnelEx::Connection> Accept() = 0;

		//! Attaches connection to existing tunnel.
		/** Tries to attach incoming connection to existing tunnel and
		  * returns true on success. If tunnel exists, but connection
		  * opening fails - can throws exceptions.
		  * @throw TunnelEx::LocalException
		  */
		virtual bool TryToAttach() = 0;

		virtual ::TunnelEx::UniquePtr<::TunnelEx::EndpointAddress> GetLocalAddress()
			const
			= 0;

		const ::TunnelEx::RuleEndpoint & GetRuleEndpoint() const;
		::TunnelEx::SharedPtr<const ::TunnelEx::EndpointAddress> GetRuleEndpointAddress() const;
		
		virtual ::TunnelEx::IoHandleInfo GetIoHandle() = 0;

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}

#endif // INCLUDED_FILE__TUNNELEX__Acceptor_hpp__0807121047
