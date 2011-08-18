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

	public:

		explicit PipeConnection(HANDLE handle);
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

	public:

		HANDLE GetReadEvent();
		HANDLE GetWriteEvent();
		
		void HandleEvent(HANDLE);

	protected:

		void SetAsConnected() {
			assert(m_isActive == 0);
			if (BOOST_INTERLOCKED_EXCHANGE(&m_isActive, 1) == 0) {
				StartRead();
			}
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

		void StartRead();

		void HandleRead();
		void HandleWrite();

		DWORD ReadOverlappedWriteResult();
		DWORD ReadOverlappedReadResult();

		void UpdateBufferState();
		void UpdateBufferState(size_t addSize);

		void Close(const boost::mutex::scoped_lock &);

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

		volatile long m_isActive;

		Buffer m_receiveBuffer;

		OVERLAPPED m_readOverlaped;
		OVERLAPPED m_writeOverlaped;

		bool m_isReadingStarted;

		std::list<boost::shared_ptr<Buffer>> m_sentBuffers;

	};

	////////////////////////////////////////////////////////////////////////////////

	class PipeClientConnection : public PipeConnection {

	public:

		typedef PipeClientConnection Self;
		typedef PipeConnection Base;

	public:

		explicit PipeClientConnection(const std::string &path);

	};

	////////////////////////////////////////////////////////////////////////////////

	class PipeServerConnection : public PipeConnection {

	public:

		typedef PipeClientConnection Self;
		typedef PipeConnection Base;

	public:

		explicit PipeServerConnection(const std::string &path);

	};

	////////////////////////////////////////////////////////////////////////////////

}

#endif
