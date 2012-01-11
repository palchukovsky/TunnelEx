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
#include "MessagesAllocator.hpp"
#include "MessageBlockHolder.hpp"
#include "Log.hpp"

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

public:

	AutoPtr<MessageBlock> CreateMessageBlock(
				size_t size,
				const char *data = nullptr)
			const {

		assert(size > 0);
		
		if (!m_allocator) {
			const_cast<Implementation *>(this)->CreateNewAllocator(
				1,
				std::max(MessagesAllocator::DefautDataBlockSize, size));
		} else if (
				UniqueMessageBlockHolder::GetMessageMemorySize(size)
					> m_allocator->GetDataBlockSize()) {
			const_cast<Implementation *>(this)->CreateNewAllocator(1, size);
		}
		assert(m_allocator);

		AutoPtr<UniqueMessageBlockHolder> result(new UniqueMessageBlockHolder);
		for (bool isError = false; ; ) {
			result->Reset(
				UniqueMessageBlockHolder::Create(size, m_allocator, false));
			if (result->IsSet()) {
				result->SetReceivingTimePoint();
				if (!data || result->Get().copy(data, size) != -1) {
					return result;
				}
			}
			Log::GetInstance().AppendDebug(
				"Failed to create accepting buffer with size %1% bytes.",
				size);
			assert(!isError);
			if (isError) {
				throw TunnelEx::InsufficientMemoryException(
					L"Insufficient memory for accepting message block");
			}
			const_cast<Implementation *>(this)
				->CreateNewAllocator(
					2,
					m_allocator->GetDataBlockSize()
						- UniqueMessageBlockHolder::GetMessageMemorySize(0));
			assert(m_allocator);
			isError = true;
		}

	}

private:

	void CreateNewAllocator(size_t ratio, size_t dataBlockSize) {
		// no locking, under server lock
		const auto messageBlockQueueBufferSize
			= (MessagesAllocator::DefautConnectionBufferSize * ratio)
				/ UniqueMessageBlockHolder::GetMessageMemorySize(dataBlockSize);
		boost::shared_ptr<MessagesAllocator> allocator(
			new MessagesAllocator(
				messageBlockQueueBufferSize + 1, // +1 for proactor (duplicate method)
				messageBlockQueueBufferSize,
				UniqueMessageBlockHolder::GetMessageMemorySize(dataBlockSize)));
		allocator.swap(m_allocator);
	}

public:

	const RuleEndpoint &m_ruleEndpoint;
	const SharedPtr<const EndpointAddress> m_ruleEndpointAddress;
	
	boost::shared_ptr<MessagesAllocator> m_allocator;

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
	return m_pimpl->CreateMessageBlock(size, data);
}

const RuleEndpoint & Acceptor::GetRuleEndpoint() const {
	return m_pimpl->m_ruleEndpoint;
}

SharedPtr<const EndpointAddress> Acceptor::GetRuleEndpointAddress() const {
	return m_pimpl->m_ruleEndpointAddress;
}

//////////////////////////////////////////////////////////////////////////
