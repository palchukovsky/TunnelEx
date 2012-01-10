/**************************************************************************
 *   Created: 2010/01/09 2:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "MessagesAllocator.hpp"

using namespace TunnelEx;

//////////////////////////////////////////////////////////////////////////

// IPv4 links must be able to forward packets of size up to 68 bytes.
// Systems may use Path MTU Discovery[4] to find the actual path MTU.
// This should not be mistaken with the packet size every host must be able
// to handle, which is 576.[5] (http://en.wikipedia.org/wiki/Maximum_transmission_unit)
const size_t MessagesAllocator::DefautDataBlockSize = 576;
const size_t MessagesAllocator::DefautConnectionBufferSize = 150 * 1024;

//////////////////////////////////////////////////////////////////////////

MessagesAllocator::MessagesAllocator(
			size_t messageBlocksCount,
			size_t dataBlocksCount,
			size_t dataBlockSize)
		: m_dataBlockSize(dataBlockSize),
		m_messageBlocksAllocator(messageBlocksCount),
		m_messageBlockSatellitesAllocator(dataBlocksCount),
		m_dataBlocksAllocator(dataBlocksCount),
		m_dataBlocksBufferAllocator(dataBlocksCount, m_dataBlockSize) {
	//...//
}

//////////////////////////////////////////////////////////////////////////
