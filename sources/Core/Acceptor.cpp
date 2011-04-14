/**************************************************************************
 *   Created: 2008/06/19 6:40
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 * -------------------------------------------------------------------
 *       $Id: Acceptor.cpp 959 2010-06-22 11:36:04Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "Acceptor.hpp"

using namespace TunnelEx;

//////////////////////////////////////////////////////////////////////////

class Acceptor::Implementation : private boost::noncopyable {

public:

	explicit Implementation(
				const RuleEndpoint &ruleEndpoint, 
				SharedPtr<const EndpointAddress> ruleEndpointAddress)
			: m_ruleEndpoint(ruleEndpoint),
			m_ruleEndpointAddress(ruleEndpointAddress) {
		//...//
	}

	~Implementation() {
		//...//
	}

public:

	const RuleEndpoint &m_ruleEndpoint;
	const SharedPtr<const EndpointAddress> m_ruleEndpointAddress;

};

//////////////////////////////////////////////////////////////////////////

Acceptor::Acceptor(
			const RuleEndpoint &ruleEndpoint,
			SharedPtr<const EndpointAddress> ruleEndpointAddress)
		: m_pimpl(new Implementation(ruleEndpoint, ruleEndpointAddress)) {
	//...//
}

Acceptor::~Acceptor() throw() {
	delete m_pimpl;
}

const RuleEndpoint & Acceptor::GetRuleEndpoint() const {
	return m_pimpl->m_ruleEndpoint;
}

SharedPtr<const EndpointAddress> Acceptor::GetRuleEndpointAddress() const {
	return m_pimpl->m_ruleEndpointAddress;
}

//////////////////////////////////////////////////////////////////////////
