
#ifndef MESSAGE_BLOCK_ADAPTER_H_INCLUDED
#define MESSAGE_BLOCK_ADAPTER_H_INCLUDED

#include "MessageBlock.hpp"
#include "Exceptions.hpp"
#include "ObjectsDeletionCheck.h"

namespace TunnelEx {

	//////////////////////////////////////////////////////////////////////////

	class UniqueMessageBlockHolder : public MessageBlock {

	private:

		enum Flag {
			FLAG_TUNNEL_MESSAGE = ACE_Message_Block::USER_FLAGS,
			FLAG_HAS_TIMINGS
		};

	private:

		class Timings : private boost::noncopyable {

		public:
		
			Timings()
					: m_refsCount(0) {
				TUNNELEX_OBJECTS_DELETION_CHECK_CTOR(m_instancesNumber);
			}

			~Timings() {
				assert(m_refsCount == 0);
				TUNNELEX_OBJECTS_DELETION_CHECK_DTOR(m_instancesNumber);
			}

		public:

			long AddRef() throw() {
				return Interlocked::Increment(m_refsCount);
			}
			
			long RemoveRef() throw() {
				return Interlocked::Decrement(m_refsCount);
			}

		public:

			void SetReceivingStartTimePoint() {
				SetCurrentTimePoint(m_receivingStartTime);
			}

			void SetReceivingTimePoint() {
				SetCurrentTimePoint(m_receivingTime);
			}
			const boost::posix_time::ptime & GetReceivingTime() const {
				assert(!m_receivingTime.is_not_a_date_time());
				return m_receivingTime;
			}

			void SetSendingStartTimePoint() {
				SetCurrentTimePoint(m_sendingStartTime);
			}
			const boost::posix_time::ptime & GetSendingStartTime() const {
				assert(!m_sendingStartTime.is_not_a_date_time());
				return m_sendingStartTime;
			}

			void SetSendingTimePoint() {
				SetCurrentTimePoint(m_sendingTime);
			}

		public:

			boost::posix_time::time_duration GetReceivingLatency() const {
				assert(!m_receivingStartTime.is_not_a_date_time());
				assert(!m_receivingTime.is_not_a_date_time());
				assert(m_receivingStartTime <= m_receivingTime);
				return m_receivingTime - m_receivingStartTime;
			}

			boost::posix_time::time_duration GetSendingLatency() const {
				assert(!m_sendingStartTime.is_not_a_date_time());
				assert(!m_sendingTime.is_not_a_date_time());
				assert(m_sendingStartTime <= m_sendingTime);
				return m_sendingTime - m_sendingStartTime;
			}

			boost::posix_time::time_duration GetProcessingLatency() const {
				assert(!m_receivingTime.is_not_a_date_time());
				assert(!m_sendingStartTime.is_not_a_date_time());
				assert(m_receivingTime <= m_sendingStartTime);
				return m_sendingStartTime - m_receivingTime;
			}

			boost::posix_time::time_duration GetFullLatency() const {
				assert(!m_receivingTime.is_not_a_date_time());
				assert(!m_sendingTime.is_not_a_date_time());
				assert(m_receivingTime <= m_sendingTime);
				return m_sendingTime - m_receivingTime;
			}

		public:

#			ifdef DEV_VER
				static long GetInstancesNumber() {
					return m_instancesNumber;
				}
#			endif

		private:

			static void SetCurrentTimePoint(boost::posix_time::ptime &var) {
				assert(var.is_not_a_date_time());
				var = boost::posix_time::microsec_clock::local_time();
			}

		private:

			boost::posix_time::ptime m_receivingStartTime;
			boost::posix_time::ptime m_receivingTime;
			boost::posix_time::ptime m_sendingStartTime;
			boost::posix_time::ptime m_sendingTime;

			volatile long m_refsCount;

			TUNNELEX_OBJECTS_DELETION_CHECK_DECLARATION(m_instancesNumber);

		};

	public:

		explicit UniqueMessageBlockHolder() throw() 
				: m_messageBlock(nullptr),
				m_isAddedToQueue(false) {
			//...//
		}

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
			Reset();
		}

	public:

		void Reset(ACE_Message_Block *newMessageBlock = nullptr) throw() {
			if (IsSet()) {
				Delete(*m_messageBlock);
			}
			m_messageBlock = newMessageBlock;
		}

		void Release() throw() {
			assert(IsSet());
			m_messageBlock = nullptr;
		}

		bool IsSet() const throw() {
			return m_messageBlock ? true : false;
		}

	public:

		ACE_Message_Block & Get() throw() {
			assert(IsSet());
			return *m_messageBlock;
		}

		const ACE_Message_Block & Get() const throw() {
			return const_cast<UniqueMessageBlockHolder *>(this)->Get();
		}

	public:

		virtual const char * GetData() const throw() {
			assert(IsSet());
			return m_messageBlock->rd_ptr();
		}

		virtual char * GetWritableSpace(size_t size) {
			assert(IsSet());
			assert(m_messageBlock->space() >= size);
			if (m_messageBlock->space() < size) {
				// using SystemException instead InsufficientMemoryException
				// as using internal memory buffer, not system.
				throw InsufficientMemoryException(
					L"Could not allocate new buffer for message block - memory insufficient");
			}
			return m_messageBlock->wr_ptr();
		}
		virtual void TakeWritableSpace(size_t size) {
			assert(IsSet());
			assert(m_messageBlock->space() >= size);
			if (m_messageBlock->space() < size) {
				// using SystemException instead InsufficientMemoryException
				// as using internal memory buffer, not system.
				throw InsufficientMemoryException(
					L"Could not allocate new buffer for message block - memory insufficient");
			}
			m_messageBlock->wr_ptr(size);
		}

		virtual size_t GetUnreadedDataSize() const throw() {
			assert(IsSet());
			return m_messageBlock->length();
		}

		virtual void SetData(const char *data, size_t length){
			
			assert(IsSet());
			
			ACE_Allocator *messageBlocksAllocator = nullptr;
			ACE_Allocator *dataBlocksAllocator = nullptr;
			ACE_Allocator *dataBlocksBufferAllocator = nullptr;
			m_messageBlock->access_allocators(
				dataBlocksBufferAllocator,
				dataBlocksAllocator,
				messageBlocksAllocator);

			const bool hasTimings = m_messageBlock->flags() & FLAG_HAS_TIMINGS;
			assert(hasTimings);
			if (hasTimings) {
				length += sizeof(Timings *);
			}

			std::unique_ptr<ACE_Message_Block> newBlock(
				new ACE_Message_Block(
					length,
					ACE_Message_Block::MB_DATA,
					0,
					0,
					dataBlocksBufferAllocator,
					0,
					ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
					ACE_Time_Value::zero,
					ACE_Time_Value::max_time,
					dataBlocksAllocator,
					messageBlocksAllocator));
			assert(!(newBlock->data_block()->size() < length));

			if (hasTimings) {
				assert(m_messageBlock->space() >= sizeof(Timings *));
				if (newBlock->copy(m_messageBlock->base(), sizeof(Timings *)) == -1) {
					throw TunnelEx::InsufficientMemoryException(
						L"Insufficient message block memory");
				}
				newBlock->rd_ptr(sizeof(Timings *));
				newBlock->set_flags(FLAG_HAS_TIMINGS);
			}

			assert(m_messageBlock->space() >= length);
			if (newBlock->copy(data, length) == -1) {
				throw TunnelEx::InsufficientMemoryException(
					L"Insufficient message block memory");
			}
			newBlock->set_flags(FLAG_TUNNEL_MESSAGE);
			
			m_messageBlock->clr_flags(FLAG_HAS_TIMINGS);
			Delete(*m_messageBlock);
			m_messageBlock = newBlock.release();			
		
		}

		virtual void MarkAsAddedToQueue() throw() {
			assert(IsSet());
			assert(!m_isAddedToQueue);
			m_isAddedToQueue = true;
		}
		virtual bool IsAddedToQueue() const throw() {
			assert(IsSet());
			return m_isAddedToQueue;
		}

		virtual bool IsTunnelMessage() const throw() {
			assert(IsSet());
			return m_messageBlock->flags() & FLAG_TUNNEL_MESSAGE ? true : false;
		}

	public:

		template<typename T>
		static T GetMessageMemorySize(T blockSize) {
			return
				blockSize
					+ sizeof(Timings *)
					+ sizeof(ACE_Message_Block)
					+ sizeof(ACE_Data_Block);
		}

		ACE_Message_Block & Duplicate() {
			assert(IsSet());
			auto result = m_messageBlock->duplicate();
			assert(result);
			if (!result) {
				// using SystemException instead InsufficientMemoryException
				// as using internal memory buffer, not system.
				throw InsufficientMemoryException(
					L"Could not allocate new buffer for message block - memory insufficient");
			}
			if (m_messageBlock->flags() & FLAG_HAS_TIMINGS) {
				verify(GetTimings().AddRef() > 1);
			}
			return *result;
		}

		static ACE_Message_Block & Create(
					size_t size,
					ACE_Allocator &messageBlocksAllocator,
					ACE_Allocator &dataBlocksAllocator,
					ACE_Allocator &dataBlocksBufferAllocator,
					bool isTunnelMessage) {
			ACE_Message_Block &result = CreateObject(
				size,
				messageBlocksAllocator,
				dataBlocksAllocator,
				dataBlocksBufferAllocator);
			try {
				Init(isTunnelMessage, result);
			} catch (...) {
				Delete(result);
				throw;
			}
			return result;
		}

		static ACE_Message_Block & Create(size_t size, bool isTunnelMessage) {
			ACE_Message_Block &result = CreateObject(size);
			try {
				Init(isTunnelMessage, result);
			} catch (...) {
				Delete(result);
				throw;
			}
			return result;
		}

		static void Delete(ACE_Message_Block &messageBlock) throw() {
			
			ACE_Allocator *messageBlocksAllocator = 0;
			ACE_Allocator *dataBlocksAllocator = 0;
			ACE_Allocator *dataBlocksBufferAllocator = 0;
			messageBlock.access_allocators(
				dataBlocksBufferAllocator,
				dataBlocksAllocator,
				messageBlocksAllocator);

			if (
					messageBlock.flags() & FLAG_HAS_TIMINGS
					&& GetTimings(messageBlock).RemoveRef() == 0) {
				delete &GetTimings(messageBlock);
			}

			if (messageBlocksAllocator) {
				assert(dataBlocksAllocator);
				assert(dataBlocksBufferAllocator);
				messageBlock.~ACE_Message_Block();
				messageBlocksAllocator->free(&messageBlock);
			} else {
				delete &messageBlock;
			}
		
		}

	public:

		void SetReceivingStartTimePoint() {
			GetTimings().SetReceivingStartTimePoint();
		}

		void SetReceivingTimePoint() {
			GetTimings().SetReceivingTimePoint();
		}
		const boost::posix_time::ptime & GetReceivingTime() const {
			return GetTimings().GetReceivingTime();
		}

		void SetSendingStartTimePoint() {
			GetTimings().SetSendingStartTimePoint();
		}
		const boost::posix_time::ptime & GetSendingStartTime() const {
			return GetTimings().GetSendingStartTime();
		}

		void SetSendingTimePoint() {
			GetTimings().SetSendingTimePoint();
		}

#		ifdef DEV_VER
			static long GetTimingsInstancesNumber() {
				return Timings::GetInstancesNumber();
			}
#		endif

	public:

		boost::posix_time::time_duration GetReceivingLatency() const {
			return GetTimings().GetReceivingLatency();
		}

		boost::posix_time::time_duration GetSendingLatency() const {
			return GetTimings().GetSendingLatency();
		}

		boost::posix_time::time_duration GetProcessingLatency() const {
			return GetTimings().GetProcessingLatency();
		}

		boost::posix_time::time_duration GetFullLatency() const {
			return GetTimings().GetFullLatency();
		}

	private:

		static Timings & GetTimings(ACE_Message_Block &messageBlock) {
			assert(messageBlock.flags() & FLAG_HAS_TIMINGS);
			Timings *result;
			memcpy(&result, messageBlock.base(), sizeof(result));
			return *result;
		}
		Timings & GetTimings() {
			assert(IsSet());
			return GetTimings(*m_messageBlock);
		}
		const Timings & GetTimings() const {
			return const_cast<UniqueMessageBlockHolder *>(this)->GetTimings();
		}

	private:

		static ACE_Message_Block & CreateObject(
					size_t size,
					ACE_Allocator &messageBlocksAllocator,
					ACE_Allocator &dataBlocksAllocator,
					ACE_Allocator &dataBlocksBufferAllocator) {

			ACE_Message_Block *result
				= static_cast<ACE_Message_Block *>(
					messageBlocksAllocator.malloc(
						sizeof(ACE_Message_Block)));
			assert(result != 0);
			if (result == 0) {
				// using SystemException instead InsufficientMemoryException
				// as using internal memory buffer, not system.
				throw SystemException(
					L"Could not allocate new message block - insufficient internal memory buffer (internal error)");
			}
			
			try {
				new(result)ACE_Message_Block(
					size + sizeof(Timings *),
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
			} catch (...) {
				messageBlocksAllocator.free(result);
				throw;
			}

			assert(result->data_block());
			if (!result->data_block()) {
				messageBlocksAllocator.free(result);
				// using SystemException instead InsufficientMemoryException
				// as using internal memory buffer, not system.
				throw SystemException(
					L"Could not allocate new message block - insufficient internal memory buffer (internal error)");
			}

			return *result;

		}

		static ACE_Message_Block & CreateObject(size_t size) {
			auto result = new ACE_Message_Block(size + sizeof(Timings *));
			assert(result->data_block());
			if (!result->data_block()) {
				delete result;
				throw InsufficientMemoryException(
					L"Could not allocate new buffer for message block - insufficient memory");
			}
			return *result;
		}

		static void Init(bool isTunnelMessage, ACE_Message_Block &object) {

			if (isTunnelMessage) {
				object.set_flags(FLAG_TUNNEL_MESSAGE);
			}

			std::unique_ptr<Timings> timings(new Timings);
			assert(object.size() >= sizeof(Timings *));
			const Timings *const timingsPtr = timings.get();
			assert(object.space() >= sizeof(timingsPtr));
			if (	object.copy(
							reinterpret_cast<const char *>(&timingsPtr),
							sizeof(timingsPtr))
						== -1) {
				throw TunnelEx::InsufficientMemoryException(
					L"Insufficient message block memory");
			}
			object.rd_ptr(sizeof(Timings *));
			object.set_flags(FLAG_HAS_TIMINGS);
			verify(timings->AddRef() == 1);
			timings.release();

		}

	private:

		ACE_Message_Block *m_messageBlock;
		bool m_isAddedToQueue;

	};

	//////////////////////////////////////////////////////////////////////////

}

#endif // #ifndef MESSAGE_BLOCK_ADAPTER_H_INCLUDED
