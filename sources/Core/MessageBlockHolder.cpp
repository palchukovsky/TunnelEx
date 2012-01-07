/**************************************************************************
 *   Created: 2011/12/24/ 11:26
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "MessagesAllocator.hpp"
#include "Log.hpp"
#include "MessageBlockHolder.hpp"

using namespace TunnelEx;
namespace pt = boost::posix_time;

//////////////////////////////////////////////////////////////////////////

namespace {

	enum UniqueMessageBlockHolderFlag {
		UMBHF_TUNNEL_MESSAGE = ACE_Message_Block::USER_FLAGS,
		UMBHF_HAS_SATELLITE = UMBHF_TUNNEL_MESSAGE * 2
	};

	template<typename T>
	class UniqueMallocPtr : boost::noncopyable {

	public:

		typedef T ValueType;

	public:

		explicit UniqueMallocPtr(
					ValueType *ptr,
					ACE_Allocator &allocatorRef) throw()
				: m_isCreated(false),
				m_ptr(ptr),
				m_allocator(allocatorRef) {
			//...//
		}
		~UniqueMallocPtr() throw() {
			Reset();
		}

	public:

		operator bool() const throw() {
			return m_ptr ? true : false;
		}

		ValueType & operator *() throw() {
			assert(m_ptr);
			return *m_ptr;
		}

		ValueType * operator ->() throw() {
			assert(m_ptr);
			return m_ptr;
		}

	public:

		void MarkAsCreated() throw() {
			assert(!m_isCreated);
			m_isCreated = true;
		}

		ValueType * Release() throw() {
			assert(m_ptr);
			auto tmp = m_ptr;
			m_ptr = nullptr;
			return tmp;
		}

		void Reset() throw() {
			if (!m_ptr) {
				return;
			}
			if (m_isCreated) {
				m_ptr->~ValueType();
			}
			m_allocator.free(m_ptr);
			m_ptr = nullptr;
		}

	private:

		bool m_isCreated;
		ValueType *m_ptr;
		ACE_Allocator &m_allocator;

	};

}

//////////////////////////////////////////////////////////////////////////

UniqueMessageBlockHolder::Satellite::Satellite(
			boost::shared_ptr<MessagesAllocator> allocators)
		: m_allocators(allocators),
		m_refsCount(0) {
	TUNNELEX_OBJECTS_DELETION_CHECK_CTOR(m_instancesNumber);
}

UniqueMessageBlockHolder::Satellite::~Satellite() {
	assert(m_refsCount == 0);
	TUNNELEX_OBJECTS_DELETION_CHECK_DTOR(m_instancesNumber);
}

long UniqueMessageBlockHolder::Satellite::AddRef() throw() {
	return Interlocked::Increment(m_refsCount);
}

long UniqueMessageBlockHolder::Satellite::RemoveRef() throw() {
	return Interlocked::Decrement(m_refsCount);
}

const MessagesAllocator &
UniqueMessageBlockHolder::Satellite::GetAllocator()
		const throw() {
	return const_cast<Satellite *>(this)->GetAllocator();
}

MessagesAllocator & UniqueMessageBlockHolder::Satellite::GetAllocator() throw() {
	return *m_allocators;
}

boost::shared_ptr<MessagesAllocator>
UniqueMessageBlockHolder::Satellite::GetAllocatorPtr() {
	return m_allocators;
}

UniqueMessageBlockHolder::Satellite::Timings &
UniqueMessageBlockHolder::Satellite::GetTimings() {
	return m_timings;
}

const UniqueMessageBlockHolder::Satellite::Timings &
UniqueMessageBlockHolder::Satellite::GetTimings() const {
	return const_cast<Satellite *>(this)->GetTimings();
}

UniqueMessageBlockHolder::Satellite::Lock &
UniqueMessageBlockHolder::Satellite::GetLock() {
	return m_lock;
}

const UniqueMessageBlockHolder::Satellite::Lock &
UniqueMessageBlockHolder::Satellite::GetLock() const {
	return const_cast<Satellite *>(this)->GetLock();
}

#ifdef DEV_VER
	long UniqueMessageBlockHolder::Satellite::GetInstancesNumber() {
		return m_instancesNumber;
	}
#endif

TUNNELEX_OBJECTS_DELETION_CHECK_DEFINITION(
	UniqueMessageBlockHolder::Satellite,
	m_instancesNumber);

//////////////////////////////////////////////////////////////////////////

UniqueMessageBlockHolder::Satellite::Timings::LatencyPolicy::StatValueType
UniqueMessageBlockHolder::Satellite::Timings::LatencyPolicy::GetStatValue(
			const boost::posix_time::time_duration &val) {
	assert(!val.is_not_a_date_time());
	return val.total_microseconds();
}

pt::ptime
UniqueMessageBlockHolder::Satellite::Timings::LatencyPolicy::GetCurrentTime() {
	return boost::posix_time::microsec_clock::local_time();
}

void UniqueMessageBlockHolder::Satellite::Timings::SetReceivingStartTimePoint() {
	SetCurrentTimePoint(m_receivingStartTime);
}

void UniqueMessageBlockHolder::Satellite::Timings::SetReceivingTimePoint() {
	SetCurrentTimePoint(m_receivingTime);
}

const pt::ptime &
UniqueMessageBlockHolder::Satellite::Timings::GetReceivingTime() const {
	assert(!m_receivingTime.is_not_a_date_time());
	return m_receivingTime;
}

void UniqueMessageBlockHolder::Satellite::Timings::SetSendingStartTimePoint() {
	SetCurrentTimePoint(m_sendingStartTime);
}

const pt::ptime &
UniqueMessageBlockHolder::Satellite::Timings::GetSendingStartTime() const {
	assert(!m_sendingStartTime.is_not_a_date_time());
	return m_sendingStartTime;
}

void UniqueMessageBlockHolder::Satellite::Timings::SetSendingTimePoint() {
	SetCurrentTimePoint(m_sendingTime);
}

pt::time_duration
UniqueMessageBlockHolder::Satellite::Timings::GetReceivingLatency() const {
	if (m_receivingStartTime.is_not_a_date_time()) {
		return pt::not_a_date_time;
	}
	assert(!m_receivingTime.is_not_a_date_time());
	assert(m_receivingStartTime <= m_receivingTime);
	return m_receivingTime - m_receivingStartTime;
}

pt::time_duration
UniqueMessageBlockHolder::Satellite::Timings::GetSendingLatency() const {
	assert(!m_sendingStartTime.is_not_a_date_time());
	assert(!m_sendingTime.is_not_a_date_time());
	assert(m_sendingStartTime <= m_sendingTime);
	return m_sendingTime - m_sendingStartTime;
}

pt::time_duration
UniqueMessageBlockHolder::Satellite::Timings::GetProcessingLatency() const {
	assert(!m_receivingTime.is_not_a_date_time());
	assert(!m_sendingStartTime.is_not_a_date_time());
	assert(m_receivingTime <= m_sendingStartTime);
	return m_sendingStartTime - m_receivingTime;
}

pt::time_duration
UniqueMessageBlockHolder::Satellite::Timings::GetFullLatency() const {
	assert(!m_receivingTime.is_not_a_date_time());
	assert(!m_sendingTime.is_not_a_date_time());
	assert(m_receivingTime <= m_sendingTime);
	return m_sendingTime - m_receivingTime;
}

void UniqueMessageBlockHolder::Satellite::Timings::SetCurrentTimePoint(
			pt::ptime &var) {
	assert(var.is_not_a_date_time());
	var = LatencyPolicy::GetCurrentTime();
}

//////////////////////////////////////////////////////////////////////////

UniqueMessageBlockHolder::Satellite::Lock::~Lock() {
	//...//
}

int UniqueMessageBlockHolder::Satellite::Lock::acquire() {
	m_mutex.Acquire();
	return 0;
}

int UniqueMessageBlockHolder::Satellite::Lock::remove(void) {
	assert(false);
	return -1;
}

int UniqueMessageBlockHolder::Satellite::Lock::tryacquire() {
	assert(false);
	return -1;
}

int UniqueMessageBlockHolder::Satellite::Lock::release() {
	m_mutex.Release();
	return 0;
}

int UniqueMessageBlockHolder::Satellite::Lock::acquire_read() {
	assert(false);
	return -1;
}

int UniqueMessageBlockHolder::Satellite::Lock::acquire_write() {
	assert(false);
	return -1;
}

int UniqueMessageBlockHolder::Satellite::Lock::tryacquire_read() {
	assert(false);
	return -1;
}

int UniqueMessageBlockHolder::Satellite::Lock::tryacquire_write() {
	assert(false);
	return -1;
}

int UniqueMessageBlockHolder::Satellite::Lock::tryacquire_write_upgrade() {
	assert(false);
	return -1;
}

//////////////////////////////////////////////////////////////////////////

UniqueMessageBlockHolder::UniqueMessageBlockHolder() throw() 
		: m_messageBlock(nullptr),
		m_isAddedToQueue(false) {
	//...//
}

UniqueMessageBlockHolder::UniqueMessageBlockHolder(ACE_Message_Block *messageBlock) throw() 
		: m_messageBlock(messageBlock),
		m_isAddedToQueue(false) {
	//...//
}

UniqueMessageBlockHolder::UniqueMessageBlockHolder(ACE_Message_Block &messageBlock) throw() 
		: m_messageBlock(&messageBlock),
		m_isAddedToQueue(false) {
	//...//
}

UniqueMessageBlockHolder::~UniqueMessageBlockHolder() throw() {
	Reset();
}

UniqueMessageBlockHolder::Satellite & UniqueMessageBlockHolder::GetSatellite(
			ACE_Message_Block &messageBlock) {
	assert(messageBlock.flags() & UMBHF_HAS_SATELLITE);
	Satellite *result;
	memcpy(&result, messageBlock.base(), sizeof(result));
	return *result;
}

UniqueMessageBlockHolder::Satellite & UniqueMessageBlockHolder::GetSatellite() {
	assert(IsSet());
	return GetSatellite(*m_messageBlock);
}

const UniqueMessageBlockHolder::Satellite &
UniqueMessageBlockHolder::GetSatellite() const {
	return const_cast<UniqueMessageBlockHolder *>(this)->GetSatellite();
}

UniqueMessageBlockHolder::Timings & UniqueMessageBlockHolder::GetTimings(
			ACE_Message_Block &messageBlock) {
	return GetSatellite(messageBlock).GetTimings();
}

UniqueMessageBlockHolder::Timings & UniqueMessageBlockHolder::GetTimings() {
	assert(IsSet());
	return GetTimings(*m_messageBlock);
}

const UniqueMessageBlockHolder::Timings & UniqueMessageBlockHolder::GetTimings()
		const {
	return const_cast<UniqueMessageBlockHolder *>(this)->GetTimings();
}

size_t UniqueMessageBlockHolder::GetMessageMemorySize(size_t clientSize) {
	return clientSize + sizeof(Satellite *);
}

double UniqueMessageBlockHolder::GetUsage() const {
	assert(IsSet());
	assert(m_messageBlock->rd_ptr() - m_messageBlock->base() >= 0);
	size_t result = m_messageBlock->rd_ptr() - m_messageBlock->base();
	assert(result >= 0);
	if (m_messageBlock->flags() & UMBHF_HAS_SATELLITE) {
		assert(GetMessageMemorySize(0) <= result);
		result -= GetMessageMemorySize(0);
	}
	return (result * 100) / GetSatellite().GetAllocator().GetDataBlockSize();
}

ACE_Message_Block & UniqueMessageBlockHolder::Duplicate() {
	assert(IsSet());
	auto result = m_messageBlock->duplicate();
	if (!result) {
		throw InsufficientMemoryException(
			L"Failed to allocate memory for message block duplicate");
	}
	verify(GetSatellite().AddRef() >= 2);
	return *result;
}

namespace {

	ACE_Message_Block & CreateMessageBlock(
				UniqueMallocPtr<UniqueMessageBlockHolder::Satellite> &satellite,
				size_t size,
				bool isTunnelMessage) {

		typedef UniqueMessageBlockHolder::Satellite Satellite;
		auto &allocators = satellite->GetAllocator();

		UniqueMallocPtr<ACE_Message_Block> result(
			static_cast<ACE_Message_Block *>(
				allocators.GetMessageBlocksAllocator().malloc(sizeof(ACE_Message_Block))),
			allocators.GetMessageBlocksAllocator());
		if (!result) {
			throw InsufficientMemoryException(
				L"Failed to allocate memory for new message block");
		}
		new(&*result)ACE_Message_Block(
			UniqueMessageBlockHolder::GetMessageMemorySize(size),
			ACE_Message_Block::MB_DATA,
			0,
			0,
			&allocators.GetDataBlocksBufferAllocator(),
			&satellite->GetLock(),
			ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
			ACE_Time_Value::zero,
			ACE_Time_Value::max_time,
			&allocators.GetDataBlocksAllocator(),
			&allocators.GetMessageBlocksAllocator());
		result.MarkAsCreated();
		if (!result->data_block()) {
			throw InsufficientMemoryException(
				L"Failed to allocate memory for new data block");
		}

		{
			Satellite *const satellitePtr = &*satellite;
			assert(result->size() >= sizeof(satellitePtr));
			assert(result->space() >= sizeof(satellitePtr));
			if (	result->copy(
						reinterpret_cast<const char *>(&satellitePtr),
						sizeof(satellitePtr))
					== -1) {
				throw InsufficientMemoryException(
					L"Failed to allocate memory for message block satellite pointer");
			}
			result->rd_ptr(sizeof(satellitePtr));
			result->set_flags(UMBHF_HAS_SATELLITE);
			verify(satellitePtr->AddRef() == 1);
			satellite.Release();
		}

		if (isTunnelMessage) {
			result->set_flags(UMBHF_TUNNEL_MESSAGE);
		}
	
		return *result.Release();

	}

}

ACE_Message_Block & UniqueMessageBlockHolder::Create(
			size_t size,
			boost::shared_ptr<MessagesAllocator> allocators,
			bool isTunnelMessage) {

	UniqueMallocPtr<Satellite> satellite(
	static_cast<Satellite *>(
			allocators->GetMessageBlockSatellitesAllocator().malloc(sizeof(Satellite))),
		allocators->GetMessageBlockSatellitesAllocator());
	if (!satellite) {
		throw InsufficientMemoryException(
			L"Failed to allocate memory for message block satellite");
	}
	new(&*satellite)Satellite(allocators);
	satellite.MarkAsCreated();

	return CreateMessageBlock(satellite, size, isTunnelMessage);

}

void UniqueMessageBlockHolder::Delete(ACE_Message_Block &messageBlock) throw() {
			
	ACE_Allocator *messageBlocksAllocator = nullptr;
	ACE_Allocator *dataBlocksAllocator = nullptr;
	ACE_Allocator *dataBlocksBufferAllocator = nullptr;
	messageBlock.access_allocators(
		dataBlocksBufferAllocator,
		dataBlocksAllocator,
		messageBlocksAllocator);
	assert(
		messageBlocksAllocator
		&& dataBlocksAllocator
		&& dataBlocksBufferAllocator);

	Satellite *satellite = nullptr;
	if (messageBlock.flags() & UMBHF_HAS_SATELLITE) {
		satellite = &GetSatellite(messageBlock);
		assert(
			messageBlocksAllocator == &satellite->GetAllocator().GetMessageBlocksAllocator()
			&& dataBlocksAllocator == &satellite->GetAllocator().GetDataBlocksAllocator()
			&& dataBlocksBufferAllocator == &satellite->GetAllocator().GetDataBlocksBufferAllocator());
	}

	UniqueMallocPtr<ACE_Message_Block>(&messageBlock, *messageBlocksAllocator)
		.MarkAsCreated();

	if (satellite && satellite->RemoveRef() == 0) {
		const boost::shared_ptr<const MessagesAllocator> allocator(
			satellite->GetAllocatorPtr());
		UniqueMallocPtr<Satellite>(
					satellite,
					satellite->GetAllocator().GetMessageBlockSatellitesAllocator())
			.MarkAsCreated();
	}

}

void UniqueMessageBlockHolder::Reset(
			ACE_Message_Block *newMessageBlock /*= nullptr*/)
		throw() {
	if (IsSet()) {
		Delete(*m_messageBlock);
	}
	m_messageBlock = newMessageBlock;
}

void UniqueMessageBlockHolder::Release() throw() {
	assert(IsSet());
	m_messageBlock = nullptr;
}

bool UniqueMessageBlockHolder::IsSet() const throw() {
	return m_messageBlock ? true : false;
}

ACE_Message_Block & UniqueMessageBlockHolder::Get() throw() {
	assert(IsSet());
	return *m_messageBlock;
}

const ACE_Message_Block & UniqueMessageBlockHolder::Get() const throw() {
	return const_cast<UniqueMessageBlockHolder *>(this)->Get();
}

const char * UniqueMessageBlockHolder::GetData() const throw() {
	assert(IsSet());
	return m_messageBlock->rd_ptr();
}

char * UniqueMessageBlockHolder::GetWritableSpace(size_t size) {
	assert(IsSet());
	assert(m_messageBlock->space() >= size);
	if (m_messageBlock->space() < size) {
		throw InsufficientMemoryException(
			L"Failed to allocate memory for existing message block (logic error)");
	}
	return m_messageBlock->wr_ptr();
}

void UniqueMessageBlockHolder::TakeWritableSpace(size_t size) {
	assert(IsSet());
	assert(m_messageBlock->space() >= size);
	if (m_messageBlock->space() < size) {
		throw InsufficientMemoryException(
			L"Failed to allocate memory for existing message block (logic error)");
	}
	m_messageBlock->wr_ptr(size);
}

size_t UniqueMessageBlockHolder::GetUnreadedDataSize() const throw() {
	assert(IsSet());
	return m_messageBlock->length();
}

void UniqueMessageBlockHolder::SetData(const char *data, size_t length) {
			
	assert(IsSet());
			
	ACE_Allocator *messageBlocksAllocator = nullptr;
	ACE_Allocator *dataBlocksAllocator = nullptr;
	ACE_Allocator *dataBlocksBufferAllocator = nullptr;
	m_messageBlock->access_allocators(
		dataBlocksBufferAllocator,
		dataBlocksAllocator,
		messageBlocksAllocator);
	assert(dataBlocksBufferAllocator && dataBlocksAllocator && messageBlocksAllocator);

	assert(m_messageBlock->flags() & UMBHF_HAS_SATELLITE);

	UniqueMallocPtr<ACE_Message_Block> newBlock(
		static_cast<ACE_Message_Block *>(
			messageBlocksAllocator->malloc(sizeof(ACE_Message_Block))),
			*messageBlocksAllocator);
	if (!newBlock) {
		throw InsufficientMemoryException(
			L"Failed to allocate memory for existing message block");
	}
	new(&*newBlock)ACE_Message_Block(
		GetMessageMemorySize(length),
		ACE_Message_Block::MB_DATA,
		0,
		0,
		dataBlocksBufferAllocator,
		&GetSatellite(*m_messageBlock).GetLock(),
		ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
		ACE_Time_Value::zero,
		ACE_Time_Value::max_time,
		dataBlocksAllocator,
		messageBlocksAllocator);
	newBlock.MarkAsCreated();
	if (!newBlock->data_block()) {
		throw InsufficientMemoryException(
			L"Failed to allocate memory for new data block (existing message block)");
	}
	assert(newBlock->data_block()->size() >= GetMessageMemorySize(length));
	assert(m_messageBlock->space() >= GetMessageMemorySize(length));

	{
		assert(m_messageBlock->flags() & UMBHF_HAS_SATELLITE);
		assert(m_messageBlock->space() >= sizeof(Satellite *));
		if (newBlock->copy(m_messageBlock->base(), sizeof(Satellite *)) == -1) {
			throw InsufficientMemoryException(
				L"Failed to allocate memory for new data block (existing message block)");
		}
		newBlock->rd_ptr(sizeof(Satellite *));
	}

	if (newBlock->copy(data, length) == -1) {
		throw InsufficientMemoryException(
			L"Failed to allocate memory for new data block (existing message block)");
	}
	
	if (m_messageBlock->flags() & UMBHF_TUNNEL_MESSAGE) { 
		newBlock->set_flags(UMBHF_TUNNEL_MESSAGE);
	}
	m_messageBlock->clr_flags(UMBHF_HAS_SATELLITE);
	newBlock->set_flags(UMBHF_HAS_SATELLITE);

	Delete(*m_messageBlock);
	m_messageBlock = newBlock.Release();			
		
}

void UniqueMessageBlockHolder::MarkAsAddedToQueue() throw() {
	m_isAddedToQueue = true;
}

bool UniqueMessageBlockHolder::IsAddedToQueue() const throw() {
	return m_isAddedToQueue;
}

bool UniqueMessageBlockHolder::IsTunnelMessage() const throw() {
	assert(IsSet());
	return m_messageBlock->flags() & UMBHF_TUNNEL_MESSAGE ? true : false;
}

#ifdef DEV_VER
	long UniqueMessageBlockHolder::GetSatellitesInstancesNumber() {
		return Satellite::GetInstancesNumber();
	}
#endif

void UniqueMessageBlockHolder::SetReceivingStartTimePoint() {
	GetTimings().SetReceivingStartTimePoint();
}

void UniqueMessageBlockHolder::SetReceivingTimePoint() {
	GetTimings().SetReceivingTimePoint();
}
const pt::ptime & UniqueMessageBlockHolder::GetReceivingTime() const {
	return GetTimings().GetReceivingTime();
}

void UniqueMessageBlockHolder::SetSendingStartTimePoint() {
	GetTimings().SetSendingStartTimePoint();
}
const pt::ptime & UniqueMessageBlockHolder::GetSendingStartTime() const {
	return GetTimings().GetSendingStartTime();
}

void UniqueMessageBlockHolder::SetSendingTimePoint() {
	GetTimings().SetSendingTimePoint();
}

pt::time_duration UniqueMessageBlockHolder::GetReceivingLatency() const {
	return GetTimings().GetReceivingLatency();
}

pt::time_duration UniqueMessageBlockHolder::GetSendingLatency() const {
	return GetTimings().GetSendingLatency();
}

pt::time_duration UniqueMessageBlockHolder::GetProcessingLatency() const {
	return GetTimings().GetProcessingLatency();
}

pt::time_duration UniqueMessageBlockHolder::GetFullLatency() const {
	return GetTimings().GetFullLatency();
}
