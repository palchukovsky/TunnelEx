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
			m_ruleEndpointAddress(ruleEndpointAddress),
			//! @todo: hardcode, get MTU, see TEX-542 [2010/01/20 21:18]
			m_dataBlockSize(MessagesAllocator::DefautDataBlockSize) {
		//...//
	}

	~Implementation() {
		//...//
	}

public:

	AutoPtr<MessageBlock> CreateMessageBlock(
				size_t size,
				const char *data = nullptr)
			const {
		
		if (!m_allocator) {
			const_cast<Implementation *>(this)->CreateNewAllocator(1, m_dataBlockSize);
			assert(m_allocator);
		} else if (size > m_dataBlockSize) {
			const_cast<Implementation *>(this)->CreateNewAllocator(1, size);
			assert(m_allocator);
		}

		assert(size > 0);
		AutoPtr<UniqueMessageBlockHolder> result(new UniqueMessageBlockHolder);
		for (bool isError = false; ; ) {
			try {
				result->Reset(
					&UniqueMessageBlockHolder::Create(size, m_allocator, false));
				break;
			} catch (const TunnelEx::InsufficientMemoryException &ex) {
				if (Log::GetInstance().IsDebugRegistrationOn()) {
					Log::GetInstance().AppendDebug(
						"Failed to create accepting buffer: %1%.",
						ConvertString<String>(ex.GetWhat()).GetCStr());
				}
				assert(!isError);
				if (isError) {
					throw;
				}
				const_cast<Implementation *>(this)
					->CreateNewAllocator(2, m_dataBlockSize);
				assert(m_allocator);
				isError = true;
			}
		}
		result->SetReceivingTimePoint();
		
		if (data && result->Get().copy(data, size) == -1) {
			throw TunnelEx::InsufficientMemoryException(
				L"Insufficient message block memory");
		}

		return result;

	}

private:

	void CreateNewAllocator(size_t ratio, size_t dataBlockSize) {
		// no locking, under server lock
		const auto messageBlockQueueBufferSize
			= (MessagesAllocator::DefautConnectionBufferSize * ratio)
				/ UniqueMessageBlockHolder::GetMessageMemorySize(dataBlockSize);
		boost::shared_ptr<MessagesAllocator> allocator(
			new MessagesAllocator(
				messageBlockQueueBufferSize,
				messageBlockQueueBufferSize,
				UniqueMessageBlockHolder::GetMessageMemorySize(dataBlockSize)));
		allocator.swap(m_allocator);
	}

public:

	const RuleEndpoint &m_ruleEndpoint;
	const SharedPtr<const EndpointAddress> m_ruleEndpointAddress;
	
	boost::shared_ptr<MessagesAllocator> m_allocator;
	size_t m_dataBlockSize;

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
