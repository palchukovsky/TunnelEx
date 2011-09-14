/**************************************************************************
 *   Created: 2011/07/12 19:29
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__PipeConnection_hpp__1107121929
#define INCLUDED_FILE__TUNNELEX__PipeConnection_hpp__1107121929

#include "ClientServer.hpp"

namespace TestUtil {

	////////////////////////////////////////////////////////////////////////////////

	class PipeConnection : private boost::noncopyable {

	public:

		typedef PipeConnection Self;

	private:

		struct SentBuffer {
			
			size_t sentBytes;
			boost::shared_ptr<Buffer> buffer;

			SentBuffer()
					: sentBytes(0) {
				//...//
			}
			SentBuffer(boost::shared_ptr<Buffer> buffer)
					: sentBytes(0),
					buffer(buffer) {
				//...//
			}

		};
		typedef std::list<SentBuffer> SentBuffers;

	public:

		explicit PipeConnection(
				HANDLE handle,
				const boost::posix_time::time_duration &waitTime);
		~PipeConnection();

	public:

		static size_t GetBufferSize();

	public:

		void Close();

	public:

		void Send(std::auto_ptr<Buffer>);

		Buffer::size_type GetReceivedSize() const;

		void GetReceived(Buffer::size_type maxSize, Buffer &destination) const;

		void ClearReceived(size_t bytesCount = 0);

		bool IsActive() const;

		bool WaitDataReceiveEvent(
					const boost::system_time &waitUntil,
					Buffer::size_type minSize)
				const;

		const boost::posix_time::time_duration & GetWaitTime() const {
			return m_waitTime;
		}

	public:

		HANDLE GetReadEvent();
		HANDLE GetWriteEvent();
		HANDLE GetCloseEvent();
		
		void HandleEvent(HANDLE);

	protected:

		void SetAsConnected() {
			SetAsConnected(boost::mutex::scoped_lock(m_stateMutex));
		}

		void SetHandle(HANDLE handle) throw() {
			m_handle = handle;
		}
		HANDLE GetHandle() {
			assert(m_handle != INVALID_HANDLE_VALUE);
			return m_handle;
		}

		OVERLAPPED & GetReadOverlaped() {
			return m_readOverlaped;
		}
		OVERLAPPED & GetWriteOverlaped() {
			return m_writeOverlaped;
		}

	private:

		void SetAsConnected(const boost::mutex::scoped_lock &lock) {
			assert(m_isActive == 0);
			assert(m_handle != INVALID_HANDLE_VALUE);
			if (BOOST_INTERLOCKED_EXCHANGE(&m_isActive, 1) == 0) {
				StartRead(lock);
			} else {
				assert(false);
			}
		}

		void StartRead(const boost::mutex::scoped_lock &);
		bool StartReadAndRead(const boost::mutex::scoped_lock &);

		void HandleRead();
		void ReadReceived(DWORD bytesNumber, const boost::mutex::scoped_lock &);
		void HandleWrite();
		virtual void HandleClose() {
			//...//
		}

		DWORD ReadOverlappedWriteResult(const boost::mutex::scoped_lock &);
		DWORD ReadOverlappedReadResult(const boost::mutex::scoped_lock &);

		void UpdateBufferState();
		void UpdateBufferState(size_t addSize);

		void Close(const boost::mutex::scoped_lock &);
		virtual void CloseHandles();

	private:

		HANDLE m_handle;

		Buffer m_dataBuffer;
		Buffer::const_iterator m_dataBufferStart;
#		ifdef DEV_VER
			const char *m_dataBufferPch;
			size_t m_dataBufferFullSize;
			const char *m_dataBufferStartPch;
#		endif
		volatile long m_dataBufferSize;

		mutable boost::mutex m_stateMutex;
		mutable boost::condition_variable m_dataReceivedCondition;
		mutable boost::condition_variable m_dataSentCondition;

		volatile long m_isActive;

		Buffer m_receiveBuffer;

		OVERLAPPED m_readOverlaped;
		OVERLAPPED m_writeOverlaped;

		SentBuffers m_sentBuffers;

		const boost::posix_time::time_duration m_waitTime;

	};

	////////////////////////////////////////////////////////////////////////////////

	class PipeClientConnection : public PipeConnection {

	public:

		typedef PipeClientConnection Self;
		typedef PipeConnection Base;

	public:

		explicit PipeClientConnection(
				const std::string &path,
				const boost::posix_time::time_duration &waitTime);

	};

	////////////////////////////////////////////////////////////////////////////////

	class PipeServerConnection : public PipeConnection {

	public:

		typedef PipeClientConnection Self;
		typedef PipeConnection Base;

	public:

		explicit PipeServerConnection(
				const std::string &path,
				const boost::posix_time::time_duration &waitTime);
		virtual ~PipeServerConnection();

	private:

		virtual void HandleClose();
		virtual void CloseHandles();

	private:

		HANDLE m_closeEvent;

	};

	////////////////////////////////////////////////////////////////////////////////

}

#endif
