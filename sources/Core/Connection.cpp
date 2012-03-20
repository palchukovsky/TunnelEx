/**************************************************************************
 *                                                                        *
 *   Created: 2007/06/12 20:34                                            *
 *     Author: Eugene V. Palchukovsky                                     *
 *     E-mail: eugene@palchukovsky.com                                    *
 * -------------------------------------------------------------------    *
 *    Project: TunnelEx                                                   *
 *                                                                        *
 **************************************************************************/

#include "Prec.h"
#include "Connection.hpp"
#include "EndpointAddress.hpp"
#include "ConnectionSignal.hpp"
#include "Exceptions.hpp"
#include "Log.hpp"
#include "MessageBlocksLatencyStat.hpp"
#include "MessageBlockHolder.hpp"
#include "Tunnel.hpp"
#include "MessagesAllocator.hpp"
#include "Error.hpp"
#include "Locking.hpp"

using namespace TunnelEx;
using namespace TunnelEx::Helpers::Asserts;
namespace pt = boost::posix_time;

//////////////////////////////////////////////////////////////////////////

namespace {

	template<int TypeVal>
	struct MutexTypeToType {
		typedef MutexTypeToType<TypeVal> Type;
	};

	template<typename Mutex, typename TypeT>
	class TypedMutex : public Mutex {
		typedef typename TypeT::Type Type;
	};

}

#if defined(DEV_VER) && defined(NOT_DEFINED)
namespace {

	////////////////////////////////////////////////////////////////////////////////

	struct DebugLockStat {

		static volatile long locks;
		static volatile long fails;
		static volatile long proactor;

		static volatile long lastReportTime;
		static const long reportPeriod;

		static const double errorLevel;
		static const double warnLevel;
		static const double infoLevel;

		static void Report() {
			const auto failesLevel
				= (double(DebugLockStat::fails) / double(DebugLockStat::locks)) * 100;
			Format stat(
				"Connection locks/waits/proactor statistic: %1%/%2%/%3% (%4%%% waits).");
			stat
				% DebugLockStat::locks
				% DebugLockStat::fails
				% DebugLockStat::proactor
				% failesLevel;
			assert(errorLevel > warnLevel && warnLevel > infoLevel);
			if (failesLevel >= errorLevel) {
				Log::GetInstance().AppendError(stat.str());
			} else if (failesLevel >= warnLevel) {
				Log::GetInstance().AppendWarn(stat.str());
			} else if (failesLevel >= infoLevel) {
				Log::GetInstance().AppendInfo(stat.str());
			} else {
				Log::GetInstance().AppendDebug(stat.str());
			}
		}

	};

	volatile long DebugLockStat::locks = 0;
	volatile long DebugLockStat::fails = 0;
	volatile long DebugLockStat::proactor = 0;
	
	volatile long DebugLockStat::lastReportTime = 0;
	const long DebugLockStat::reportPeriod = 60;
	
	const double DebugLockStat::errorLevel = 15;
	const double DebugLockStat::warnLevel = 12.5;
	const double DebugLockStat::infoLevel = 10;

	////////////////////////////////////////////////////////////////////////////////

	template<typename MutexT>
	class LockWithDebugReports : public ACE_Guard<MutexT> {

	public:

		typedef MutexT Mutex;
		typedef ACE_Guard<Mutex> Base;

	public:

		explicit LockWithDebugReports(Mutex &mutex, const bool isProactorCall)
				: Base(mutex, 0) {

			if (!locked()) {
				verify(acquire() != -1);
				Interlocked::Increment(DebugLockStat::fails);
			}

			if (isProactorCall) {
				Interlocked::Increment(DebugLockStat::proactor);
			}

			if (!(Interlocked::Increment(DebugLockStat::locks) % 20000)) {
				const long now = long(time(nullptr));
				if (now - DebugLockStat::lastReportTime >= DebugLockStat::reportPeriod) {
					DebugLockStat::Report();
					Interlocked::Exchange(DebugLockStat::lastReportTime, now);
				}
			}

		}

	};

	////////////////////////////////////////////////////////////////////////////////

}
#else
namespace {

	template<typename MutexT>
	class LockWithDebugReports : public ACE_Guard<MutexT> {
	public:
		typedef MutexT Mutex;
		typedef ACE_Guard<Mutex> Base;
	public:
		explicit LockWithDebugReports(Mutex &mutex, const bool)
				: Base(mutex) {
			//...//
		}
	};

}
#endif

//////////////////////////////////////////////////////////////////////////

class Connection::Implementation : public ACE_Handler {

private:

	enum MutexType {
		MT_DEFAULT
	};

	enum ReadingState {
		RS_NOT_ALLOWED,
		RS_NOT_STARTED,
		RS_STOPPING_READING,
		RS_READING,
		RS_NOT_READING
	};

	enum SetupState {
		SETUP_STATE_NOT_COMPLETED,
		SETUP_STATE_FAILED,
		SETUP_STATE_COMPLETED_PENDING,
		SETUP_STATE_COMPLETED,
	};

	typedef TypedMutex<ACE_Recursive_Thread_Mutex, MutexTypeToType<MT_DEFAULT>>
		Mutex;
	typedef LockWithDebugReports<Mutex> Lock;

	typedef ReadWriteSpinMutex ExternalMessagesAllocatorMutex;
	typedef ReadLock<ExternalMessagesAllocatorMutex> ExternalMessagesAllocatorReadLock;
	typedef WriteLock<ExternalMessagesAllocatorMutex> ExternalMessagesAllocatorWriteLock;

	typedef MessageBlocksLatencyStat LatencyStat;

	struct IdleTimeoutPolicy {
		static pt::ptime GetCurrentTime() {
			return pt::second_clock::local_time();
		}
		static long GetEventOffset(
					const pt::ptime &startTime,
					const pt::ptime &eventTime) {
			assert(startTime <= eventTime);
			return (eventTime - startTime).total_seconds();
		}
		static long GetIdleTime(const pt::ptime &startTime, long eventTime) {
			const auto now = GetEventOffset(startTime, GetCurrentTime());
			assert(eventTime <= now);
			const auto idle = now - eventTime;
			return idle;
		}
		static long GetInterval(long /*hours*/, long /*mins*/, long secs) {
			return secs;
		}
		static long CalcSleepTime(long idleTimeoutInterval, long idleTime) {
			return idleTimeoutInterval - idleTime;
		}
		static ACE_Time_Value IntervalToAceTime(long val) {
			return ACE_Time_Value(val);
		}
	};

public:

	explicit Implementation(
				Connection &myInteface,
				const RuleEndpoint &ruleEndpoint,
				SharedPtr<const EndpointAddress> &ruleEndpointAddress,
				TimeSeconds idleTimeoutSeconds = 0)
			: m_myInterface(myInteface),
			m_instanceId(m_myInterface.GetInstanceId()),
			m_refsCount(0),
			m_ruleEndpoint(ruleEndpoint),
			m_ruleEndpointAddress(ruleEndpointAddress),
			m_readingState(RS_NOT_ALLOWED),
			m_setupState(SETUP_STATE_NOT_COMPLETED),
			m_proactor(nullptr),
			m_isClosed(false),
			m_startTime(IdleTimeoutPolicy::GetCurrentTime()),
			m_idleTimeoutInterval(
				IdleTimeoutPolicy::GetInterval(0, 0, idleTimeoutSeconds)),
			m_idleTimeoutTimer(-1),
			m_idleTimeoutUpdateTime(-1),
			//! @todo: hardcoded - latency stat period (secs)
			m_latencyStat(m_instanceId, 60),
			m_readStartAttemptsCount(0),
			m_readsMallocFailsCount(0),
			m_closeCode(m_closeCodeNotSetValue) {
		assert(m_closeCode == std::numeric_limits<long>::min());
	}
	
	virtual ~Implementation() throw() {
		assert(m_setupState != SETUP_STATE_COMPLETED_PENDING);
		assert(m_refsCount == 0);
		if (!IsSetupCompleted()) {
			m_ruleEndpointAddress->StatConnectionSetupCanceling();
		}
		m_latencyStat.Dump();
	}

private:

	void CheckedDelete(Lock &lock, bool removeRef = true) throw() {

		assert(m_refsCount > 0 || !removeRef);

		Interlocked::Exchange(m_isClosed, true);
		if (m_idleTimeoutTimer >= 0) {
			const auto timerId = Interlocked::Exchange(m_idleTimeoutTimer, -2);
			assert(timerId >= 0);
			if (m_proactor->cancel_timer(timerId, nullptr, false) != 1) {
				return;
			}
		}

		if (removeRef) {
			verify(Interlocked::Decrement(m_refsCount) >= 0);
		}
		if (m_refsCount > 0) {
			return;
		}

		assert(m_refsCount == 0);
		assert(m_readingState != RS_STOPPING_READING);
		assert(m_readingState != RS_READING);

		lock.release();

		const auto instanceId = m_instanceId;
		try {
			SharedPtr<ConnectionSignal> signal = m_signal;
			signal->OnConnectionClosed(m_instanceId);
		} catch (...) {
			Format message(
				"Unknown system error occurred for connection %5%: %1%:%2%."
				" Please restart the service"
				" and contact product support to resolve this issue."
				" %3% %4%");
			message
				% __FILE__ % __LINE__
				% TUNNELEX_NAME % TUNNELEX_BUILD_IDENTITY
				% instanceId;
			Log::GetInstance().AppendFatalError(message.str());
			assert(false);
		}

	}

	//! Object can be deleted after calling.
	void RemoveRef(const bool isProactorCall) throw() {
		assert(isProactorCall);
		if (Interlocked::Decrement(m_refsCount) > 0) {
			return;
		}
		Lock lock(m_mutex, isProactorCall);
		assert(m_refsCount >= 0);
		if (m_refsCount > 0) {
			return;
		}
		CheckedDelete(lock, false);
	}

public:

	void Open(SharedPtr<ConnectionSignal> &signal, Connection::Mode mode) {

		const bool isReadingAllowed = mode != Connection::MODE_WRITE;

		Lock lock(m_mutex, false);

		assert(!m_proactor || m_proactor == &signal->GetTunnel().GetProactor());
		assert(!m_tunnelMessagesAllocator);

		if (m_proactor) {
			if (	m_proactor != &signal->GetTunnel().GetProactor()
					|| m_readingState == RS_NOT_ALLOWED || isReadingAllowed) {
				throw TunnelEx::LogicalException(L"Could not open data transfer twice");
			}
			m_signal = signal;
			Log::GetInstance().AppendDebug(
				"Connection %1% already opened.",
				m_instanceId);
			return;
		}

		const auto dataBlockSize = MessagesAllocator::DefautDataBlockSize;
		const auto messageBlockQueueBufferSize
			= MessagesAllocator::DefautConnectionBufferSize
				/ UniqueMessageBlockHolder::GetMessageMemorySize(dataBlockSize);

		const IoHandleInfo ioHandleInfo = m_myInterface.GetIoHandle();

		boost::shared_ptr<MessagesAllocator> allocator;
		if (isReadingAllowed && ioHandleInfo.handle != INVALID_HANDLE_VALUE) {
			allocator.reset(
				new MessagesAllocator(
					messageBlockQueueBufferSize + 1, // plus 1 for message, that duplicated for proactor
					messageBlockQueueBufferSize,
					UniqueMessageBlockHolder::GetMessageMemorySize(dataBlockSize)));
		}

		ACE_Proactor &proactor = signal->GetTunnel().GetProactor();
			
		std::unique_ptr<ACE_Asynch_Operation> readStream;
		std::unique_ptr<ACE_Asynch_Operation> writeStream;

		boost::function<int(ACE_Handler &, ACE_HANDLE, const void *, ACE_Proactor *)>
			openReadStreamFunc;
		boost::function<int(ACE_Handler &, ACE_HANDLE, const void *, ACE_Proactor *)>
			openWriteStreamFunc;

		boost::function<int(ACE_Message_Block &, size_t)> readStreamFunc;
		boost::function<int(ACE_Message_Block &, size_t)> writeStreamFunc;
			
		if (ioHandleInfo.handle != INVALID_HANDLE_VALUE) {
			switch (ioHandleInfo.type) {
				default:
					assert(false);
				case IoHandleInfo::TYPE_OTHER:
					readStream.reset(new ACE_Asynch_Read_File);
					openReadStreamFunc = boost::bind(
						&ACE_Asynch_Read_File::open,
						boost::polymorphic_downcast<ACE_Asynch_Read_File *>(readStream.get()),
						_1,
						_2,
						_3,
						_4);
					readStreamFunc = boost::bind(
						&ACE_Asynch_Read_File::read,
						boost::polymorphic_downcast<ACE_Asynch_Read_File *>(readStream.get()),
						_1,
						_2,
						0,
						0,
						static_cast<void *>(0),
						0,
						ACE_SIGRTMIN);
					if (isReadingAllowed) {
						writeStream.reset(new ACE_Asynch_Write_File);
						openWriteStreamFunc = boost::bind(
							&ACE_Asynch_Write_File::open,
							boost::polymorphic_downcast<ACE_Asynch_Write_File *>(writeStream.get()),
							_1,
							_2,
							_3,
							_4);
						writeStreamFunc = boost::bind(
							&ACE_Asynch_Write_File::write,
							boost::polymorphic_downcast<ACE_Asynch_Write_File *>(writeStream.get()),
							_1,
							_2,
							0,
							0,
							static_cast<void *>(0),
							0,
							ACE_SIGRTMIN);
					}
					break;
				case IoHandleInfo::TYPE_SOCKET:
					readStream.reset(new ACE_Asynch_Read_Stream);
					openReadStreamFunc = boost::bind(
						&ACE_Asynch_Read_Stream::open,
						boost::polymorphic_downcast<ACE_Asynch_Read_Stream *>(readStream.get()),
						_1,
						_2,
						_3,
						_4);
					readStreamFunc = boost::bind(
						&ACE_Asynch_Read_Stream::read,
						boost::polymorphic_downcast<ACE_Asynch_Read_Stream *>(readStream.get()),
						_1,
						_2,
						static_cast<void *>(0),
						0,
						ACE_SIGRTMIN);
					if (isReadingAllowed) {
						writeStream.reset(new ACE_Asynch_Write_Stream);
						openWriteStreamFunc = boost::bind(
							&ACE_Asynch_Write_Stream::open,
							boost::polymorphic_downcast<ACE_Asynch_Write_Stream *>(writeStream.get()),
							_1,
							_2,
							_3,
							_4);
						writeStreamFunc = boost::bind(
							&ACE_Asynch_Write_Stream::write,
							boost::polymorphic_downcast<ACE_Asynch_Write_Stream *>(writeStream.get()),
							_1,
							_2,
							static_cast<void *>(0),
							0,
							ACE_SIGRTMIN);
					}
					break;
			}
			if (openReadStreamFunc(*this, ioHandleInfo.handle, 0, &proactor) == -1) {
				const Error error(errno);
				WFormat message(
					L"Could not open connection read stream: %1% (%2%)");
				message % error.GetStringW() % error.GetErrorNo();
				throw ConnectionOpeningException(message.str().c_str());
			}
			if (	openWriteStreamFunc
					&& openWriteStreamFunc(*this, ioHandleInfo.handle, 0, &proactor) == -1) {
				const Error error(errno);
				WFormat message(
					L"Could not open connection write stream: %1% (%2%)");
				message % error.GetStringW() % error.GetErrorNo();	
				throw ConnectionOpeningException(message.str().c_str());
			}
		}

		allocator.swap(m_tunnelMessagesAllocator);
		m_writeStream.reset(writeStream.release());
		m_readStream.reset(readStream.release());
		readStreamFunc.swap(m_readStreamFunc);
		writeStreamFunc.swap(m_writeStreamFunc);
		m_readingState
			= !isReadingAllowed || ioHandleInfo.handle == INVALID_HANDLE_VALUE
				?	RS_NOT_ALLOWED
				:	RS_NOT_STARTED;
		signal.Swap(m_signal);
		m_proactor = &proactor;
		verify(Interlocked::Increment(m_refsCount) == 1);

	}

	void Close() throw() {
		assert(IsNotLockedByMyThread(m_mutex));
		Lock lock(m_mutex, false);
		assert(m_refsCount > 0);
		assert(
			m_refsCount > 1
			|| (m_readingState != RS_STOPPING_READING
				&& m_readingState != RS_READING));
		m_myInterface.CloseIoHandle();
		CheckedDelete(lock);
	}

public:

	DataTransferCommand SendToRemote(MessageBlock &messageBlock) {

		if (m_isClosed) {
			return DATA_TRANSFER_CMD_SEND_PACKET;
		}

		UniqueMessageBlockHolder &messageBlockHolder
			= *boost::polymorphic_downcast<UniqueMessageBlockHolder *>(&messageBlock);
		assert(messageBlock.GetUnreadedDataSize() > 0);
		UniqueMessageBlockHolder messageBlockDuplicate(messageBlockHolder.Duplicate());

		Lock lock(m_mutex, false);
		assert(m_writeStream.get());

		if (!m_writeStream.get()) {
			throw LogicalException(
				L"Could not send data in connection, which does not opened");
		}

		if (m_isClosed) {
			return DATA_TRANSFER_CMD_SEND_PACKET;
		}

		messageBlockHolder.SetSendingStartTimePoint();
		const auto writeResult = m_writeStreamFunc(
			messageBlockDuplicate.Get(),
			messageBlockDuplicate.GetUnreadedDataSize());
		if (writeResult == -1) {
			const auto errNo = errno;
			lock.release();
			ReportSendError(errNo); // not only log, can throws
		} else {
			// incrementing only here as "isClosed + locking" guaranties that 
			// the m_refsCount is not zero and object will not destroyed from
			// another thread (also see read-init incrimination)
			Interlocked::Increment(m_refsCount);
			lock.release();
			UpdateIdleTimer(messageBlockHolder.GetSendingStartTime());
			messageBlockDuplicate.Release();
			messageBlockHolder.MarkAsAddedToQueue();
		}

		return DATA_TRANSFER_CMD_SEND_PACKET;
	
	}

public:

	void OnMessageBlockSent(MessageBlock &messageBlock) {
		assert(IsNotLockedByMyThread(m_mutex));
		UniqueMessageBlockHolder &messageHolder
			= *boost::polymorphic_downcast<UniqueMessageBlockHolder *>(&messageBlock);
		if (!messageHolder.IsTunnelMessage()) {
			CollectLatencyStat(messageHolder);
			messageHolder.Reset();
			return;
		}
		CollectLatencyStat(messageHolder);
		messageHolder.Reset();
		Lock lock(m_mutex, false);
		if (!m_isClosed && InitMessageReading(false)) {
			return;
		}
		m_signal->OnConnectionClose(m_instanceId);
	}

	void OnSetupSuccess(bool isDefaultSetupImplementation = false) {
		Lock lock(m_mutex, false);
		assert(m_setupState == SETUP_STATE_NOT_COMPLETED);
		if (!isDefaultSetupImplementation) {
			m_setupState = SETUP_STATE_COMPLETED_PENDING;
		} else {
			m_setupState = SETUP_STATE_COMPLETED;
			m_signal->OnConnectionSetupCompleted(m_instanceId);
		}
		m_ruleEndpointAddress->StatConnectionSetupCompleting();
		StartIdleTimer();
	}

	void OnSetupFail(const WString &failReason) {
		Lock lock(m_mutex, false);
		assert(m_setupState == SETUP_STATE_NOT_COMPLETED);
		m_setupState = SETUP_STATE_FAILED;
		m_ruleEndpointAddress->StatConnectionSetupCanceling(failReason);
		Log::GetInstance().AppendDebug(
			"Connection %1% setup has been canceled.",
			m_instanceId);
	}

public:

	void StartReading() {
		{
			Lock lock(m_mutex, false);
			assert(m_readingState <= RS_STOPPING_READING);
			assert(!m_isClosed);
			if (m_readingState == RS_NOT_ALLOWED) {
				return;
			}
			m_readingState = RS_NOT_READING;
			if (InitMessageReading(true)) {
				return;
			}
		}
		Log::GetInstance().AppendDebug(
			"Failed to start reading for connection %1%.",
			m_instanceId);
		m_signal->OnConnectionClose(m_instanceId);
	}

	void StopReading() {
		assert(IsLockedByMyThread(m_mutex));
		if (m_readingState > RS_NOT_ALLOWED) {
			m_readingState = m_readingState == RS_READING
				?	RS_STOPPING_READING
				:	RS_NOT_STARTED;
		}
	}

	const RuleEndpoint & GetRuleEndpoint() const {
		return m_ruleEndpoint;
	}

	SharedPtr<const EndpointAddress> GetRuleEndpointAddress() const {
		return m_ruleEndpointAddress;
	}

	void ReadRemote(MessageBlock &messageBlock) {
		assert(IsLockedByMyThread(m_mutex));
		if (m_setupState == SETUP_STATE_FAILED) {
			return;
		}
		if (m_setupState == SETUP_STATE_COMPLETED_PENDING) {
			m_setupState = SETUP_STATE_COMPLETED;
			m_signal->OnConnectionSetupCompleted(m_instanceId);
		}
		SendToTunnel(messageBlock);
	}

	void SendToTunnel(MessageBlock &messageBlock) {
		assert(
			!boost::polymorphic_downcast<UniqueMessageBlockHolder *>(&messageBlock)
					->GetReceivingTime()
					.is_not_a_date_time());
		UpdateIdleTimer(
			boost::polymorphic_downcast<UniqueMessageBlockHolder *>(&messageBlock)
				->GetReceivingTime());
		if (messageBlock.GetUnreadedDataSize() == 0) {
			return;
		}
		m_signal->OnNewMessageBlock(messageBlock);
	}

	bool IsSetupCompleted() const {
		Lock lock(m_mutex, false);
		assert(m_setupState != SETUP_STATE_COMPLETED_PENDING);
		return m_setupState >= SETUP_STATE_COMPLETED_PENDING;
	}

	bool IsSetupFailed() const {
		Lock lock(m_mutex, false);
		assert(m_setupState != SETUP_STATE_COMPLETED_PENDING);
		return m_setupState == SETUP_STATE_FAILED;
	}

	long GetCloseCode() const throw() {
		return m_closeCode;
	}

public:

	virtual void handle_read_file(const ACE_Asynch_Read_File::Result &result) {
		HandleReadStream(result);
	}

	virtual void handle_write_file(const ACE_Asynch_Write_File::Result &result) {
		HandleWriteStream(result);
	}

	virtual void handle_read_stream(const ACE_Asynch_Read_Stream::Result &result) {
		HandleReadStream(result);
	}

	virtual void handle_write_stream(const ACE_Asynch_Write_Stream::Result &result) {
		HandleWriteStream(result);
	}

	virtual void handle_time_out(const ACE_Time_Value &, const void * /*act*/ = 0) {

		Lock lock(m_mutex, true);

		if (m_idleTimeoutTimer < 0) {
			Log::GetInstance().AppendDebug(
				"Canceling idle timeout for connection %1%...",
				m_instanceId);
			assert(m_idleTimeoutTimer == -2);
			CheckedDelete(lock);
			return;
		}
		
		const auto idleTimeoutUpdateTime = m_idleTimeoutUpdateTime;
		if (!(idleTimeoutUpdateTime < 0)) {
			const auto idleTime = IdleTimeoutPolicy::GetIdleTime(
				m_startTime,
				idleTimeoutUpdateTime);
			if (idleTime < m_idleTimeoutInterval) {
				const auto sleepTime = IdleTimeoutPolicy::CalcSleepTime(
					m_idleTimeoutInterval,
					idleTime);
				const auto idleTimeoutTimer = m_proactor->schedule_timer(
					*this,
					nullptr,
					IdleTimeoutPolicy::IntervalToAceTime(sleepTime));
				assert(idleTimeoutTimer != -1);
				Interlocked::Exchange(m_idleTimeoutTimer, idleTimeoutTimer);
				return;
			}
		}
		assert(idleTimeoutUpdateTime >= -1);

		const auto signal = m_signal;
		Interlocked::Exchange(m_idleTimeoutTimer, -3);
		lock.release();
		Log::GetInstance().AppendDebug(
			"Closing connection %1% by idle timeout...",
			m_instanceId);
		signal->OnConnectionClose(m_instanceId);

	}

public:

	AutoPtr<MessageBlock> CreateMessageBlock(
				size_t size,
				const char *data = nullptr)
			const {

		assert(size > 0);
		AutoPtr<UniqueMessageBlockHolder> result(new UniqueMessageBlockHolder);

		for (bool isSizeError = false; ; ) {

			{
				ExternalMessagesAllocatorReadLock readLock(
					m_externalMessagesAllocatorMutex);
				if (
						m_externalMessagesAllocator
						&& UniqueMessageBlockHolder::GetMessageMemorySize(size)
							<= m_externalMessagesAllocator->GetDataBlockSize()) {
					result->Reset(
						UniqueMessageBlockHolder::Create(
							size,
							m_externalMessagesAllocator,
							false));
					if (result->IsSet()) {
						result->SetReceivingTimePoint();
						if (!data || result->Get().copy(data, size) != -1) {
							return result;
						}
					}
					Log::GetInstance().AppendDebugEx(
						[this, size]() -> Format {
							Format message(
								"External messaging buffer for connection %1%"
									" should be reallocated with new block size:"
									" %2% -> %3%.");
							message
								% this->m_instanceId
								% (m_externalMessagesAllocator->GetDataBlockSize()
									- UniqueMessageBlockHolder::GetMessageMemorySize(0))
								% size;
							return message;
						});
					assert(!isSizeError);
					if (isSizeError) {
						throw TunnelEx::InsufficientMemoryException(
							L"Insufficient memory for external messaging buffer");
					}
					isSizeError = true;
				}
			} // releasing read lock
			
			ExternalMessagesAllocatorWriteLock writeLock(
				m_externalMessagesAllocatorMutex);
			if (isSizeError) {
				assert(m_externalMessagesAllocator);
				const_cast<Implementation *>(this)
					->CreateNewExternalMessagesAllocator(
						2,
						m_externalMessagesAllocator->GetDataBlockSize()
							- UniqueMessageBlockHolder::GetMessageMemorySize(0));
			} else if (!m_externalMessagesAllocator) {
				const_cast<Implementation *>(this)
					->CreateNewExternalMessagesAllocator(
						1,
						std::max(MessagesAllocator::DefautDataBlockSize, size));
			} else if (
					UniqueMessageBlockHolder::GetMessageMemorySize(size)
						> m_externalMessagesAllocator->GetDataBlockSize()) {
				const_cast<Implementation *>(this)
					->CreateNewExternalMessagesAllocator(1, size);
			}
			assert(m_externalMessagesAllocator);

		} // for ( ; ; )

	}

private:

	void CreateNewExternalMessagesAllocator(size_t ratio, size_t dataBlockSize) {
		const auto queueBufferSize
			= (MessagesAllocator::DefautConnectionBufferSize * ratio)
				/ UniqueMessageBlockHolder::GetMessageMemorySize(dataBlockSize);
		boost::shared_ptr<MessagesAllocator> allocator(
			new MessagesAllocator(
				queueBufferSize + 1,
				queueBufferSize,
				UniqueMessageBlockHolder::GetMessageMemorySize(dataBlockSize)));
		allocator.swap(m_externalMessagesAllocator);
	}

	void CollectLatencyStat(const UniqueMessageBlockHolder &message) {
		assert(IsNotLockedByMyThread(m_mutex));
		m_latencyStat.Accumulate(
			message,
			m_readStartAttemptsCount > 0
				?	(m_readsMallocFailsCount * 100) / m_readStartAttemptsCount
				:	0);
	}

	void ReportSendError(int errorNo) const {
		switch (errorNo) {
			case ERROR_NETNAME_DELETED: // see TEX-553
			case WSAECONNABORTED:
			case WSAECONNRESET:
				Log::GetInstance().AppendDebugEx(
					[this, errorNo]() -> Format {
						const Error error(errorNo);
						Format message(
							"Write operation for connection %1% has been canceled: %2% (%3%).");
						message
							% this->m_instanceId
							% error.GetStringA()
							% error.GetErrorNo();
						return message;
					});
				return;
			default:
				if (Log::GetInstance().IsCommonErrorsRegistrationOn()) {
					const Error error(errorNo);
					WFormat message(L"Failed to send data to connection %1%: %2% (%3%)");
					message % m_instanceId % error.GetStringW() % error.GetErrorNo();
					throw SystemException(message.str().c_str());
				}
		}
	}

	void CancelReadingState() throw() {
		assert(IsLockedByMyThread(m_mutex));
		assert(m_refsCount > 0);
		assert(
			m_readingState == RS_STOPPING_READING
			|| m_readingState == RS_READING);
		switch (m_readingState) {
			case RS_STOPPING_READING:
				m_readingState = RS_NOT_STARTED;
				break;
			case  RS_READING:
				m_readingState = RS_NOT_READING;
				break;
		}
	}

	template<typename Result>
	void HandleReadStream(const Result &result) {

		UniqueMessageBlockHolder messageBlock(result.message_block());
		messageBlock.SetReceivingTimePoint();
		assert(messageBlock.IsTunnelMessage());

		if (!result.success()) {
			messageBlock.Reset();
			Interlocked::CompareExchange(
				m_closeCode,
				result.error(),
				m_closeCodeNotSetValue);
			ReportReadError(result);
			Lock lock(m_mutex, true);
			CancelReadingState();
			m_signal->OnConnectionClose(m_instanceId);
			assert(m_refsCount > 0);
			CheckedDelete(lock);
			return;
		} else if (result.bytes_transferred() == 0) {
			messageBlock.Reset();
			Interlocked::CompareExchange(
				m_closeCode,
				result.error(),
				m_closeCodeNotSetValue);
			Log::GetInstance().AppendDebug(
				"Connection %1% closed by remote side.",
				m_instanceId);
			Lock lock(m_mutex, true);
			CancelReadingState();
			m_signal->OnConnectionClose(m_instanceId);
			assert(m_refsCount > 0);
			CheckedDelete(lock);
			return;
		}
		
		bool isSuccess = false;
		try {
			Lock lock(m_mutex, true);
			CancelReadingState();
			assert(
				m_setupState == SETUP_STATE_NOT_COMPLETED
				|| m_setupState == SETUP_STATE_COMPLETED);
			assert(!IsSetupFailed());
			if (!m_isClosed) {
				m_myInterface.ReadRemote(messageBlock);
				isSuccess = InitMessageReading(true);
			}
		} catch (const TunnelEx::LocalException &ex) {
			Format message("%1% (reading connection %2%)");
			message % WString(ex.GetWhat()) % m_instanceId;
			Log::GetInstance().AppendError(message.str());
		}

		if (!isSuccess) {
			m_signal->OnConnectionClose(m_instanceId);
		}

		RemoveRef(true);

	}

	template<typename Result>
	void ReportReadError(const Result &result) const {
		switch (result.error()) {
			case ERROR_MORE_DATA: // see TEX-685
				break;
			case ERROR_NETNAME_DELETED: // see TEX-553
			case ERROR_BROKEN_PIPE: // see TEX-338
			case ERROR_PIPE_NOT_CONNECTED:
			case ERROR_OPERATION_ABORTED:
			case ERROR_CONNECTION_ABORTED:
				Log::GetInstance().AppendDebugEx(
					[this, &result]() -> Format {
						const Error error(result.error());
						Format message(
							"Read operation for connection %1% has been canceled: %2% (%3%).");
						message
							% this->m_instanceId
							% error.GetStringA()
							% error.GetErrorNo();
						return message;
					});
				break;
			default:
				if (Log::GetInstance().IsCommonErrorsRegistrationOn()) {
					const Error error(result.error());
					Format message("Failed read from connection %1%: %2% (%3%).");
					message % m_instanceId % error.GetStringA() % error.GetErrorNo();
					Log::GetInstance().AppendError(message.str());
				}
				break;
		}
	}
	
	template<typename Result>
	void HandleWriteStream(const Result &result) {
		UniqueMessageBlockHolder messageBlock(result.message_block());
		messageBlock.SetSendingTimePoint();
		m_signal->OnMessageBlockSent(messageBlock);
		RemoveRef(true);
	}

	bool InitMessageReading(bool isForcedInit) {

		assert(IsLockedByMyThread(m_mutex));
		assert(!m_isClosed);

		if (m_readingState < RS_NOT_READING) {
			return true;
		}
		assert(m_readingState >= RS_NOT_READING);

		if (isForcedInit) {
			Interlocked::Increment(m_readStartAttemptsCount);
		}
		UniqueMessageBlockHolder messageBlock(
			UniqueMessageBlockHolder::Create(
				m_tunnelMessagesAllocator->GetDataBlockSize()
					- UniqueMessageBlockHolder::GetMessageMemorySize(0),
				m_tunnelMessagesAllocator,
				true));
		if (!messageBlock.IsSet()) {
			if (isForcedInit) {
				Interlocked::Increment(m_readsMallocFailsCount);
			}
			return true;
		}
		messageBlock.SetReceivingStartTimePoint();

		ACE_Message_Block &aceMessageBlock = messageBlock.Get();
		if (m_readStreamFunc(aceMessageBlock, aceMessageBlock.space()) == -1) {
			const Error error(errno);
			WFormat message(
				L"Could not initiate read stream for connection %3%: %1% (%2%)");
			message
				% error.GetStringW()
				% error.GetErrorNo()
				% m_instanceId;
			switch (error.GetErrorNo()) {
				case ERROR_NETNAME_DELETED: // see TEX-553
				case ERROR_BROKEN_PIPE:
					Log::GetInstance().AppendDebug(message);
					messageBlock.Reset();
					return false;
			}
			throw ConnectionException(message.str().c_str());
		}
		m_readingState = RS_READING;
		// incrementing only here as "isClosed + locking" guaranties that 
		// the m_refsCount is not zero and object will not destroyed from
		// another thread (also see write-init incrimination)
		Interlocked::Increment(m_refsCount);
		messageBlock.Release();

		return true;
	
	}

	void UpdateIdleTimer(const pt::ptime &eventTime) throw() {
		assert(!eventTime.is_not_a_date_time());
		if (m_idleTimeoutTimer < 0) {
			return;
		}
		try {
			Interlocked::Exchange(
				m_idleTimeoutUpdateTime,
				IdleTimeoutPolicy::GetEventOffset(m_startTime, eventTime));
		} catch (...) {
			Log::GetInstance().AppendError("Failed to update connection idle timer.");
			assert(false);
		}
	}

	void StartIdleTimer() {
		
		assert(m_idleTimeoutTimer == -1);
		assert(m_idleTimeoutUpdateTime == -1);
		
		if (m_idleTimeoutInterval == 0) {
			return;
		}
		Log::GetInstance().AppendDebugEx(
			[this]() -> Format {
				Format message(
					"Setting idle timeout %1% for connection %2%...");
				message
					% this->m_idleTimeoutInterval
					% this->m_instanceId;
				return message;
			});
		
		assert(IsNotLockedOrLockedByMyThread(m_mutex));
		{
			Lock lock(m_mutex, false);
			// no interlocking needed, work not started yet
			m_idleTimeoutTimer = m_proactor->schedule_timer(
				*this,
				nullptr,
				IdleTimeoutPolicy::IntervalToAceTime(m_idleTimeoutInterval));
			assert(m_idleTimeoutTimer >= 0);
		}

	}

private:
	
	Connection &m_myInterface;
	const Id m_instanceId;

	volatile long m_refsCount;
	
	const RuleEndpoint &m_ruleEndpoint;
	const SharedPtr<const EndpointAddress> m_ruleEndpointAddress;
	
	SharedPtr<ConnectionSignal> m_signal;
	
	std::unique_ptr<ACE_Asynch_Operation> m_readStream;
	std::unique_ptr<ACE_Asynch_Operation> m_writeStream;
	boost::function<int(ACE_Message_Block &, size_t)> m_readStreamFunc;
	boost::function<int(ACE_Message_Block &, size_t)> m_writeStreamFunc;
	
	mutable Mutex m_mutex;

	ReadingState m_readingState;
	SetupState m_setupState;
	
	ACE_Proactor *m_proactor;

	volatile long m_isClosed;

	boost::shared_ptr<MessagesAllocator> m_tunnelMessagesAllocator;
	
	mutable ExternalMessagesAllocatorMutex m_externalMessagesAllocatorMutex;
	boost::shared_ptr<MessagesAllocator> m_externalMessagesAllocator;

	//! @todo: create separated structure for these vars
	const pt::ptime m_startTime;
	const long m_idleTimeoutInterval;
	volatile long m_idleTimeoutTimer;
	volatile long m_idleTimeoutUpdateTime;

	LatencyStat m_latencyStat;
	volatile long m_readStartAttemptsCount;
	volatile long m_readsMallocFailsCount;

	volatile long m_closeCode;
	static const long m_closeCodeNotSetValue;

};

const long Connection::Implementation::m_closeCodeNotSetValue
	= std::numeric_limits<long>::min();

//////////////////////////////////////////////////////////////////////////

Connection::Connection(
			const RuleEndpoint &ruleLocalEndpoint,
			SharedPtr<const EndpointAddress> &ruleEndpointAddress) {
	m_pimpl = new Implementation(*this, ruleLocalEndpoint, ruleEndpointAddress);
}

Connection::Connection(
			const RuleEndpoint &ruleLocalEndpoint,
			SharedPtr<const EndpointAddress> &ruleEndpointAddress,
			TimeSeconds idleTimeoutSeconds) {
	m_pimpl = new Implementation(*this, ruleLocalEndpoint, ruleEndpointAddress, idleTimeoutSeconds);
}

Connection::~Connection() {
	delete m_pimpl;
}

void Connection::Open(SharedPtr<ConnectionSignal> signal, Mode mode) {
	m_pimpl->Open(signal, mode);
}

void Connection::Close() throw() {
	m_pimpl->Close();
}

DataTransferCommand Connection::WriteDirectly(MessageBlock &messageBlock) {
	return m_pimpl->SendToRemote(messageBlock);
}

DataTransferCommand Connection::Write(MessageBlock &messageBlock) {
	return WriteDirectly(messageBlock);
}

DataTransferCommand Connection::SendToRemote(MessageBlock &messageBlock) {
	return Write(messageBlock);
}

void Connection::SendToTunnel(MessageBlock &messageBlock) {
	m_pimpl->SendToTunnel(messageBlock);
}

AutoPtr<MessageBlock> Connection::CreateMessageBlock(
			size_t size,
			const char *data /*= nullptr*/)
		const {
	return m_pimpl->CreateMessageBlock(size, data);
}

const RuleEndpoint & Connection::GetRuleEndpoint() const {
	return m_pimpl->GetRuleEndpoint();
}

SharedPtr<const EndpointAddress> Connection::GetRuleEndpointAddress() const {
	return m_pimpl->GetRuleEndpointAddress();
}

void Connection::OnMessageBlockSent(MessageBlock &messageBlock) {
	m_pimpl->OnMessageBlockSent(messageBlock);
}

void Connection::ReadRemote(MessageBlock &messageBlock) {
	m_pimpl->ReadRemote(messageBlock);
}

void Connection::Setup() {
	m_pimpl->OnSetupSuccess(true);
}

void Connection::StartSetup() {
	Setup();
}

void Connection::CancelSetup(const ::TunnelEx::WString &reason) {
	m_pimpl->OnSetupFail(reason);
}

void Connection::StartReadingRemote() {
	m_pimpl->StartReading();
}

void Connection::StopReadingRemote() {
	m_pimpl->StopReading();
}

bool Connection::IsSetupCompleted() const {
	return m_pimpl->IsSetupCompleted();
}

bool Connection::IsSetupFailed() const {
	return m_pimpl->IsSetupFailed();
}

bool Connection::IsOneWay() const {
	return false;
}

long Connection::GetCloseCode() const throw() {
	return m_pimpl->GetCloseCode();
}
