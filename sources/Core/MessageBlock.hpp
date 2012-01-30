/**************************************************************************
 *   Created: 2008/07/31 16:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__MessageBlock_h__080731161000
#define INCLUDED_FILE__MessageBlock_h__080731161000

#include "Instance.hpp"
#include "Api.h"

namespace TunnelEx {

	class TUNNELEX_CORE_API MessageBlock {

	public:

		MessageBlock() throw();
		virtual ~MessageBlock() throw();

	private:

		MessageBlock(const MessageBlock &);
		const MessageBlock & operator =(const MessageBlock &);

	public:

		virtual const char * GetData() const throw() = 0;
		virtual size_t GetUnreadedDataSize() const throw() = 0;

		virtual char * GetWritableSpace(size_t size) = 0;
		virtual void TakeWritableSpace(size_t size) = 0;

		virtual void Read() = 0;
		virtual void Read(size_t size) = 0;

		virtual void SetData(const char *data, size_t dataLength) = 0;
		
		virtual void MarkAsAddedToQueue() throw() = 0;
		virtual bool IsAddedToQueue() const throw() = 0;

		virtual bool IsTunnelMessage() const throw() = 0;

		virtual size_t GetBlockSize() const throw() = 0;

	};

}

#endif // #ifndef INCLUDED_FILE__MessageBlock_h__080731161000
