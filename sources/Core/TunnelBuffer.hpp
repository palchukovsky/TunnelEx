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

namespace TunnelEx {

	class TunnelBuffer : private boost::noncopyable {

	public:

		struct Allocators {
			Allocators()
					: messageBlock(0),
					dataBlock(0),
					dataBlockBuffer(0) {
				//...//
			}
			ACE_Allocator *messageBlock;
			ACE_Allocator *dataBlock;
			ACE_Allocator *dataBlockBuffer;
		};

	private:

		typedef std::set<ACE_Allocator *> AllAllocators;

	public:
		
		TunnelBuffer();
		~TunnelBuffer() throw();

	public:

		Allocators CreateBuffer(
				size_t messageBlocksCount,
				size_t dataBlocksCount,
				size_t dataBlockSize);

		void DeleteBuffer(const Allocators &) throw();

	private:

		AllAllocators m_allocators;

	};

}

#endif // INCLUDED_FILE__TUNNELEX__TunnelBuffer_hpp__1001090208
