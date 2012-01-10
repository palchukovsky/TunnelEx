/**************************************************************************
 *   Created: 2010/01/09 2:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__TunnelBuffer_hpp__1001090208
#define INCLUDED_FILE__TUNNELEX__TunnelBuffer_hpp__1001090208

#include "Locking.hpp"
#include "MessagesAllocator.hpp"
#include "MessageBlockHolder.hpp"

namespace TunnelEx {

	class MessagesAllocator : private boost::noncopyable {

	public:

		const static size_t DefautDataBlockSize;
		const static size_t DefautConnectionBufferSize;

	private:

		class Mutex : private boost::noncopyable {
		public:
			int acquire() {
				m_impl.Acquire();
				return 0;
			}
			int release() {
				m_impl.Release();
				return 0;
			}
		private:
			SpinMutex m_impl;
		};

		typedef ACE_Cached_Allocator<ACE_Message_Block, Mutex> MessageBlocksAllocator;
		typedef ACE_Cached_Allocator<
				UniqueMessageBlockHolder::Satellite,
				Mutex>
			MessageBlockSatellitesAllocator;
		typedef ACE_Cached_Allocator<ACE_Data_Block, Mutex> DataBlocksAllocator;
		typedef ACE_Dynamic_Cached_Allocator<Mutex> DataBlocksBufferAllocator;

	public:
		
		explicit MessagesAllocator(
				size_t messageBlocksCount,
				size_t dataBlocksCount,
				size_t dataBlockSize);

	public:

		MessageBlocksAllocator & GetMessageBlocksAllocator() {
			return m_messageBlocksAllocator;
		}

		MessageBlockSatellitesAllocator & GetMessageBlockSatellitesAllocator() {
			return m_messageBlockSatellitesAllocator;
		}

		DataBlocksAllocator & GetDataBlocksAllocator() {
			return m_dataBlocksAllocator;
		}
		
		DataBlocksBufferAllocator & GetDataBlocksBufferAllocator() {
			return m_dataBlocksBufferAllocator;
		}

		size_t GetDataBlockSize() const {
			return m_dataBlockSize;
		}

	private:

		const size_t m_dataBlockSize;

		MessageBlocksAllocator m_messageBlocksAllocator;
		MessageBlockSatellitesAllocator m_messageBlockSatellitesAllocator;
		DataBlocksAllocator m_dataBlocksAllocator;
		DataBlocksBufferAllocator m_dataBlocksBufferAllocator;

	};

}

#endif // INCLUDED_FILE__TUNNELEX__TunnelBuffer_hpp__1001090208
