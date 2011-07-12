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

	class PipeConnection : private boost::noncopyable {

	public:

		typedef PipeConnection Self;

	public:

		explicit PipeConnection(HANDLE handle);
		~PipeConnection();

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

	private:

		void ReadThreadMain();

		void UpdateBufferState();
		void UpdateBufferState(size_t addSize);

	private:

		HANDLE m_handle;

		Buffer m_dataBuffer;
		Buffer::const_iterator m_dataBufferStart;
#		ifdef DEV_VER
			const char *m_dataBufferPch;
			size_t m_dataBufferFullSize;
			const char *m_dataBufferStartPch;
#		endif
		Buffer::size_type m_dataBufferSize;

		volatile long m_isActive;

		mutable boost::mutex m_mutex;
		mutable boost::condition_variable m_dataReceivedCondition;

		Buffer m_receiveBuffer;

		std::auto_ptr<boost::thread> m_readThread;

	};

}

#endif
