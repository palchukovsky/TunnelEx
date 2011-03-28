/**************************************************************************
 *   Created: 2008/11/27 2:54
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 * -------------------------------------------------------------------
 *       $Id: IncomingPipeConnection.hpp 1129 2011-02-22 17:28:50Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__IncomingPipeConnection_hpp__0811270254
#define INCLUDED_FILE__TUNNELEX__IncomingPipeConnection_hpp__0811270254

#include "PipeConnection.hpp"

#include <TunnelEx/Endpoint.hpp>
#include <TunnelEx/Error.hpp>
#include <TunnelEx/Log.hpp>
#include <TunnelEx/Exceptions.hpp>

namespace TunnelEx { namespace Mods { namespace Pipe {

	class IncomingPipeConnection : public PipeConnection {

	public:

		explicit IncomingPipeConnection(
					const TunnelEx::RuleEndpoint &ruleEndpoint,
					SharedPtr<const EndpointAddress> ruleEndpointAddress,
					ACE_SPIPE_Acceptor &acceptor)
				: PipeConnection(
					ruleEndpoint,
					ruleEndpointAddress,
					AcceptConnection(acceptor, ruleEndpoint)) {
			//...//
		}

		~IncomingPipeConnection() throw() {
			//...//
		}

	private:
	
		std::auto_ptr<ACE_SPIPE_Stream> AcceptConnection(
					ACE_SPIPE_Acceptor &acceptor,
					const TunnelEx::RuleEndpoint &ruleEndpoint)
				const {
			std::auto_ptr<ACE_SPIPE_Stream> stream(new ACE_SPIPE_Stream);
			ACE_Time_Value timeout(ruleEndpoint.GetOpenTimeout());
			if (acceptor.accept(*stream, 0, &timeout) != 0) {
				const Error error(errno);
				WFormat message(L"Failed to accept incoming pipe connection: \"%1% (%2%)\".");
				message % error.GetString().GetCStr() % error.GetErrorNo();
				throw ConnectionOpeningException(message.str().c_str());
			}
			return stream;
		}

	};

} } }

#endif // INCLUDED_FILE__TUNNELEX__IncomingPipeConnection_hpp__0811270254