/**************************************************************************
 *   Created: 2008/01/12 3:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.h"

#include "Filter.hpp"
#include "Locking.hpp"
#include "Rule.hpp"

using namespace TunnelEx;

//////////////////////////////////////////////////////////////////////////

class Filter::Implementation : private boost::noncopyable {

public:

	Implementation(	Filter &myInterface,
					SharedPtr<TunnelRule> rule,
					SharedPtr<RecursiveMutex> ruleChangingMutex)
		: m_myInterface(myInterface)
		, m_rule(rule)
		, m_ruleChangingMutex(ruleChangingMutex) {
		/*...*/
	}

	~Implementation() {
		/*...*/
	}

private:

	Filter &m_myInterface;
	SharedPtr<TunnelRule> m_rule;
	SharedPtr<RecursiveMutex> m_ruleChangingMutex;

public:

	void ScheduleRuleChange() {
		Lock lock(*m_ruleChangingMutex);
		m_myInterface.ChangeRule(*m_rule);
	}

	UniquePtr<Lock> LockRule() {
		return UniquePtr<Lock>(new Lock(*m_ruleChangingMutex));
	}

	const TunnelRule& GetRule() const {
		return *m_rule;
	}

};

//////////////////////////////////////////////////////////////////////////


#pragma warning(push)
#pragma warning(disable: 4355)
Filter::Filter(SharedPtr<TunnelRule> rule, SharedPtr<RecursiveMutex> ruleChangingMutex)
: m_pimpl(new Implementation(*this, rule, ruleChangingMutex)) {
	/*...*/
}
#pragma warning(pop)

Filter::~Filter() {
	delete m_pimpl;
}

void Filter::ScheduleRuleChange() {
	m_pimpl->ScheduleRuleChange();
}

UniquePtr<Lock> Filter::LockRule() {
	return m_pimpl->LockRule();
}

const TunnelRule& Filter::GetRule() const {
	return m_pimpl->GetRule();
}

//////////////////////////////////////////////////////////////////////////