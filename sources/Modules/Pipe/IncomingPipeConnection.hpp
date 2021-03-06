/**************************************************************************
 *   Created: 2008/11/27 2:54
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__IncomingPipeConnection_hpp__0811270254
#define INCLUDED_FILE__TUNNELEX__IncomingPipeConnection_hpp__0811270254

#include "PipeConnection.hpp"

#include "Core/Endpoint.hpp"
#include "Core/Error.hpp"
#include "Core/Log.hpp"
#include "Core/Exceptions.hpp"

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
			if (acceptor.accept(*stream, 0, &timeout) == 0) {
				return stream;
			}
			const Error error(errno);
			if (!error.IsError()) {
				throw ConnectionOpeningGracefullyCanceled();
			}
			WFormat message(L"Failed to accept incoming pipe connection: \"%1% (%2%)\".");
			message % error.GetStringW() % error.GetErrorNo();
			throw ConnectionOpeningException(message.str().c_str());
		}

	};

} } }

#endif // INCLUDED_FILE__TUNNELEX__IncomingPipeConnection_hpp__0811270254