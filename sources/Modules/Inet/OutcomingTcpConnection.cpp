/**************************************************************************
 *   Created: 2011/02/22 19:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "OutcomingTcpConnection.hpp"

using namespace TunnelEx;
using namespace TunnelEx::Mods::Inet;

OutcomingTcpConnection::OutcomingTcpConnection(
			const ACE_INET_Addr &address,
			const RuleEndpoint &ruleEndpoint,
			SharedPtr<const EndpointAddress> ruleEndpointAddress)
		: Base(ruleEndpoint, ruleEndpointAddress) {
	OpenConnection(address, ruleEndpoint);
}

OutcomingTcpConnection::~OutcomingTcpConnection() {
	//...//
}

void OutcomingTcpConnection::OpenConnection(
				const ACE_INET_Addr &address,
				const RuleEndpoint &ruleEndpoint) {
	std::auto_ptr<Stream> stream(new Stream);
	ACE_SOCK_Connector connector;
	const ACE_Time_Value timeout(ruleEndpoint.GetOpenTimeout());
	if (0 != connector.connect(*stream, address, &timeout, ACE_Addr::sap_any, 1)) {
		const Error error(errno);
		WFormat message(L"%1% (%2%)");
		message % error.GetStringW() % error.GetErrorNo();
		throw ConnectionOpeningException(message.str().c_str());
	}
	SetDataStream(stream);
}
