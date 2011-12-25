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
#include "SmartPtr.hpp"
#include "Instance.hpp"
#include "Api.h"

namespace TunnelEx {

	class Connection;
	class RuleEndpoint;
	class EndpointAddress;
	class ConnectionOpeningException;
	class MessageBlock;

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

		//! Creates a new message block.
		/** The message block should be created immediately after the data
		  * reading from system buffers. It is important for latency
		  * statistics.
		  */
		::TunnelEx::AutoPtr<::TunnelEx::MessageBlock> CreateMessageBlock(
				size_t size,
				const char *data = nullptr)
			const;

	public:

		//! Accepts incoming connection and creates object for it.
		/** Endpoint open timeout value must be used.
		  * @sa RuleEndpoint::GetOpenTimeout
		  * @throw TunnelEx::ConnectionOpeningException
		  * @return accepted connection object;
		  */
		virtual ::TunnelEx::AutoPtr<::TunnelEx::Connection> Accept() = 0;

		//! Attaches connection to existing tunnel.
		/** Tries to attach incoming connection to existing tunnel and
		  * returns true on success. If tunnel exists, but connection
		  * opening fails - can throws exceptions.
		  * @throw TunnelEx::LocalException
		  */
		virtual bool TryToAttach() = 0;

		virtual ::TunnelEx::AutoPtr<::TunnelEx::EndpointAddress> GetLocalAddress()
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
