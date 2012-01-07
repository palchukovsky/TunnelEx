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
#ifdef TUNNELEX_OBJECTS_DELETION_CHECK
#	include "Server.hpp"
#endif

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

#ifdef DEV_VER
namespace {

	////////////////////////////////////////////////////////////////////////////////

	struct DebugLockStat {

		static volatile long locks;
		static volatile long fails;
		static volatile long proactor;

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
	
	const double DebugLockStat::errorLevel = 1;
	const double DebugLockStat::warnLevel = .85;
	const double DebugLockStat::infoLevel = .5;

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
				Interlocked::Increment(&DebugLockStat::fails);
			}

			if (isProactorCall) {
				Interlocked::Increment(&DebugLockStat::proactor);
			}

			if (!(Interlocked::Increment(&DebugLockStat::locks) % 20000)) {
				DebugLockStat::Report();
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

	typedef TypedMutex<ACE_Recursive_Thread_Mutex, MutexTypeToType<MT_DEFAULT>>
		Mutex;
	typedef LockWithDebugReports<Mutex> Lock;

	typedef MessageBlocksLatencyStat LatencyStat;

public:

	explicit Implementation(
				Connection &myInteface,
				const RuleEndpoint &ruleEndpoint,
				SharedPtr<const EndpointAddress> &ruleEndpointAddress,
				TimeSeconds idleTimeoutSeconds = 0)
			: m_myInterface(myInteface),
			m_instanceId(m_myInterface.GetInstanceId()),
			m_refsCount(1),
			m_ruleEndpoint(ruleEndpoint),
			m_ruleEndpointAddress(ruleEndpointAddress),
			m_isReadingAllowed(false),
			m_isReadingInitiated(0),
			m_isReadingActive(true),
			m_isSetupCompleted(false),
			m_isSetupCompletedWithSuccess(false),
			m_proactor(nullptr),
			//! @todo: hardcode, get MTU, see TEX-542 [2010/01/20 21:18]
			m_dataBlockSize(MessagesAllocator::DefautDataBlockSize),
			//! @todo: hardcoded memory size
 			m_messageBlockQueueBufferSize(
 				(MessagesAllocator::DefautConnectionBufferSize * 1)
 					/ UniqueMessageBlockHolder::GetMessageMemorySize(m_dataBlockSize)),
			m_sentMessageBlockQueueSize(0),
			m_isClosed(false),
			m_idleTimeoutInterval(0, 0, idleTimeoutSeconds),
			m_idleTimeoutTimer(-1),
			//! @todo: hardcoded - latency stat period (secs)
			m_latencyStat(m_instanceId, 60) {
		TUNNELEX_OBJECTS_DELETION_CHECK_CTOR(m_instancesNumber);
#		ifdef DEV_VER
			Log::GetInstance().AppendDebug(
				"New connection object %1% created. Active objects: %2%.",
				m_instanceId,
				m_instancesNumber);
#		endif
	}
	
private:

	//! D-or is private, use ScheduleDeletion instead.
	virtual ~Implementation() throw() {
		if (!m_isSetupCompleted) {
			m_ruleEndpointAddress->StatConnectionSetupCanceling();
		}
		m_latencyStat.Dump();
#		ifdef DEV_VER
			Log::GetInstance().AppendDebug(
				"Connection object %1% deleted."
					" Active objects: %2%."
					" Messages satellites: %3%.",
				m_instanceId,
				m_instancesNumber,
				UniqueMessageBlockHolder::GetSatellitesInstancesNumber());
			DebugLockStat::Report();
#		endif
		TUNNELEX_OBJECTS_DELETION_CHECK_DTOR(m_instancesNumber);
	}

public:

	void CheckedDelete() {
		if (!m_proactor) {
			assert(!m_signal);
			delete this;
		} else {
			Lock lock(m_mutex, false);
			CheckedDelete(lock);
		}
	}

private:

	//! Deletes or prepares object to delete.
	/** @return true if object was deleted
	  */
	bool CheckedDelete(Lock &lock) {

		assert(m_proactor);
		assert(m_refsCount > 0);

		Interlocked::Exchange(m_isClosed, true);
		m_readStream.reset();
		m_writeStream.reset();
		m_allocator.reset();
		
		if (m_idleTimeoutTimer >= 0) {
			const auto timerId = m_idleTimeoutTimer;
			m_idleTimeoutTimer = -2;
			if (m_proactor->cancel_timer(timerId, nullptr, false) != 1) {
				return false;
			}
		}

		lock.release();
		
		if (Interlocked::Decrement(m_refsCount) > 0) {
			return false;
		}

		UncheckedDelete();
		return true;

	}

	//! Object can be deleted after calling.
	void RemoveRef(const bool isProactorCall) {
		assert(isProactorCall);
		if (Interlocked::Decrement(m_refsCount) > 0) {
			return;
		}
		{
			Lock lock(m_mutex, isProactorCall);
			if (m_refsCount > 0) {
				return;
			}
		}
		UncheckedDelete();
	}

	void UncheckedDelete() {
		assert(m_refsCount == 0);
		assert(!m_readStream);
		assert(!m_writeStream);
		assert(!m_allocator);
		assert(m_isClosed);
		const auto instanceId = m_instanceId;
		const auto signal = m_signal;
		delete this;
		signal->OnConnectionClosed(instanceId);		
	}

public:

	void Open(SharedPtr<ConnectionSignal> &signal, Connection::Mode mode) {

		const bool isReadingAllowed = mode != Connection::MODE_WRITE;

		Lock lock(m_mutex, false);

		assert(!m_proactor || m_proactor == &signal->GetTunnel().GetProactor());
		assert(!m_proactor || m_isReadingAllowed == isReadingAllowed);
		assert(!m_allocator);

		if (m_proactor) {
			if (	m_proactor != &signal->GetTunnel().GetProactor()
					|| m_isReadingAllowed != isReadingAllowed) {
				throw TunnelEx::LogicalException(L"Could not open data transfer twice");
			}
			m_signal = signal;
			Log::GetInstance().AppendDebug(
				"Connection %1% already opened.",
				m_instanceId);
			return;
		}

		boost::shared_ptr<MessagesAllocator> allocator(
			!isReadingAllowed
				?	new MessagesAllocator(
						1 + 1, // plus 1 for message, that duplicated for proactor
						1,
						UniqueMessageBlockHolder::GetMessageMemorySize(m_dataBlockSize))
				:	new MessagesAllocator(
						m_messageBlockQueueBufferSize + 1, // plus 1 for message, that duplicated for proactor
						m_messageBlockQueueBufferSize,
						UniqueMessageBlockHolder::GetMessageMemorySize(m_dataBlockSize)));

		ACE_Proactor &proactor = signal->GetTunnel().GetProactor();
			
		IoHandleInfo ioHandleInfo = m_myInterface.GetIoHandle();
			
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

		allocator.swap(m_allocator);
		m_writeStream.reset(writeStream.release());
		m_readStream.reset(readStream.release());
		readStreamFunc.swap(m_readStreamFunc);
		writeStreamFunc.swap(m_writeStreamFunc);
		m_isReadingAllowed = isReadingAllowed;
		signal.Swap(m_signal);
		m_proactor = &proactor;

	}

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
			UpdateIdleTimer(messageBlockHolder.GetSendingStartTime());
			lock.release();
			messageBlockDuplicate.Release();
			messageBlockHolder.MarkAsAddedToQueue();
		}

		return DATA_TRANSFER_CMD_SEND_PACKET;
	
	}

public:

	void OnMessageBlockSent(const MessageBlock &messageBlock) {
		assert(IsNotLockedByMyThread(m_mutex));
		if (!messageBlock.IsTunnelMessage()) {
			CollectLatencyStat(messageBlock);
			return;
		}
		Interlocked::Decrement(&m_sentMessageBlockQueueSize);
		CollectLatencyStat(messageBlock);
		if (m_isReadingActive) {
			return;
		}
		{
			Lock lock(m_mutex, false);
			if (m_isReadingActive || InitReadIfPossible(lock)) {
				return;
			}
		}
		m_signal->OnConnectionClose(m_instanceId);
	}

	void OnSetupSuccess() {
		assert(IsNotLockedOrLockedByMyThread(m_mutex));
		CompleteSetup(true);
		m_ruleEndpointAddress->StatConnectionSetupCompleting();
		m_signal->OnConnectionSetupCompleted(m_instanceId);
		StartIdleTimer();
	}

	void OnSetupFail(const WString &failReason) {
		assert(IsNotLockedOrLockedByMyThread(m_mutex));
		CompleteSetup(false);
		m_ruleEndpointAddress->StatConnectionSetupCanceling(failReason);
		Log::GetInstance().AppendDebug(
			"Setup for connection %1% has been canceled - connection will be closed.",
			m_instanceId);
		m_signal->OnConnectionClose(m_instanceId);
	}

public:

	void StartRead() {
		assert(!m_isReadingInitiated);
		{
			Lock lock(m_mutex, false);
			m_isReadingInitiated = true;
			assert(!m_isClosed); // sanity check, never should be true
			if (InitReadIfPossible(lock)) {
				return;
			}
		}
		Log::GetInstance().AppendDebug(
			"Failed to start reading for connection %1%.",
			m_instanceId);
		m_signal->OnConnectionClose(m_instanceId);
	}

	void StopRead() {
		// Protected in Connection. Also see the comment about locking assert in
		// the SendToTunnelUnsafe-method.
		assert(IsLockedByMyThread(m_mutex) || !m_isSetupCompleted);
		assert(m_isReadingInitiated);
		m_isReadingInitiated = false;
	}

	const RuleEndpoint & GetRuleEndpoint() const {
		return m_ruleEndpoint;
	}

	SharedPtr<const EndpointAddress> GetRuleEndpointAddress() const {
		return m_ruleEndpointAddress;
	}

	void SendToTunnel(MessageBlock &messageBlock) {
		assert(IsNotLockedByMyThread(m_mutex));
		Lock lock(m_mutex, false);
		SendToTunnelUnsafe(messageBlock);
	}

	void SendToTunnelUnsafe(MessageBlock &messageBlock) {
		// It must be locked by "my" thread or in the setup process (locking not
		// required at setup). Ex.: UDP incoming connection works so: starts read,
		// sends initial data, stops read, completes setup.
		assert(IsLockedByMyThread(m_mutex) || !m_isSetupCompleted);
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
		if (messageBlock.IsAddedToQueue() && messageBlock.IsTunnelMessage()) {
			Interlocked::Increment(&m_sentMessageBlockQueueSize);
		}
	}

	bool IsSetupCompleted() const {
		return m_isSetupCompleted && m_isSetupCompletedWithSuccess;
	}

	bool IsSetupFailed() const {
		return m_isSetupCompleted && !m_isSetupCompletedWithSuccess;
	}

public:

	virtual void handle_read_file(const ACE_Asynch_Read_File::Result &result) {
		DoHandleReadStream(result);
	}

	virtual void handle_write_file(const ACE_Asynch_Write_File::Result &result) {
		DoHandleWriteStream(result);
	}

	virtual void handle_read_stream(const ACE_Asynch_Read_Stream::Result &result) {
		DoHandleReadStream(result);
	}

	virtual void handle_write_stream(const ACE_Asynch_Write_Stream::Result &result) {
		DoHandleWriteStream(result);
	}

	virtual void handle_time_out(const ACE_Time_Value &, const void * /*act*/ = 0) {

		assert(m_idleTimeoutInterval.ticks() > 0);

		Lock lock(m_mutex, true);

		if (m_idleTimeoutTimer < 0) {
			assert(m_idleTimeoutTimer == -2);
			Log::GetInstance().AppendDebug(
				"Canceling idle timeout for connection %1%...",
				m_instanceId);
			CheckedDelete(lock);
			return;
		}
		
		if (!m_idleTimeoutUpdateTime.is_not_a_date_time()) {

			assert(pt::microsec_clock::local_time() >= m_idleTimeoutUpdateTime);
			const auto secondFromLastAction
				= pt::microsec_clock::local_time() - m_idleTimeoutUpdateTime;
			if (secondFromLastAction <= m_idleTimeoutInterval) {

				m_idleTimeoutUpdateTime = pt::not_a_date_time;

				const auto sleepTime
					= m_idleTimeoutInterval
						- secondFromLastAction
						+ pt::microseconds(1000000 - 1);
				Log::GetInstance().AppendDebugEx(
					[this, &sleepTime]() -> Format {
						Format message(
							"Resetting idle timeout to %1% seconds for connection %2%...");
						message % sleepTime.total_seconds() % this->m_instanceId;
						return message;
					});
				m_idleTimeoutTimer = m_proactor->schedule_timer(
					*this,
					nullptr,
					ACE_Time_Value(sleepTime.total_seconds()));
				assert(m_idleTimeoutTimer != -1);

				return;
			
			}

		}

		const auto signal = m_signal;
		m_idleTimeoutTimer = -3;
		lock.release();
		Log::GetInstance().AppendDebug(
			"Closing connection %1% by idle timeout...",
			m_instanceId);
		signal->OnConnectionClose(m_instanceId);

	}

	AutoPtr<MessageBlock> CreateMessageBlock(
				size_t size,
				const char *data = nullptr)
			const {
		assert(size > 0);
		AutoPtr<UniqueMessageBlockHolder> result(new UniqueMessageBlockHolder);
		result->Reset(
			&UniqueMessageBlockHolder::Create(size, m_allocator, false));
		result->SetReceivingTimePoint();
		if (data && result->Get().copy(data, size) == -1) {
			throw TunnelEx::InsufficientMemoryException(
				L"Insufficient message block memory");
		}
		return result;
	}

private:

	void CollectLatencyStat(const MessageBlock &message) {
		assert(IsNotLockedByMyThread(m_mutex));
		const UniqueMessageBlockHolder &messageHolder
			= *boost::polymorphic_downcast<const UniqueMessageBlockHolder *>(
				&message);
		assert(m_sentMessageBlockQueueSize <= m_messageBlockQueueBufferSize);
		m_latencyStat.Accumulate(
			messageHolder,
			(m_sentMessageBlockQueueSize * 100) / m_messageBlockQueueBufferSize);
	}

	void ReportSendError(int errorNo) const {
		const bool isClosedConnectionError
			= errorNo == ERROR_NETNAME_DELETED // see TEX-553
				|| errorNo == WSAECONNRESET;
		if (	isClosedConnectionError
				&& !Log::GetInstance().IsDebugRegistrationOn()) {
			return;
		}
		const Error error(errorNo);
		WFormat errorStr(L"Could not write data into stream: %1% (%2%)");
		errorStr % error.GetStringW() % error.GetErrorNo();
		if (isClosedConnectionError) {
			Log::GetInstance().AppendDebug(
				ConvertString<String>(errorStr.str().c_str()).GetCStr());
		} else {
			throw SystemException(errorStr.str().c_str());
		}
	}

	template<typename Result>
	void DoHandleReadStream(const Result &result) {

		UniqueMessageBlockHolder messageBlock(result.message_block());
		messageBlock.SetReceivingTimePoint();
		assert(messageBlock.IsTunnelMessage());

		bool isSuccess = false;
		if (!result.success() && ReportReadError(result)) {
			Log::GetInstance().AppendDebugEx(
				[this, &result]() -> Format {
					const Error error(result.error());
					Format message(
						"Closing connection %1% with error \"%2%\" (code: %3%)...");
					message
						% this->m_instanceId
						% error.GetStringA()
						% error.GetErrorNo();
					return message;
				});
			messageBlock.Reset();
			m_signal->OnConnectionClose(m_instanceId);
			Lock lock(m_mutex, true);
			CheckedDelete(lock);
			return;
		} else if (result.bytes_transferred() == 0) {
			Log::GetInstance().AppendDebug(
				"Connection %1% closed by remote side.",
				m_instanceId);
			messageBlock.Reset();
			m_signal->OnConnectionClose(m_instanceId);
			Lock lock(m_mutex, true);
			CheckedDelete(lock);
			return;
		} else if (m_isReadingAllowed && !m_isClosed) {
			Lock lock(m_mutex, true);
			try {
				if (m_isReadingAllowed && !m_isClosed) {
					m_myInterface.ReadRemote(messageBlock);
					InitReadIfPossible(lock);
					isSuccess = true;
				}
			} catch (const TunnelEx::LocalException &ex) {
				Log::GetInstance().AppendError(
					ConvertString<String>(ex.GetWhat()).GetCStr());
			}
		}

		messageBlock.Reset();
		if (!isSuccess) {
			m_signal->OnConnectionClose(m_instanceId);
		}

		RemoveRef(true);

	}

	template<typename Result>
	bool ReportReadError(const Result &result) const {
		
		switch (result.error()) {
			case ERROR_MORE_DATA: // see TEX-685
				return false;
			case ERROR_NETNAME_DELETED: // see TEX-553
			case ERROR_BROKEN_PIPE: // see TEX-338
			case ERROR_PIPE_NOT_CONNECTED:
				Log::GetInstance().AppendDebug(
					"Read operation has been canceled for connection %1%.",
					m_instanceId);
				return true;
			case ERROR_OPERATION_ABORTED:
				// don't tall about it anything - proactor just removed
				// message blocks for canceled operations.
				return true;
			default:
				break;
		}

		if (!Log::GetInstance().IsSystemErrorsRegistrationOn()) {
			const Error error(result.error());
			Format message(
				"Connection %3% read operation completes with error: %1% (%2%).");
			message
				% error.GetStringA()
				% error.GetErrorNo()
				% m_instanceId;
			Log::GetInstance().AppendSystemError(message.str().c_str());
		}
		
		return true;
	
	}
	
	template<typename Result>
	void DoHandleWriteStream(const Result &result) {
		UniqueMessageBlockHolder messageBlock(result.message_block());
		messageBlock.SetSendingTimePoint();
		m_signal->OnMessageBlockSent(messageBlock);
		RemoveRef(true);
	}

	//! Inits read buffer if it possible and allowed
	/**  Can be called only from proactor thread.
	  */
	bool InitReadIfPossible(const Lock &) {

		// FIXME
		// Can't explain this situation, try to do it at assert fail.
		// Who can stop reading if it already started and working?
		assert(m_isReadingInitiated);

		if (m_isClosed || !m_isReadingInitiated) {
			return true;
		} else if (m_sentMessageBlockQueueSize >= m_messageBlockQueueBufferSize) {
			if (m_isReadingActive) {
				const char *const message
					= "Message queue size is %1% packets (%2% allowed),"
						" data read from connection %3% will be suspended.";
				Log::GetInstance().AppendDebug(
					message,
					m_sentMessageBlockQueueSize,
					m_messageBlockQueueBufferSize,
					m_instanceId);
#				ifdef DEV_VER
				{
					Format message(message);
					message
						% m_sentMessageBlockQueueSize
						% m_messageBlockQueueBufferSize
						% m_instanceId;
					Log::GetInstance().AppendWarn(message.str());
				}
#				endif
				verify(Interlocked::Exchange(m_isReadingActive, false));
			}
			return true;
		} else if (!m_isReadingActive) {
			if (m_sentMessageBlockQueueSize >= m_messageBlockQueueBufferSize / 2) {
				return true;
			}
			const char *const message
				= "Message queue size is %1% packets (%2% allowed),"
					" data read from connection %3% will be resumed.";
			Log::GetInstance().AppendDebug(
				message,
				m_sentMessageBlockQueueSize,
				m_messageBlockQueueBufferSize,
				m_instanceId);
#			ifdef DEV_VER
			{
				Format message(message);
				message
					% m_sentMessageBlockQueueSize
					% m_messageBlockQueueBufferSize
					% m_instanceId;
				Log::GetInstance().AppendInfo(message.str());
			}
#			endif
			verify(!Interlocked::Exchange(m_isReadingActive, true));
		}

		// read init
		if (m_readStream.get()) { // ex UDP
			UniqueMessageBlockHolder messageBlock(
				UniqueMessageBlockHolder::Create(
					m_dataBlockSize,
					m_allocator,
					true));
			messageBlock.SetReceivingStartTimePoint();
			if (m_readStreamFunc(messageBlock.Get(), m_dataBlockSize) == -1) {
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
			// incrementing only here as "isClosed + locking" guaranties that 
			// the m_refsCount is not zero and object will not destroyed from
			// another thread (also see write-init incrimination)
			Interlocked::Increment(m_refsCount);
			messageBlock.Release();
		}

		return true;
	
	}

	void UpdateIdleTimer(const pt::ptime &eventTime) throw() {
		assert(!eventTime.is_not_a_date_time());
		// It must be locked by "my" thread or in the setup process (locking not
		// required at setup). Ex.: UDP incoming connection works so: starts read,
		// sends initial data, stops read, completes setup.
		assert(IsLockedByMyThread(m_mutex) || !m_isSetupCompleted);
		if (m_idleTimeoutTimer < 0) {
			return;
		}
		assert(m_idleTimeoutInterval.ticks() > 0);
		try {
			m_idleTimeoutUpdateTime = eventTime;
		} catch (...) {
			Log::GetInstance().AppendError("Failed to update connection idle timer.");
			assert(false);
		}
	}

	void CompleteSetup(bool setupCompletedWithSuccess) {
		assert(IsNotLockedOrLockedByMyThread(m_mutex));
		m_isSetupCompleted = true;
		m_isSetupCompletedWithSuccess = setupCompletedWithSuccess;
		// Must be not be started yet!
		assert(m_idleTimeoutTimer == -1);
	}

	void StartIdleTimer() {
		assert(m_idleTimeoutTimer == -1);
		assert(m_isSetupCompleted);
		assert(m_isSetupCompletedWithSuccess);
		assert(m_idleTimeoutUpdateTime.is_not_a_date_time());
		if (m_idleTimeoutInterval.ticks() == 0) {
			return;
		}
		Log::GetInstance().AppendDebugEx(
			[this]() -> Format {
				Format message(
					"Setting idle timeout %1% seconds for connection %2%...");
				message
					% this->m_idleTimeoutInterval.total_seconds()
					% this->m_instanceId;
				return message;
			});
		m_idleTimeoutTimer = m_proactor->schedule_timer(
			*this,
			nullptr,
			ACE_Time_Value(m_idleTimeoutInterval.total_seconds()));
		assert(m_idleTimeoutTimer >= 0);
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
	
	Mutex m_mutex;

	bool m_isReadingAllowed;
	
	bool m_isReadingInitiated;
	volatile long m_isReadingActive;
	bool m_isSetupCompleted;
	bool m_isSetupCompletedWithSuccess;
	
	ACE_Proactor *m_proactor;

	const size_t m_dataBlockSize;
	const long m_messageBlockQueueBufferSize;
	volatile long m_sentMessageBlockQueueSize;

	volatile long m_isClosed;

	boost::shared_ptr<MessagesAllocator> m_allocator;

	const pt::time_duration m_idleTimeoutInterval;
	long m_idleTimeoutTimer;
	pt::ptime m_idleTimeoutUpdateTime;

	LatencyStat m_latencyStat;

	TUNNELEX_OBJECTS_DELETION_CHECK_DECLARATION(m_instancesNumber);

};

TUNNELEX_OBJECTS_DELETION_CHECK_DEFINITION(Connection::Implementation, m_instancesNumber);

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
	m_pimpl->CheckedDelete();
}

void Connection::Open(SharedPtr<ConnectionSignal> signal, Mode mode) {
	m_pimpl->Open(signal, mode);
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

void Connection::SendToTunnelUnsafe(MessageBlock &messageBlock) {
	m_pimpl->SendToTunnelUnsafe(messageBlock);
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

void Connection::OnMessageBlockSent(const MessageBlock &messageBlock) {
	m_pimpl->OnMessageBlockSent(messageBlock);
}

void Connection::ReadRemote(MessageBlock &messageBlock) {
	SendToTunnelUnsafe(messageBlock);
}

void Connection::Setup() {
	m_pimpl->OnSetupSuccess();
}

void Connection::StartSetup() {
	Setup();
}

void Connection::CancelSetup(const ::TunnelEx::WString &reason) {
	m_pimpl->OnSetupFail(reason);
}

void Connection::StartReadRemote() {
	m_pimpl->StartRead();
}

void Connection::StopReadRemote() {
	m_pimpl->StopRead();
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
