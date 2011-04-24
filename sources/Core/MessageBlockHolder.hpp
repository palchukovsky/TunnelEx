
#ifndef MESSAGE_BLOCK_ADAPTER_H_INCLUDED
#define MESSAGE_BLOCK_ADAPTER_H_INCLUDED

#include "MessageBlock.hpp"
#include "Exceptions.hpp"

namespace TunnelEx {

	//////////////////////////////////////////////////////////////////////////

	class UniqueMessageBlockHolder : public MessageBlock {

	private:

		enum Flag {
			FLAG_TUNNEL_MESSAGE = ACE_Message_Block::USER_FLAGS
		};

	public:

		explicit UniqueMessageBlockHolder(ACE_Message_Block *messageBlock) throw() 
				: m_messageBlock(messageBlock),
				m_isAddedToQueue(false) {
			//...//
		}

		explicit UniqueMessageBlockHolder(ACE_Message_Block &messageBlock) throw() 
				: m_messageBlock(&messageBlock),
				m_isAddedToQueue(false) {
			//...//
		}

		virtual ~UniqueMessageBlockHolder() throw() {
			if (Test()) {
				Delete(*m_messageBlock);
			}
		}

	public:

		void Release() throw() {
			assert(Test());
			m_messageBlock = 0;
		}

		bool Test() const throw() {
			return m_messageBlock != 0;
		}

	public:

		ACE_Message_Block & Get() throw() {
			assert(Test());
			return *m_messageBlock;
		}

		const ACE_Message_Block & Get() const throw() {
			return const_cast<UniqueMessageBlockHolder *>(this)->Get();
		}

	public:

		virtual const char * GetData() const throw() {
			assert(Test());
			return m_messageBlock->rd_ptr();
		}

		virtual size_t GetUnreadedDataSize() const throw() {
			assert(Test());
			return m_messageBlock->length();
		}

		virtual void SetData(const char *data, size_t length){
			assert(Test());
			std::auto_ptr<ACE_Message_Block> newBlock(
				new ACE_Message_Block(
					length,
					ACE_Message_Block::MB_DATA,
					0,
					0,
					0,
					0,
					ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
					ACE_Time_Value::zero,
					ACE_Time_Value::max_time,
					0,
					0));
			assert(!(newBlock->data_block()->size () < length));
			if (newBlock->data_block()->size () < length) {
				// using SystemException instead InsufficientMemoryException
				// as using internal memory buffer, not system.
				throw InsufficientMemoryException(
					L"Could not allocate new buffer for message block - memory insufficient");
			}
			newBlock->set_flags(FLAG_TUNNEL_MESSAGE);
			if (newBlock->copy(data, length) == -1) {
				throw TunnelEx::InsufficientMemoryException(
					L"Insufficient message block memory");
			}
			Delete(*m_messageBlock);
			m_messageBlock = newBlock.release();			
		}

		virtual void MarkAsAddedToQueue() throw() {
			assert(Test());
			assert(!m_isAddedToQueue);
			m_isAddedToQueue = true;
		}
		virtual bool IsAddedToQueue() const throw() {
			assert(Test());
			return m_isAddedToQueue;
		}

		virtual bool IsTunnelMessage() const throw() {
			assert(Test());
			return m_messageBlock->flags() & FLAG_TUNNEL_MESSAGE ? true : false;
		}

	public:

		//! @todo: use here std::unique_ptr with custom destroyer
		static ACE_Message_Block & CreateMessageBlockForTunnel(
					size_t size,
					ACE_Allocator &messageBlocksAllocator,
					ACE_Allocator &dataBlocksAllocator,
					ACE_Allocator &dataBlocksBufferAllocator) {
			ACE_Message_Block *const result
				= static_cast<ACE_Message_Block *>(
					messageBlocksAllocator.malloc(sizeof(ACE_Message_Block)));
			assert(result != 0);
			if (result == 0) {
				// using SystemException instead InsufficientMemoryException
				// as using internal memory buffer, not system.
				throw SystemException(
					L"Could not allocate new message block - insufficient internal memory buffer (internal error)");
			}
			(void)new (result) ACE_Message_Block(
				size,
				ACE_Message_Block::MB_DATA,
				0,
				0,
				&dataBlocksBufferAllocator,
				0,
				ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
				ACE_Time_Value::zero,
				ACE_Time_Value::max_time,
				&dataBlocksAllocator,
				&messageBlocksAllocator);
			assert(!(result->data_block()->size () < size));
			if (result->data_block()->size () < size) {
				Delete(*result);
				// using SystemException instead InsufficientMemoryException
				// as using internal memory buffer, not system.
				throw SystemException(
					L"Could not allocate new buffer for message block - insufficient internal memory buffer (internal error)");
			}
			result->set_flags(FLAG_TUNNEL_MESSAGE);
			return *result;
		}

		static void Delete(ACE_Message_Block &messageBlock) throw() {
			ACE_Allocator *messageBlocksAllocator = 0;
			ACE_Allocator *dataBlocksAllocator = 0;
			ACE_Allocator *dataBlocksBufferAllocator = 0;
			messageBlock.access_allocators(
				dataBlocksBufferAllocator,
				dataBlocksAllocator,
				messageBlocksAllocator);
			if (messageBlocksAllocator) {
				assert(messageBlock.flags() & FLAG_TUNNEL_MESSAGE);
				assert(dataBlocksAllocator);
				assert(dataBlocksBufferAllocator);
				messageBlock.~ACE_Message_Block();
				messageBlocksAllocator->free(&messageBlock);
			} else {
				delete &messageBlock;
			}
		}

	private:

		ACE_Message_Block *m_messageBlock;
		bool m_isAddedToQueue;

	};

	//////////////////////////////////////////////////////////////////////////

}

#endif // #ifndef MESSAGE_BLOCK_ADAPTER_H_INCLUDED
