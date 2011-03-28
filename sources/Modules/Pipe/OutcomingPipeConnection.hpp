/**************************************************************************
 *   Created: 2008/11/27 3:16
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 * -------------------------------------------------------------------
 *       $Id: OutcomingPipeConnection.hpp 959 2010-06-22 11:36:04Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__OutcomingPipeConnection_hpp__0811270316
#define INCLUDED_FILE__TUNNELEX__OutcomingPipeConnection_hpp__0811270316

#include "PipeConnection.hpp"
#include "PipeEndpointAddress.hpp"

#include <TunnelEx/Endpoint.hpp>
#include <TunnelEx/Log.hpp>
#include <TunnelEx/Exceptions.hpp>
#include <TunnelEx/Error.hpp>


namespace TunnelEx { namespace Mods { namespace Pipe {

	class OutcomingPipeConnection : public PipeConnection {

	public:

		explicit OutcomingPipeConnection(
					const PipeEndpointAddress &address,
					const RuleEndpoint &ruleEndpoint,
					SharedPtr<const EndpointAddress> ruleEndpointAddress)
				throw(ConnectionOpeningException)
				: PipeConnection(
					ruleEndpoint,
					ruleEndpointAddress,
					OpenConnection(address, ruleEndpoint)) {
			LogTracking("OutcomingPipeConnection", "OutcomingPipeConnection", __FILE__, __LINE__);
		}

		~OutcomingPipeConnection() {
			LogTracking("OutcomingPipeConnection", "~OutcomingPipeConnection", __FILE__, __LINE__);
		}

	private:

		std::auto_ptr<ACE_SPIPE_Stream> OpenConnection(
					const PipeEndpointAddress &address,
					const TunnelEx::RuleEndpoint &ruleEndpoint)
				const
				throw(ConnectionOpeningException) {
			LogTracking("OutcomingPipeConnection", "OpenConnection", __FILE__, __LINE__);
			std::auto_ptr<ACE_SPIPE_Stream> stream(new ACE_SPIPE_Stream);
			ACE_SPIPE_Connector connector;
			ACE_Time_Value timeout(ruleEndpoint.GetOpenTimeout());
			const int connectResult = connector.connect(
				*stream, address.GetAcePipeAddr(), &timeout, ACE_Addr::sap_any, 0,
				O_RDWR | FILE_FLAG_OVERLAPPED);
			if (connectResult != 0) {
				const Error error(errno);
				LogTracking("OutcomingPipeConnection", "OpenConnection", __FILE__, __LINE__);
				WFormat message(L"Could not open pipe: \"%1% (%2%)\".");
				message % error.GetString().GetCStr() % error.GetErrorNo();
				throw ConnectionOpeningException(message.str().c_str());
			}
			return stream;
		}

	};

} } }

#endif // INCLUDED_FILE__TUNNELEX__OutcomingPipeConnection_hpp__0811270316