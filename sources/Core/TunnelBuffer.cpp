/**************************************************************************
 *   Created: 2010/01/09 2:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: TunnelBuffer.cpp 959 2010-06-22 11:36:04Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "TunnelBuffer.hpp"

using namespace TunnelEx;

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

	// all operation with memory protected by StateMutex, so no locking needed here
	typedef ACE_Null_Mutex BlocksAllocatorMutex;
	typedef ACE_Cached_Allocator<
			ACE_Message_Block,
			BlocksAllocatorMutex>
		MessageBlocksAllocator;
	typedef ACE_Cached_Allocator<
			ACE_Data_Block,
			BlocksAllocatorMutex>
		DataBlocksAllocator;
	typedef ACE_Dynamic_Cached_Allocator<
			BlocksAllocatorMutex>
		DataBlocksBufferAllocator;

	std::auto_ptr<ACE_Allocator> messageBlocksAllocator(
		new MessageBlocksAllocator(messageBlocksCount));
	std::auto_ptr<ACE_Allocator> dataBlocksAllocator(
		new DataBlocksAllocator(messageBlocksCount));
	std::auto_ptr<ACE_Allocator> dataBlocksBufferAllocator(
		new DataBlocksBufferAllocator(dataBlocksCount, dataBlockSize));

	Allocators result;
	result.messageBlock = messageBlocksAllocator.get();
	result.dataBlock = dataBlocksAllocator.get();
	result.dataBlockBuffer = dataBlocksBufferAllocator.get();
	
	AllAllocators allocators(m_allocators);
	allocators.insert(result.messageBlock);
	allocators.insert(result.dataBlock);
	allocators.insert(result.dataBlockBuffer);
	m_allocators.swap(allocators);

	messageBlocksAllocator.release();
	dataBlocksAllocator.release();
	dataBlocksBufferAllocator.release();

	return result;

}

void TunnelBuffer::DeleteBuffer(const Allocators &allocators) throw()  {
	if (	!allocators.messageBlock
			&& !allocators.dataBlock
			&& !allocators.dataBlockBuffer) {
		return;
	}
	BOOST_ASSERT(allocators.messageBlock && allocators.dataBlock && allocators.dataBlockBuffer);
	AllAllocators allAllocators(m_allocators);
	allAllocators.erase(allocators.messageBlock);
	allAllocators.erase(allocators.dataBlock);
	allAllocators.erase(allocators.dataBlockBuffer);
	m_allocators.swap(allAllocators);
	delete allocators.messageBlock;
	delete allocators.dataBlock;
	delete allocators.dataBlockBuffer;
}
