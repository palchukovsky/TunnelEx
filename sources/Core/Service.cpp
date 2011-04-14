/**************************************************************************
 *   Created: 2010/06/02 22:05
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Service.cpp 959 2010-06-22 11:36:04Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "Service.hpp"
#include "Log.hpp"

using namespace TunnelEx;

//////////////////////////////////////////////////////////////////////////

class Service::Implementation : private boost::noncopyable {

public:

	Implementation(
				SharedPtr<const ServiceRule> rule,
				const ServiceRule::Service &service)
			: m_rule(rule),
			m_service(service),
			m_isStated(false) {
		//...//
	}

public:

	const SharedPtr<const ServiceRule> m_rule;
	const ServiceRule::Service &m_service;
	bool m_isStated;

};


//////////////////////////////////////////////////////////////////////////

Service::Service(
			SharedPtr<const ServiceRule> rule,
			const ServiceRule::Service &service)
		: m_pimpl(new Implementation(rule, service)) {
	//...//
}

Service::~Service() throw() {
	delete m_pimpl;
}

const ServiceRule & Service::GetRule() const {
	return *m_pimpl->m_rule;
}

const ServiceRule::Service & Service::GetService() const {
	return m_pimpl->m_service;
}

bool Service::IsStarted() const throw() {
	return m_pimpl->m_isStated;
}

void Service::SetStarted(bool isStarted) throw() {
	m_pimpl->m_isStated = isStarted;
	if (Log::GetInstance().IsDebugRegistrationOn()) {
		Log::GetInstance().AppendDebug(
			"%1% service %2%: %3%.",
			ConvertString<String>(GetService().name).GetCStr(),
			ConvertString<String>(GetService().uuid).GetCStr(),
			IsStarted() ? "started" : "stopped");

	}
}
