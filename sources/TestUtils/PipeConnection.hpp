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
		void Read();

		Buffer::size_type GetReceivedSize() const;

		void GetReceived(Buffer::size_type maxSize, Buffer &destination) const;

		void ClearReceived(size_t bytesCount = 0);

		bool IsActive() const;

		bool WaitDataReceiveEvent(
					const boost::system_time &waitUntil,
					Buffer::size_type minSize)
				const;

	public:

		HANDLE GetEvent();

	protected:

		void SetHandle(HANDLE handle) throw() {
			m_handle = handle;
		}

		OVERLAPPED & GetOverlaped();

	private:

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

		Buffer m_receiveBuffer;

		OVERLAPPED m_overlaped;

	};

	////////////////////////////////////////////////////////////////////////////////

	class PipeClientConnection : public PipeConnection {

	public:

		typedef PipeClientConnection Self;
		typedef PipeConnection Base;

	public:

		explicit PipeClientConnection(const std::string &path);
		~PipeClientConnection();

	private:

		const std::string m_path;
		const DWORD m_waitTimeout;

	};

	////////////////////////////////////////////////////////////////////////////////

	class PipeServerConnection : public PipeConnection {

	public:

		typedef PipeClientConnection Self;
		typedef PipeConnection Base;

	public:

		explicit PipeServerConnection(const std::string &path);
		~PipeServerConnection();

	};

}

#endif
