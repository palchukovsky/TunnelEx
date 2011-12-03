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
#include "Format.hpp"
#include "MessageBlockHolder.hpp"
#include "Tunnel.hpp"
#include "TunnelBuffer.hpp"
#include "Error.hpp"
#include "Locking.hpp"
#include "ObjectsDeletionCheck.h"
#ifdef TUNNELEX_OBJECTS_DELETION_CHECK
#	include "Server.hpp"
#endif

using namespace TunnelEx;
using namespace TunnelEx::Helpers::Asserts;
	
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
		static volatile long recursive;

		static const double errorLevel;
		static const double warnLevel;
		static const double infoLevel;

		static void Report() {
			const auto failesLevel
				= (double(DebugLockStat::fails) / double(DebugLockStat::locks)) * 100;
			Format stat(
				"Connection locks/waits/proactor/recursive statistic: %1%/%2%/%3%/%4% (%5%%% waits).");
			stat
				% DebugLockStat::locks
				% DebugLockStat::fails
				% DebugLockStat::proactor
				% DebugLockStat::recursive
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
	volatile long DebugLockStat::recursive = 0;
	
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

		explicit LockWithDebugReports(Mutex &mutex, const bool isFromProactor)
				: Base(mutex, 0) {

			if (!locked()) {
				verify(acquire() != -1);
				Interlocked::Increment(&DebugLockStat::fails);
			}

			if (mutex.get_nesting_level() > 0) {
				Interlocked::Increment(&DebugLockStat::recursive);
			}

			if (isFromProactor) {
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
			m_dataBlockSize(1480), //! @todo: hardcode, get MTU, see TEX-542 [2010/01/20 21:18]
			m_messageBlockQueueBufferSize((256 * 1024) / m_dataBlockSize),
			m_sentMessageBlockQueueSize(0),
			m_closeAtLastMessageBlock(false),
			m_sendQueueSize(0),
			m_idleTimeoutInterval(idleTimeoutSeconds, 0),
			m_idleTimeoutTimer(-1),
			m_idleTimeoutUpdateTime(0) {
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
		//! @todo: fix (currently don't know when buffer deletion is secure)
		// m_buffer->DeleteBuffer(m_allocators);
		TUNNELEX_OBJECTS_DELETION_CHECK_DTOR(m_instancesNumber);
#		ifdef DEV_VER
			Log::GetInstance().AppendDebug(
				"Connection object %1% deleted. Active objects: %2%.",
				m_instanceId,
				m_instancesNumber);
			DebugLockStat::Report();
#		endif
	}

public:

	void CheckedDelete() {
		Lock lock(m_mutex, false);
		CheckedDelete(lock);
	}

private:

	//! Deletes or prepares object to delete.
	/** @return true if object was deleted
	  */
	bool CheckedDelete(Lock &lock) {
		
		assert(m_proactor);
		assert(m_refsCount <= 2);
		assert(m_refsCount >= 1);

		m_readStream.reset();
		m_writeStream.reset();
		
		if (m_idleTimeoutTimer >= 0) {
			const auto timerId = m_idleTimeoutTimer;
			m_idleTimeoutTimer = -2;
			if (m_proactor->cancel_timer(timerId, nullptr, false) != 1) {
				return false;
			}
		} 
		
		if (Interlocked::Decrement(&m_refsCount) > 0) {
			return false;
		}

		const auto instanceId = m_instanceId;
		const auto signal = m_signal;
		lock.release();
		delete this;

		if (signal) {
			signal->OnConnectionClosed(instanceId);
		}

		return true;

	}

	//! Closed connection and deletes or prepares object to delete.
	/** Can be called only from 1) reading handling at 0 2) from data send
	  * notification if m_closeAtLastMessageBlock is true and buffer now is
	  * empty
      * @return true if object was deleted
	  */
	void Close(Lock &lock) {
		assert(!m_closeAtLastMessageBlock || m_sendQueueSize == 0);
		if (m_sendQueueSize != 0) {
			Interlocked::Exchange(m_closeAtLastMessageBlock, true);
			return;
		}
		CloseUnsafe(lock);
	}

	void CloseForced(Lock &lock) {
		Interlocked::Exchange(m_closeAtLastMessageBlock, true);
		CloseUnsafe(lock);
	}

	void CloseUnsafe(Lock &lock) {
		m_signal->OnConnectionClose(m_instanceId);
		CheckedDelete(lock);
	}

public:

	void Open(SharedPtr<ConnectionSignal> &signal, Connection::Mode mode) {

		const bool isReadingAllowed = mode != Connection::MODE_WRITE;

		Lock lock(m_mutex, false);

		assert(!m_proactor || m_proactor == &signal->GetTunnel().GetProactor());
		assert(!m_proactor || m_isReadingAllowed == isReadingAllowed);
		assert(!IsOpened());

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

		boost::shared_ptr<TunnelBuffer> buffer;
		TunnelBuffer::Allocators allocators;

		try {

			buffer = signal->GetTunnel().GetBuffer();
			if (!isReadingAllowed) {
				allocators = buffer->CreateBuffer(
					1 + 1, // plus 1 for message, that duplicated for proactor
					1,
					m_dataBlockSize);
			} else {
				allocators = buffer->CreateBuffer(
					m_messageBlockQueueBufferSize + 1, // plus 1 for message, that duplicated for proactor
					m_messageBlockQueueBufferSize,
					m_dataBlockSize);
			}

			ACE_Proactor &proactor = signal->GetTunnel().GetProactor();
			
			IoHandleInfo ioHandleInfo = m_myInterface.GetIoHandle();
			
			std::auto_ptr<ACE_Asynch_Operation> readStream;
			std::auto_ptr<ACE_Asynch_Operation> writeStream;

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
					message % error.GetString().GetCStr() % error.GetErrorNo();
					throw ConnectionOpeningException(message.str().c_str());
				}
				if (	openWriteStreamFunc
						&& openWriteStreamFunc(*this, ioHandleInfo.handle, 0, &proactor) == -1) {
					const Error error(errno);
					WFormat message(
						L"Could not open connection write stream: %1% (%2%)");
					message % error.GetString().GetCStr() % error.GetErrorNo();	
					throw ConnectionOpeningException(message.str().c_str());
				}
			}

			m_allocators = allocators;
			m_buffer.swap(buffer);
			m_writeStream = writeStream;
			m_readStream = readStream;
			m_readStreamFunc.swap(readStreamFunc);
			m_writeStreamFunc.swap(writeStreamFunc);
			m_isReadingAllowed = isReadingAllowed;
			m_signal.Swap(signal);
			m_proactor = &proactor;
			++m_refsCount; // no interlocking needed

		} catch (...) {
			buffer->DeleteBuffer(allocators);
			throw;
		}
		
	}

	DataTransferCommand SendToRemote(const char *data, size_t size) {
		assert(size > 0);
		if (!size || m_closeAtLastMessageBlock) {
			return DATA_TRANSFER_CMD_SEND_PACKET;
		}
		//! @todo: reimplement, memory usage!!!
		// not using internal allocator for memory, so buffer can be with any size
		UniqueMessageBlockHolder messageBlock(new ACE_Message_Block(size));
		if (messageBlock.Get().copy(data, size) == -1) {
			throw TunnelEx::InsufficientMemoryException(
				L"Insufficient message block memory");
		}
		return SendToRemote(messageBlock);
	}

	DataTransferCommand SendToRemote(MessageBlock &messageBlock) {

		if (m_closeAtLastMessageBlock) {
			return DATA_TRANSFER_CMD_SEND_PACKET;
		}

		UniqueMessageBlockHolder &messageBlockHolder
			= *boost::polymorphic_downcast<UniqueMessageBlockHolder *>(&messageBlock);
		assert(messageBlock.GetUnreadedDataSize() > 0);
		UniqueMessageBlockHolder blockToSend(messageBlockHolder.Get().duplicate());

		Lock lock(m_mutex, false);
		assert(IsOpened());
		assert(m_writeStream.get());

		if (!m_writeStream.get()) {
			throw LogicalException(
				L"Could not send data in connection, which does not opened");
		}

		if (m_closeAtLastMessageBlock) {
			return DATA_TRANSFER_CMD_SEND_PACKET;
		}

		UpdateIdleTimer();

		Interlocked::Increment(m_sendQueueSize);
		const auto writeResult = m_writeStreamFunc(
			blockToSend.Get(),
			blockToSend.GetUnreadedDataSize());
		if (writeResult == -1) {
			Interlocked::Decrement(m_sendQueueSize);
			lock.release();
			ReportSendError(errno); // not only log, can throws
		} else {
			lock.release();
			blockToSend.Release();
			messageBlock.MarkAsAddedToQueue();
		}

		return DATA_TRANSFER_CMD_SEND_PACKET;
	
	}

public:

	void OnMessageBlockSent(const MessageBlock &messageBlock) {
		assert(IsNotLockedByMyThread(m_mutex));
		if (!messageBlock.IsTunnelMessage()) {
			return;
		}
		assert(m_sentMessageBlockQueueSize >= 0);
		Interlocked::Decrement(&m_sentMessageBlockQueueSize);
		assert(m_sentMessageBlockQueueSize >= 0);
		if (m_isReadingActive) {
			return;
		}
		{
			Lock lock(m_mutex, false);
			if (m_isReadingActive || InitReadIfPossible()) {
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
			if (InitReadIfPossible()) {
				return;
			}
		}
		assert(m_refsCount >= 2);
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

	void SendToTunnel(const char *data, size_t size) {
		assert(IsNotLockedByMyThread(m_mutex));
		Lock lock(m_mutex, false);
		SendToTunnelUnsafe(data, size);
	}

	void SendToTunnelUnsafe(MessageBlock &messageBlock) {
		// It must be locked by "my" thread or in the setup process (locking not
		// required at setup). Ex.: UDP incoming connection works so: starts read,
		// sends initial data, stops read, completes setup.
		assert(IsLockedByMyThread(m_mutex) || !m_isSetupCompleted);
		if (messageBlock.GetUnreadedDataSize() == 0) {
			return;
		}
		UpdateIdleTimer();
		m_signal->OnNewMessageBlock(messageBlock);
		if (messageBlock.IsAddedToQueue()) {
			Interlocked::Increment(&m_sentMessageBlockQueueSize);
		}
	}

	void SendToTunnelUnsafe(const char *data, size_t size) {
		assert(size > 0);
		if (!size) {
			return;
		}
		//! @todo: reimplement, memory usage!!!
		// not using internal allocator for memory, so buffer can be with any size
		UniqueMessageBlockHolder messageBlock(new ACE_Message_Block(size));
		if (messageBlock.Get().copy(data, size) == -1) {
			throw TunnelEx::InsufficientMemoryException(
				L"Insufficient message block memory");
		}
		SendToTunnelUnsafe(messageBlock);
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

		assert(m_idleTimeoutInterval != ACE_Time_Value::zero);
		assert(m_proactor);

		Lock lock(m_mutex, true);
		
		if (m_idleTimeoutUpdateTime > 0) {

			assert(time(nullptr) >= m_idleTimeoutUpdateTime);
			const auto secondFromLastAction = time(nullptr) - m_idleTimeoutUpdateTime;
			if (secondFromLastAction <= m_idleTimeoutInterval.sec()) {

				m_idleTimeoutUpdateTime = 0;

				const ACE_Time_Value sleepTime(
					m_idleTimeoutInterval.sec() - secondFromLastAction);
				Log::GetInstance().AppendDebug(
					"Resetting idle timeout %1% seconds for connection %2%...",
					sleepTime.sec(),
					m_instanceId);
				m_idleTimeoutTimer
					= m_proactor->schedule_timer(*this, nullptr, sleepTime);
				assert(m_idleTimeoutTimer != -1);
			
			} else {

				const auto signal = m_signal;
				const auto instanceId = m_instanceId;
				m_idleTimeoutTimer = -3;
				lock.release();
				Log::GetInstance().AppendDebug(
					"Closing connection %1% by idle timeout...",
					instanceId);
				signal->OnConnectionClose(instanceId);
			
			}

		} else {
			Log::GetInstance().AppendDebug(
				"Canceling idle timeout for connection %1%...",
				m_instanceId);
			CheckedDelete(lock);
		}

	}

private:
	
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
		errorStr % error.GetString().GetCStr();
		errorStr % error.GetErrorNo();
		if (isClosedConnectionError) {
			Log::GetInstance().AppendDebug(
				ConvertString<String>(errorStr.str().c_str()).GetCStr());
		} else {
			throw SystemException(errorStr.str().c_str());
		}
	}

	template<typename Result>
	void DoHandleReadStream(const Result &result) {

		assert(
			result.error() == ERROR_OPERATION_ABORTED
			|| IsNotLockedByMyThread(m_mutex));

		UniqueMessageBlockHolder messageBlock(result.message_block());
		assert(messageBlock.IsTunnelMessage());
		assert(
			result.error() == ERROR_OPERATION_ABORTED
			|| !m_closeAtLastMessageBlock);

		if (!result.success() && ReportReadError(result)) {
			
			Log::GetInstance().AppendDebug(
				"Closing connection with code %1%...",
				result.error());
			
			messageBlock.Reset();
			Lock lock(m_mutex, true);
			CloseForced(lock);
			return;
		
		} else if (result.bytes_transferred() == 0) {
			
			Log::GetInstance().AppendDebug(
				"Connection %1% closed by remote side.",
				m_instanceId);
		
		} else if (m_isReadingAllowed && IsOpened()) {
		
			Lock lock(m_mutex, true);
			try {
				if (IsOpened() && m_isReadingAllowed) {
					m_myInterface.ReadRemote(messageBlock);
					InitReadIfPossible();
					return;
				}
			} catch (const TunnelEx::LocalException &ex) {
				Log::GetInstance().AppendError(
					ConvertString<String>(ex.GetWhat()).GetCStr());
			}
		
		}

		messageBlock.Reset();
		m_signal->OnConnectionClose(m_instanceId);

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
				% ConvertString<String>(error.GetString()).GetCStr()
				% error.GetErrorNo()
				% m_instanceId;
			Log::GetInstance().AppendSystemError(message.str().c_str());
		}
		
		return true;
	
	}
	
	template<typename Result>
	void DoHandleWriteStream(const Result &result) {
		
		//! Waits until send method complies works to destroy Result without AV.
		class SenderWaiter : private boost::noncopyable {
		public:
			explicit SenderWaiter(Mutex &mutextRef) throw()
					: m_mutex(&mutextRef) {
				//...//
			}
			~SenderWaiter() throw() {
				if (m_mutex) {
					Lock(*m_mutex, true);
				}
			}
			void Release() throw() {
				assert(m_mutex);
				m_mutex = nullptr;
			}
		private:
			Mutex *m_mutex;
		} senderWaiter(m_mutex);

		UniqueMessageBlockHolder messageBlock(result.message_block());

		// FIXME
		// Sleep(500);
		assert(m_sendQueueSize > 0);
		Interlocked::Decrement(m_sendQueueSize);

		try {
			if (m_closeAtLastMessageBlock) { 
				Lock lock(m_mutex, true);
				senderWaiter.Release();
				if (m_sendQueueSize == 0) {
					messageBlock.Reset();
					CloseUnsafe(lock);
					return;
				}
			}
			m_signal->OnMessageBlockSent(messageBlock);
		} catch (const TunnelEx::LocalException &ex) {
			Log::GetInstance().AppendError(
				ConvertString<String>(ex.GetWhat()).GetCStr());
			m_signal->OnConnectionClose(m_instanceId);
		}

	}

	//! Inits read buffer if it possible and allowed
	/**  Can be called only from proactor thread.
	  */
	bool InitReadIfPossible() {

		assert(IsLockedByMyThread(m_mutex));

		// FIXME
		// Can't explain this situation, try to do it at assert fail.
		// Who can stop reading if it already started and working?
		assert(m_isReadingInitiated);
		
		if (!m_isReadingInitiated) {
			return true;
		} else if (m_sentMessageBlockQueueSize >= m_messageBlockQueueBufferSize) {
			if (m_isReadingActive) {
				const size_t bytes = m_sentMessageBlockQueueSize * m_dataBlockSize;
				Log::GetInstance().AppendDebug(
					"Data queue memory size is %1% bytes,"
						" data read from connection %2% will be suspended.",
					bytes,
					m_instanceId);
#				ifdef DEV_VER
				{
					Format message(
						"Data queue memory size is %1% bytes,"
							" data read from connection %2% will be suspended.");
					message % bytes % m_instanceId;
					Log::GetInstance().AppendWarn(message.str());
				}
#				endif
				Interlocked::Decrement(&m_isReadingActive);
				assert(m_isReadingActive == 0);
			}
			return true;
		} else if (!m_isReadingActive) {
			if (m_sentMessageBlockQueueSize >= m_messageBlockQueueBufferSize / 2) {
				return true;
			}
			const size_t bytes = m_sentMessageBlockQueueSize * m_dataBlockSize;
			Log::GetInstance().AppendDebug(
				"Data queue memory size is %1% bytes,"
					" data read from connection %2% will be resumed.",
				bytes,
				m_instanceId);
#			ifdef DEV_VER
			{
				Format message(
					"Data queue memory size is %1% bytes,"
						" data read from connection %2% will be resumed.");
				message % bytes % m_instanceId;
				Log::GetInstance().AppendInfo(message.str());
			}
#			endif
			Interlocked::Increment(&m_isReadingActive);
			assert(m_isReadingActive == 1);
		}

		// read init
		if (m_readStream.get()) { // ex UDP
			UniqueMessageBlockHolder messageBlock(
				UniqueMessageBlockHolder::CreateMessageBlockForTunnel(
					m_dataBlockSize,
					*m_allocators.messageBlock,
					*m_allocators.dataBlock,
					*m_allocators.dataBlockBuffer));
			if (m_readStreamFunc(messageBlock.Get(), m_dataBlockSize) == -1) {
				const Error error(errno);
				WFormat message(
					L"Could not initiate read stream for connection %3%: %1% (%2%)");
				message
					% error.GetString().GetCStr()
					% error.GetErrorNo()
					% m_instanceId;
				switch (error.GetErrorNo()) {
					case ERROR_NETNAME_DELETED: // see TEX-553
					case ERROR_BROKEN_PIPE:
						Log::GetInstance().AppendDebug(message.str().c_str());
						messageBlock.Reset();
						return false;
					default:
						throw ConnectionException(message.str().c_str());
				}
			}
			messageBlock.Release();
		}

		return true;
	
	}

	void UpdateIdleTimer() throw() {
		assert(IsLockedByMyThread(m_mutex));
		assert(m_proactor);
		if (m_idleTimeoutTimer < 0) {
			return;
		}
		assert(m_idleTimeoutInterval != ACE_Time_Value::zero);
		time(&m_idleTimeoutUpdateTime);
	}

	bool IsOpened() const {
		return m_refsCount >= 2 && !m_closeAtLastMessageBlock;
	}

	void CompleteSetup(bool setupCompletedWithSuccess) {
		assert(IsNotLockedOrLockedByMyThread(m_mutex));
		m_isSetupCompleted = true;
		m_isSetupCompletedWithSuccess = setupCompletedWithSuccess;
		// Must be not be started yet!
		assert(m_idleTimeoutTimer == -1);
	}

	void StartIdleTimer() {
		assert(m_proactor);
		assert(m_idleTimeoutTimer == -1);
		assert(m_isSetupCompleted);
		assert(m_isSetupCompletedWithSuccess);
		assert(m_idleTimeoutUpdateTime == 0);
		if (m_idleTimeoutInterval == ACE_Time_Value::zero) {
			return;
		}
		Log::GetInstance().AppendDebug(
			"Setting idle timeout %1% seconds for connection %2%...",
			m_idleTimeoutInterval.sec(),
			m_instanceId);
		m_idleTimeoutTimer
			= m_proactor->schedule_timer(*this, nullptr, m_idleTimeoutInterval);
		assert(m_idleTimeoutTimer >= 0);
	}

private:
	
	Connection &m_myInterface;
	const Id m_instanceId;

	volatile long m_refsCount;
	
	const RuleEndpoint &m_ruleEndpoint;
	const SharedPtr<const EndpointAddress> m_ruleEndpointAddress;
	
	SharedPtr<ConnectionSignal> m_signal;
	
	std::auto_ptr<ACE_Asynch_Operation> m_readStream;
	std::auto_ptr<ACE_Asynch_Operation> m_writeStream;
	boost::function<int(ACE_Message_Block &, size_t)> m_readStreamFunc;
	boost::function<int(ACE_Message_Block &, size_t)> m_writeStreamFunc;
	
	Mutex m_mutex;

	bool m_isReadingAllowed;
	
	bool m_isReadingInitiated;
	volatile long m_isReadingActive;
	bool m_isSetupCompleted;
	bool m_isSetupCompletedWithSuccess;
	
	// if null - not opened or closed by DoHandleReadStream
	ACE_Proactor *m_proactor;

	const size_t m_dataBlockSize;
	const long m_messageBlockQueueBufferSize;
	volatile long m_sentMessageBlockQueueSize;

	volatile long m_closeAtLastMessageBlock;
	volatile long m_sendQueueSize;

	boost::shared_ptr<TunnelBuffer> m_buffer;
	TunnelBuffer::Allocators m_allocators;

	const ACE_Time_Value m_idleTimeoutInterval;
	long m_idleTimeoutTimer;
	time_t m_idleTimeoutUpdateTime;

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

void Connection::WriteDirectly(const char *data, size_t size) {
	verify(m_pimpl->SendToRemote(data, size) == DATA_TRANSFER_CMD_SEND_PACKET);
}

DataTransferCommand Connection::Write(MessageBlock &messageBlock) {
	return WriteDirectly(messageBlock);
}

DataTransferCommand Connection::SendToRemote(MessageBlock &messageBlock) {
	return Write(messageBlock);
}

void Connection::SendToRemote(const char *data, size_t size) {
	WriteDirectly(data, size);
}

void Connection::SendToTunnel(MessageBlock &messageBlock) {
	m_pimpl->SendToTunnel(messageBlock);
}

void Connection::SendToTunnel(const char *data, size_t size) {
	m_pimpl->SendToTunnel(data, size);
}

void Connection::SendToTunnelUnsafe(MessageBlock &messageBlock) {
	m_pimpl->SendToTunnelUnsafe(messageBlock);
}

void Connection::SendToTunnelUnsafe(const char *data, size_t size) {
	m_pimpl->SendToTunnelUnsafe(data, size);
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
