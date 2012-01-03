/**************************************************************************
 *   Created: 2010/01/09 2:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "Locking.hpp"
#include "TunnelBuffer.hpp"
#include "MessageBlockHolder.hpp"

using namespace TunnelEx;

//////////////////////////////////////////////////////////////////////////

// IPv4 links must be able to forward packets of size up to 68 bytes.
// Systems may use Path MTU Discovery[4] to find the actual path MTU.
// This should not be mistaken with the packet size every host must be able
// to handle, which is 576.[5] (http://en.wikipedia.org/wiki/Maximum_transmission_unit)
const size_t TunnelBuffer::DefautDataBlockSize = 576;
const size_t TunnelBuffer::DefautConnectionBufferSize = 150 * 1024;

//////////////////////////////////////////////////////////////////////////

TunnelBuffer::Allocators::Allocators()
		: messageBlock(nullptr),
		messageBlockSatellite(nullptr),
		dataBlock(nullptr),
		dataBlockBuffer(nullptr) {
	//...//
}

//////////////////////////////////////////////////////////////////////////

TunnelBuffer::TunnelBuffer() {
	//...//
}

TunnelBuffer::~TunnelBuffer() {
	foreach (ACE_Allocator *allocator, m_allocators) {
		delete allocator;
	}
}

TunnelBuffer::Allocators TunnelBuffer::CreateBuffer(
			size_t messageBlocksCount,
			size_t dataBlocksCount,
			size_t dataBlockSize) {

	class Mutex : private boost::noncopyable {
	public:
		int acquire() {
			m_impl.Acquire();
			return 0;
		}
		int release() {
			m_impl.Release();
			return 0;
		}
	private:
		SpinMutex m_impl;
	};

	typedef ACE_Cached_Allocator<ACE_Message_Block, Mutex> MessageBlocksAllocator;
	typedef ACE_Cached_Allocator<
			UniqueMessageBlockHolder::Satellite,
			Mutex>
		MessageBlockSatellitesAllocator;
	typedef ACE_Cached_Allocator<ACE_Data_Block, Mutex> DataBlocksAllocator;
	typedef ACE_Dynamic_Cached_Allocator<Mutex> DataBlocksBufferAllocator;

	std::unique_ptr<ACE_Allocator> messageBlocksAllocator(
		new MessageBlocksAllocator(messageBlocksCount));
	std::unique_ptr<ACE_Allocator> messageBlockSattelitesAllocator(
		new MessageBlockSatellitesAllocator(messageBlocksCount));
	std::unique_ptr<ACE_Allocator> dataBlocksAllocator(
		new DataBlocksAllocator(messageBlocksCount));
	std::unique_ptr<ACE_Allocator> dataBlocksBufferAllocator(
		new DataBlocksBufferAllocator(dataBlocksCount, dataBlockSize));

	Allocators result;
	result.messageBlock = messageBlocksAllocator.get();
	result.messageBlockSatellite = messageBlockSattelitesAllocator.get();
	result.dataBlock = dataBlocksAllocator.get();
	result.dataBlockBuffer = dataBlocksBufferAllocator.get();
	
	AllAllocators allocators(m_allocators);
	allocators.insert(result.messageBlock);
	allocators.insert(result.messageBlockSatellite);
	allocators.insert(result.dataBlock);
	allocators.insert(result.dataBlockBuffer);
	m_allocators.swap(allocators);

	messageBlocksAllocator.release();
	messageBlockSattelitesAllocator.release();
	dataBlocksAllocator.release();
	dataBlocksBufferAllocator.release();

	return result;

}

void TunnelBuffer::DeleteBuffer(const Allocators &allocators) throw()  {
	if (allocators.messageBlock) {
		return;
	}
	assert(
		allocators.messageBlock
		&& allocators.messageBlockSatellite
		&& allocators.dataBlock
		&& allocators.dataBlockBuffer);
	AllAllocators allAllocators(m_allocators);
	allAllocators.erase(allocators.messageBlock);
	allAllocators.erase(allocators.messageBlockSatellite);
	allAllocators.erase(allocators.dataBlock);
	allAllocators.erase(allocators.dataBlockBuffer);
	m_allocators.swap(allAllocators);
	delete allocators.messageBlock;
	delete allocators.messageBlockSatellite;
	delete allocators.dataBlock;
	delete allocators.dataBlockBuffer;
}
