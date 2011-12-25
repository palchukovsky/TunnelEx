/**************************************************************************
 *   Created: 2008/06/19 6:40
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "Acceptor.hpp"
#include "MessageBlockHolder.hpp"

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

AutoPtr<MessageBlock> Acceptor::CreateMessageBlock(
			size_t size,
			const char *data /*= nullptr*/)
		const {
	assert(size > 0);
	AutoPtr<UniqueMessageBlockHolder> result(new UniqueMessageBlockHolder);
	//! @todo: reimplement, memory usage!!!
	// not using internal allocator for memory, so buffer can be with any size
	result->Reset(&UniqueMessageBlockHolder::Create(size, false));
	result->SetReceivingTimePoint();
	if (data && result->Get().copy(data, size) == -1) {
		throw TunnelEx::InsufficientMemoryException(
			L"Insufficient message block memory");
	}
	return result;
}

const RuleEndpoint & Acceptor::GetRuleEndpoint() const {
	return m_pimpl->m_ruleEndpoint;
}

SharedPtr<const EndpointAddress> Acceptor::GetRuleEndpointAddress() const {
	return m_pimpl->m_ruleEndpointAddress;
}

//////////////////////////////////////////////////////////////////////////
