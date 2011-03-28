/**************************************************************************
 *   Created: 2010/06/06 23:51
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: TcpConnectionAcceptor.cpp 1084 2010-12-05 18:57:31Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "TcpConnectionAcceptor.hpp"

#include "Modules/Inet/InetEndpointAddress.hpp"

using namespace boost;
using namespace TunnelEx;
using namespace TunnelEx::Mods;
using namespace TunnelEx::Mods::Upnp;

TcpConnectionAcceptor::TcpConnectionAcceptor(
			unsigned short externalPort,
			const Inet::InetEndpointAddress &inetAddress,
			const RuleEndpoint &ruleEndpoint,
			SharedPtr<const EndpointAddress> ruleEndpointAddress)
		: Base(inetAddress, ruleEndpoint, ruleEndpointAddress) {

	const UniquePtr<EndpointAddress> openedAddress = Base::GetLocalAddress();

	ServiceRule::Service service;
	service.uuid = Helpers::Uuid().GetAsString().c_str();
	service.name = L"Upnpc";
	service.param = UpnpcService::CreateParam(
		Client::PROTO_TCP,
		externalPort,
		polymorphic_downcast<Inet::InetEndpointAddress *>(
			openedAddress.Get())->GetHostName(),
		polymorphic_downcast<Inet::InetEndpointAddress *>(
			openedAddress.Get())->GetPort(),
		true, // @todo: see TEX-610
		false);

	SharedPtr<ServiceRule> rule(new ServiceRule);
	// WString ruleUuid = rule->GetUuid();
	rule->GetServices().Append(service);

	//! @todo: see TEX-611 [2010/06/07 1:39]
	/* m_server.UpdateRule(rule);
	ruleUuid.Swap(m_upnpRuleUuid); */

	m_upnpcService.reset(new UpnpcService(rule, rule->GetServices()[0]));
	m_upnpcService->Start();

}

TcpConnectionAcceptor::~TcpConnectionAcceptor() throw() {
	//...//
}
