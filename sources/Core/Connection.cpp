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
#include "ObjectsDeletionCheck.h"
#ifdef TUNNELEX_OBJECTS_DELETION_CHECK
# include "Server.hpp"
#endif

using namespace std;
using namespace boost;
using namespace TunnelEx;

//////////////////////////////////////////////////////////////////////////

class Connection::Implementation : public ACE_Handler {

private:

	typedef ACE_Recursive_Thread_Mutex StateMutex;
	typedef ACE_Guard<StateMutex> StateLock;
	
public:

	explicit Implementation(
				Connection &myInteface,
				const RuleEndpoint &ruleEndpoint,
				SharedPtr<const EndpointAddress> &ruleEndpointAddress)
			: m_myInterface(myInteface),
			m_instanceId(m_myInterface.GetInstanceId()),
			m_ruleEndpoint(ruleEndpoint),
			m_ruleEndpointAddress(ruleEndpointAddress),
			m_isReadingAllowed(false),
			m_isReadingInitiated(false),
			m_isReadingActive(true),
			m_isSetupCompleted(false),
			m_isSetupCompletedWithSuccess(false),
			m_proactor(0),
			m_dataBlockSize(1024), //! @todo: hardcode, get MTU, see TEX-542 [2010/01/20 21:18]
			m_messageBlockQueueBufferSize((1024 * 1024) / m_dataBlockSize), // 1 Mb
			m_sentMessageBlockQueueSize(0),
			m_closeAtLastMessageBlock(false),
			m_sendQueueSize(0),
			m_forceClosingMode(false),
			m_idleTimeoutToken(0),
			m_idleTimeoutTimer(-1) {
		TUNNELEX_OBJECTS_DELETION_CHECK_CTOR(m_instancesNumber);
	}
	
private:

	//! D-or is private, use ScheduleDeletion instead.
	virtual ~Implementation() throw() {
		if (!m_isSetupCompleted) {
			m_ruleEndpointAddress->StatConnectionSetupCanceling();
		}
		if (m_signal) {
			m_signal->OnConnectionClosed(m_instanceId);
		}
		BOOST_ASSERT(m_idleTimeoutTimer == -1);
		BOOST_ASSERT(m_proactor == 0);
		//! @todo: fix (currently don't know when buffer deletion is secure)
		// m_buffer->DeleteBuffer(m_allocators);
		TUNNELEX_OBJECTS_DELETION_CHECK_DTOR(m_instancesNumber);
		//! @todo: FIXME [2010/06/07 1:54]
		/* TUNNELEX_OBJECTS_DELETION_CHECK_ZERO(
			Server::GetInstance().GetTunnelsNumber() == 0,
			m_instancesNumber); */
	}

public:

	//! Schedules handler deletions from non-proactor's thread.
	/*	For proactor's thread use CloseFromProactor-method! */
	void ScheduleDeletion() {
		if (m_proactor) {
			StateLock lock(m_stateMutex);
			if (m_proactor) {
				if (m_sendQueueSize > 0 && !m_forceClosingMode) {
					Log::GetInstance().AppendDebug(
						"Flushing connection %1% buffer...",
						m_instanceId);
					m_closeAtLastMessageBlock = true;
				} else {
					if (m_idleTimeoutTimer != -1) {
						m_proactor->cancel_timer(m_idleTimeoutTimer);
						m_idleTimeoutTimer = -1;
					}
					m_proactor->schedule_timer(*this, 0, ACE_Time_Value::zero);
					m_proactor = 0;
				}
				return;
			}
		}
		delete this;
	}

	void ResetIdleTimeout(TimeSeconds seconds) {
		if (seconds > 0) {
			Log::GetInstance().AppendDebug(
				"Setting idle timeout %1% seconds for connection %2%...",
				seconds,
				m_instanceId);
		} else {
			Log::GetInstance().AppendDebug(
				"Disabling idle timeout for connection %1%...",
				m_instanceId);
		}
		StateLock lock(m_stateMutex);
		if (!m_proactor) {
			return;
		}
		ACE_Time_Value oldInterval(m_idleTimeoutInterval);
		m_idleTimeoutInterval.set(seconds);
		try {
			UpdateIdleTimer();
		} catch (...) {
			swap(oldInterval, m_idleTimeoutInterval);
			throw;
		}
	}

	void Open(SharedPtr<ConnectionSignal> &signal, Connection::Mode mode) {

		const bool isReadingAllowed = mode != Connection::MODE_WRITE;

		BOOST_ASSERT(!m_proactor || m_proactor == &signal->GetTunnel().GetProactor());
		BOOST_ASSERT(!m_proactor || m_isReadingAllowed == isReadingAllowed);

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

		shared_ptr<TunnelBuffer> buffer;
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
			
			auto_ptr<ACE_Asynch_Operation> readStream;
			auto_ptr<ACE_Asynch_Operation> writeStream;

			function<int(ACE_Handler &, ACE_HANDLE, const void *, ACE_Proactor *)>
				openReadStreamFunc;
			function<int(ACE_Handler &, ACE_HANDLE, const void *, ACE_Proactor *)>
				openWriteStreamFunc;

			function<int(ACE_Message_Block &, size_t)> readStreamFunc;
			function<int(ACE_Message_Block &, size_t)> writeStreamFunc;
			
			if (ioHandleInfo.handle != 0) {
				switch (ioHandleInfo.type) {
					default:
						BOOST_ASSERT(false);
					case IoHandleInfo::TYPE_OTHER:
						readStream.reset(new ACE_Asynch_Read_File);
						openReadStreamFunc = bind(
							&ACE_Asynch_Read_File::open,
							polymorphic_downcast<ACE_Asynch_Read_File *>(readStream.get()),
							_1,
							_2,
							_3,
							_4);
						readStreamFunc = bind(
							&ACE_Asynch_Read_File::read,
							polymorphic_downcast<ACE_Asynch_Read_File *>(readStream.get()),
							_1,
							_2,
							0,
							0,
							static_cast<void *>(0),
							0,
							ACE_SIGRTMIN);
						if (isReadingAllowed) {
							writeStream.reset(new ACE_Asynch_Write_File);
							openWriteStreamFunc = bind(
								&ACE_Asynch_Write_File::open,
								polymorphic_downcast<ACE_Asynch_Write_File *>(writeStream.get()),
								_1,
								_2,
								_3,
								_4);
							writeStreamFunc = bind(
								&ACE_Asynch_Write_File::write,
								polymorphic_downcast<ACE_Asynch_Write_File *>(writeStream.get()),
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
						openReadStreamFunc = bind(
							&ACE_Asynch_Read_Stream::open,
							polymorphic_downcast<ACE_Asynch_Read_Stream *>(readStream.get()),
							_1,
							_2,
							_3,
							_4);
						readStreamFunc = bind(
							&ACE_Asynch_Read_Stream::read,
							polymorphic_downcast<ACE_Asynch_Read_Stream *>(readStream.get()),
							_1,
							_2,
							static_cast<void *>(0),
							0,
							ACE_SIGRTMIN);
						if (isReadingAllowed) {
							writeStream.reset(new ACE_Asynch_Write_Stream);
							openWriteStreamFunc = bind(
								&ACE_Asynch_Write_Stream::open,
								polymorphic_downcast<ACE_Asynch_Write_Stream *>(writeStream.get()),
								_1,
								_2,
								_3,
								_4);
							writeStreamFunc = bind(
								&ACE_Asynch_Write_Stream::write,
								polymorphic_downcast<ACE_Asynch_Write_Stream *>(writeStream.get()),
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

		} catch (...) {
			buffer->DeleteBuffer(allocators);
			Proxy *const proxy = this->proxy_.get();
			if (proxy) {
				proxy->reset();
			}
			throw;
		}
		
	}

	DataTransferCommand SendToRemote(MessageBlock &messageBlock) {
		
		StateLock lock(m_stateMutex);

		BOOST_ASSERT(!m_closeAtLastMessageBlock);
		
		BOOST_ASSERT(m_writeStream.get());
		if (!m_writeStream.get()) {
			throw LogicalException(
				L"Could not send data in connection, which does not opened");
		}

		UpdateIdleTimer();

		UniqueMessageBlockHolder &messageBlockHolder
			= *polymorphic_downcast<UniqueMessageBlockHolder *>(&messageBlock);

		BOOST_ASSERT(messageBlock.GetUnreadedDataSize() > 0);
		UniqueMessageBlockHolder blockToSend(messageBlockHolder.Get().duplicate());
		const int writeResult = m_writeStreamFunc(
			blockToSend.Get(),
			blockToSend.GetUnreadedDataSize());
		if (writeResult == -1) {
			const Error error(errno);
			const bool isClosedConnectionError
				= error.GetErrorNo() == ERROR_NETNAME_DELETED // see TEX-553
				|| error.GetErrorNo() == WSAECONNRESET;
			if (	!isClosedConnectionError
					|| Log::GetInstance().IsDebugRegistrationOn()) {
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
		} else {
			blockToSend.Release();
			++m_sendQueueSize;
			messageBlock.MarkAsAddedToQueue();
		}

		return DATA_TRANSFER_CMD_SEND_PACKET;
	
	}

	void OnMessageBlockSent(const MessageBlock &messageBlock) {
		if (messageBlock.IsTunnelMessage()) {
			StateLock lock(m_stateMutex);
			BOOST_ASSERT(m_sentMessageBlockQueueSize > 0);
			--m_sentMessageBlockQueueSize;
			if (!m_isReadingActive && m_isReadingInitiated) {
				InitReadIfPossible();
			}
		}
	}

public:

	void OnSetupSuccess() {
		CompletedSetup(true);
		m_ruleEndpointAddress->StatConnectionSetupCompleting();
		m_signal->OnConnectionSetupCompleted(m_instanceId);
	}

	void OnSetupFail(const WString &failReason) {
		CompletedSetup(false);
		m_ruleEndpointAddress->StatConnectionSetupCanceling(failReason);
		Log::GetInstance().AppendDebug(
			"Setup for connection %1% has been canceled - connection will be closed.",
			m_instanceId);
		StateLock lock(m_stateMutex);
		if (m_proactor) {
			SharedPtr<ConnectionSignal> signal = m_signal;
			m_signal.Reset();
			// after "OnConnectionClose" object can be already destructed, so release mutex here
			lock.release();
			signal->OnConnectionClose(m_instanceId);
		}
	}

private:

	void CompletedSetup(bool setupCompletedWithSuccess) {
		StateLock lock(m_stateMutex);
		m_isSetupCompleted = true;
		m_isSetupCompletedWithSuccess = setupCompletedWithSuccess;
		// Must be not started yet!
		BOOST_ASSERT(m_idleTimeoutTimer == -1);
		if (m_isSetupCompleted) {
			UpdateIdleTimer();
		}
	}

public:

	void StartRead() {
		StateLock lock(m_stateMutex);
		BOOST_ASSERT(!m_isReadingInitiated);
		BOOST_ASSERT(m_proactor);
		if (m_isReadingInitiated) {
			return;
		}
		if (!m_closeAtLastMessageBlock) {
			InitRead();
		}
		m_isReadingInitiated = true;
	}

	void StopRead() {
		// works only for Connection::Read (proactor thread), so no locking needed
		m_isReadingInitiated = false;
	}

	void SetForceClosingMode() {
		m_forceClosingMode = true;
	}

	const RuleEndpoint & GetRuleEndpoint() const {
		return m_ruleEndpoint;
	}

	SharedPtr<const EndpointAddress> GetRuleEndpointAddress() const {
		return m_ruleEndpointAddress;
	}

	void SendToTunnel(MessageBlock &messageBlock) {
		if (messageBlock.GetUnreadedDataSize() == 0) {
			return;
		}
		StateLock lock(m_stateMutex);
		if (m_proactor == 0) {
			return;
		}
		BOOST_ASSERT(m_signal);
		UpdateIdleTimer();
		m_signal->OnNewMessageBlock(messageBlock);
		if (messageBlock.IsAddedToQueue()) {
			++m_sentMessageBlockQueueSize;
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

private:
	
	template<typename Result>
	void DoHandleReadStream(const Result &result) {
		{
			UniqueMessageBlockHolder messageBlock(result.message_block());
			BOOST_ASSERT(messageBlock.IsTunnelMessage());
			if (!result.success()) {
				const Error error(result.error());
				switch (error.GetErrorNo()) {
					case ERROR_NETNAME_DELETED: // see TEX-553
					case ERROR_BROKEN_PIPE: // see TEX-338
						Log::GetInstance().AppendDebug(
							"Read operation has been canceled for connection %1%, closing connection... ",
							m_instanceId);
						break;
					case ERROR_OPERATION_ABORTED:
						// don't tall about it anything - proactor just removed
						// message blocks for canceled operations.
						break;
					default:
						if (Log::GetInstance().IsSystemErrorsRegistrationOn()) {
							Format message(
								"Connection %3% read operation completes with error:"
									" %1% (%2%), closing connection...");
							message
								% ConvertString<String>(error.GetString()).GetCStr()
								% error.GetErrorNo()
								% m_instanceId;
							Log::GetInstance().AppendSystemError(message.str().c_str());
						}
						break;
				}
			} else if (result.bytes_transferred() == 0) {
				if (m_proactor && !m_closeAtLastMessageBlock) {
					StateLock lock(m_stateMutex);
					if (m_proactor) {
						Log::GetInstance().AppendDebug(
							"Connection %1% closed by remote side.",
							m_instanceId);
					}
				}
			} else {
				StateLock lock(m_stateMutex);
				try {
					if (!m_proactor || m_closeAtLastMessageBlock) {
						// closing in progress or opening failed
						return;
					} else if (!m_isReadingAllowed) {
						// connection only for writing
						return;
					}
					ReadFromStream(messageBlock);
					return;
				} catch (const TunnelEx::LocalException &ex) {
					Log::GetInstance().AppendError(
						ConvertString<String>(ex.GetWhat()).GetCStr());
				}
			}
		}

		StateLock lock(m_stateMutex);
		CloseFromProactor(lock);

	}
	
	template<typename Result>
	void DoHandleWriteStream(const Result &result) {
		StateLock lock(m_stateMutex);
		try {
			const UniqueMessageBlockHolder messageBlock(result.message_block());
			BOOST_ASSERT(m_sendQueueSize > 0);
			--m_sendQueueSize;
			if (!m_proactor) {
				// closing in progress or opening failed
				return;
			}
			//! @todo: if is a TEX-549 workaround, remove after bag will be resolved.
			if (!m_closeAtLastMessageBlock) {
				m_signal->OnMessageBlockSent(messageBlock);
			}
			if (!m_closeAtLastMessageBlock || m_sendQueueSize > 0) {
				return;
			}
		} catch (const TunnelEx::LocalException &ex) {
			Log::GetInstance().AppendError(
				ConvertString<String>(ex.GetWhat()).GetCStr());
		}
		if (m_closeAtLastMessageBlock) {
			ScheduleDeletion();
		} else {
			CloseFromProactor(lock);
		}
	}

	virtual void handle_time_out(const ACE_Time_Value &, const void *act = 0) {

		const int idleTimeoutToken = m_idleTimeoutToken;
		
		if (!m_proactor) {
			BOOST_ASSERT(m_sendQueueSize == 0);
			delete this;
			return;
		}

		StateLock lock(m_stateMutex);
		if (act == 0) {
			BOOST_ASSERT(!m_proactor);
			BOOST_ASSERT(m_sendQueueSize == 0);
			delete this;
			return;
		} else {
			BOOST_ASSERT(act == reinterpret_cast<void *>(1));
			if (idleTimeoutToken == m_idleTimeoutToken) {
				if (!m_myInterface.OnIdleTimeout()) {
					Log::GetInstance().AppendDebug(
						"Closing connection %1% by idle timeout...",
						m_instanceId);
					CloseFromProactor(lock);
				} else {
					UpdateIdleTimer();
				}
			}
		}
		
	}

private:

	void ReadFromStream(UniqueMessageBlockHolder &messageBlock) {
		m_myInterface.ReadRemote(messageBlock);
		if (m_isReadingInitiated) {
			InitReadIfPossible();
		}
		//! @todo: try to throw exception here [2008/07/30 22:17]
	}

	//! Closing in proactor thread.
	/* This method only for optimization! For another threads and if you
	 * have any doubts - use ScheduleDeletion-method.
	 */
	void CloseFromProactor(StateLock &lock) {
		if (!m_proactor) {
			return;
		}
		if (m_idleTimeoutTimer != -1) {
			m_proactor->cancel_timer(m_idleTimeoutTimer);
			m_idleTimeoutTimer = -1;
		}
		m_proactor = 0;
		m_readStream.reset();
		m_writeStream.reset();
		Proxy *const proxy = this->proxy_.get();
		if (proxy) {
			proxy->reset();
		}
		SharedPtr<ConnectionSignal> signal = m_signal;
		m_signal.Reset();
		// after "OnConnectionClose" object can be already destructed, so release mutex here
		lock.release();
		signal->OnConnectionClose(m_instanceId);
	}

	void InitReadIfPossible() {
		
		if (m_closeAtLastMessageBlock) {
			return;
		} else if (m_sentMessageBlockQueueSize >= m_messageBlockQueueBufferSize) {
			if (m_isReadingActive) {
				const size_t bytes = m_sentMessageBlockQueueSize * m_dataBlockSize;
				Log::GetInstance().AppendDebug(
					"Data queue memory size is %1% bytes,"
						" data read from connection %2% will be suspended.",
					bytes,
					m_instanceId);
				m_isReadingActive = false;
			}
			return;
		} else if (!m_isReadingActive) {
			if (m_sentMessageBlockQueueSize >= m_messageBlockQueueBufferSize / 2) {
				return;
			}
			const size_t bytes = m_sentMessageBlockQueueSize * m_dataBlockSize;
			Log::GetInstance().AppendDebug(
				"Data queue memory size is %1% bytes,"
					" data read from connection %2% will be resumed.",
				bytes,
				m_instanceId);
			m_isReadingActive = true;
		}

		InitRead();

	}

	void InitRead() const throw(ConnectionException) {
		if (!m_readStream.get()) {
			return;
		}
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
			if (error.GetErrorNo() == ERROR_NETNAME_DELETED) {
				// see TEX-553
				Log::GetInstance().AppendDebug(message.str().c_str());
			} else {
				throw ConnectionException(message.str().c_str());
			}
		}
		messageBlock.Release();
	}

	void UpdateIdleTimer() {
		BOOST_ASSERT(m_proactor != 0);
		if (m_idleTimeoutTimer != -1) {
			m_proactor->cancel_timer(m_idleTimeoutTimer);
			m_idleTimeoutTimer = -1;
		}
		if (m_idleTimeoutInterval != ACE_Time_Value::zero && m_isSetupCompleted) {
			m_idleTimeoutTimer = m_proactor->schedule_timer(
				*this,
				reinterpret_cast<void *>(1),
				m_idleTimeoutInterval);
		}
		++m_idleTimeoutToken;
	}

private:
	
	Connection &m_myInterface;
	const Id m_instanceId;
	
	const RuleEndpoint &m_ruleEndpoint;
	const SharedPtr<const EndpointAddress> m_ruleEndpointAddress;
	
	SharedPtr<ConnectionSignal> m_signal;
	
	auto_ptr<ACE_Asynch_Operation> m_readStream;
	auto_ptr<ACE_Asynch_Operation> m_writeStream;
	function<int(ACE_Message_Block &, size_t)> m_readStreamFunc;
	function<int(ACE_Message_Block &, size_t)> m_writeStreamFunc;
	
	bool m_isReadingAllowed;
	
	StateMutex m_stateMutex;
	
	bool m_isReadingInitiated;
	bool m_isReadingActive;
	bool m_isSetupCompleted;
	bool m_isSetupCompletedWithSuccess;
	
	ACE_Proactor *m_proactor;

	const size_t m_dataBlockSize;
	const size_t m_messageBlockQueueBufferSize;
	size_t m_sentMessageBlockQueueSize;

	bool m_closeAtLastMessageBlock;
	size_t m_sendQueueSize;

	shared_ptr<TunnelBuffer> m_buffer;
	TunnelBuffer::Allocators m_allocators;

	bool m_forceClosingMode;

	int m_idleTimeoutToken;
	ACE_Time_Value m_idleTimeoutInterval;
	long m_idleTimeoutTimer;

	TUNNELEX_OBJECTS_DELETION_CHECK_DECLARATION(m_instancesNumber);

};

TUNNELEX_OBJECTS_DELETION_CHECK_DEFINITION(Connection::Implementation, m_instancesNumber);

//////////////////////////////////////////////////////////////////////////

Connection::Connection(
			const RuleEndpoint &ruleLocalEndpoint,
			SharedPtr<const EndpointAddress> &ruleEndpointAddress) {
	m_pimpl = new Implementation(*this, ruleLocalEndpoint, ruleEndpointAddress);
}

Connection::~Connection() {
	m_pimpl->ScheduleDeletion();
}

void Connection::Open(SharedPtr<ConnectionSignal> signal, Mode mode) {
	m_pimpl->Open(signal, mode);
}

DataTransferCommand Connection::WriteDirectly(MessageBlock &messageBlock) {
	return m_pimpl->SendToRemote(messageBlock);
}

void Connection::WriteDirectly(const char *data, size_t size) {
	BOOST_ASSERT(size > 0);
	if (!size) {
		return;
	}
	// not using internal allocator for memory, so buffer can be with any size
	UniqueMessageBlockHolder messageBlock(new ACE_Message_Block(size));
	if (messageBlock.Get().copy(data, size) == -1) {
		throw TunnelEx::InsufficientMemoryException(
			L"Insufficient message block memory");
	}
	const DataTransferCommand cmd = WriteDirectly(messageBlock);
	ACE_UNUSED_ARG(cmd);
	BOOST_ASSERT(cmd == DATA_TRANSFER_CMD_SEND_PACKET);
}

DataTransferCommand Connection::Write(MessageBlock &messageBlock) {
	return WriteDirectly(messageBlock);
}

DataTransferCommand Connection::SendToRemote(MessageBlock &messageBlock) {
	return Write(messageBlock);
}

void Connection::SendToRemote(const char *data, size_t size) {
	BOOST_ASSERT(size > 0);
	if (!size) {
		return;
	}
	// not using internal allocator for memory, so buffer can be with any size
	UniqueMessageBlockHolder messageBlock(new ACE_Message_Block(size));
	if (messageBlock.Get().copy(data, size) == -1) {
		throw TunnelEx::InsufficientMemoryException(
			L"Insufficient message block memory");
	}
	const DataTransferCommand cmd = SendToRemote(messageBlock);
	ACE_UNUSED_ARG(cmd);
	BOOST_ASSERT(cmd == DATA_TRANSFER_CMD_SEND_PACKET);
}

void Connection::SendToTunnel(MessageBlock &messageBlock) {
	m_pimpl->SendToTunnel(messageBlock);
}

void Connection::SendToTunnel(const char *data, size_t size) {
	BOOST_ASSERT(size > 0);
	if (!size) {
		return;
	}
	// not using internal allocator for memory, so buffer can be with any size
	UniqueMessageBlockHolder messageBlock(new ACE_Message_Block(size));
	if (messageBlock.Get().copy(data, size) == -1) {
		throw TunnelEx::InsufficientMemoryException(
			L"Insufficient message block memory");
	}
	SendToTunnel(messageBlock);
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
	SendToTunnel(messageBlock);
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

void Connection::SetForceClosingMode() {
	m_pimpl->SetForceClosingMode();
}

bool Connection::IsSetupCompleted() const {
	return m_pimpl->IsSetupCompleted();
}

bool Connection::IsSetupFailed() const {
	return m_pimpl->IsSetupFailed();
}

void Connection::ResetIdleTimeout(TimeSeconds seconds) {
	m_pimpl->ResetIdleTimeout(seconds);
}

bool Connection::OnIdleTimeout() throw() {
	BOOST_ASSERT(false);
	return true;
}

bool Connection::IsOneWay() const {
	return false;
}
