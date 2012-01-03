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
#include "TunnelBuffer.hpp"
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

	~Implementation() {
		//...//
	}

public:

	AutoPtr<MessageBlock> CreateMessageBlock(
				size_t size,
				const char *data = nullptr)
			const {
		
		if (!m_buffer) {
			const_cast<Implementation *>(this)->CreateNewMessageBlockBuffer(1);
			assert(m_buffer);
		}

		assert(size > 0);
		AutoPtr<UniqueMessageBlockHolder> result(new UniqueMessageBlockHolder);
		for (bool isError = false; ; ) {
			try {
				result->Reset(
					&UniqueMessageBlockHolder::Create(
						size,
						m_bufferAllocators,
						false,
						m_buffer));
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
				const_cast<Implementation *>(this)->CreateNewMessageBlockBuffer(2);
				assert(m_buffer);
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

	void CreateNewMessageBlockBuffer(size_t ratio) {
		// no locking, under server lock
		boost::shared_ptr<TunnelBuffer> buffer(new TunnelBuffer);
		//! @todo: hardcode, get MTU, see TEX-542 [2010/01/20 21:18]
		const auto dataBlockSize = TunnelBuffer::DefautDataBlockSize;
 		const auto messageBlockQueueBufferSize =
			(TunnelBuffer::DefautConnectionBufferSize * ratio)
				/ UniqueMessageBlockHolder::GetMessageMemorySize(dataBlockSize);
		m_bufferAllocators = buffer->CreateBuffer(
			messageBlockQueueBufferSize,
			messageBlockQueueBufferSize,
			UniqueMessageBlockHolder::GetMessageMemorySize(dataBlockSize));
		buffer.swap(m_buffer);
	}

public:

	const RuleEndpoint &m_ruleEndpoint;
	const SharedPtr<const EndpointAddress> m_ruleEndpointAddress;
	
	mutable boost::shared_ptr<TunnelBuffer> m_buffer;
	mutable TunnelBuffer::Allocators m_bufferAllocators;

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
