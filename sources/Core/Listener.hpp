/**************************************************************************
 *   Created: 2008/01/17 13:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Listener.hpp 951 2010-06-06 19:04:52Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Listener_h__0801171332
#define INCLUDED_FILE__TUNNELEX__Listener_h__0801171332

#include "Instance.hpp"
#include "DataTransferCommand.hpp"
#include "Api.h"

namespace TunnelEx {

	class MessageBlock;

	//////////////////////////////////////////////////////////////////////////

	class TUNNELEX_CORE_API Listener : public ::TunnelEx::Instance {

	public:

		Listener();
		virtual ~Listener() throw();
		
	public:
	
		//! Handles new incoming data packet.
		/**	@param	messageBlock	the buffer with data;
		  *	@return	command for further actions;
		  */
		virtual ::TunnelEx::DataTransferCommand OnNewMessageBlock(
			::TunnelEx::MessageBlock &messageBlock) = 0;

	};

	//////////////////////////////////////////////////////////////////////////

	class TUNNELEX_CORE_API PreListener : public ::TunnelEx::Listener {

	public:

		PreListener();
		virtual ~PreListener() throw();

	};

	//////////////////////////////////////////////////////////////////////////

	class TUNNELEX_CORE_API PostListener : public ::TunnelEx::Listener {

	public:

		PostListener();
		virtual ~PostListener() throw();

	};

	//////////////////////////////////////////////////////////////////////////

}

#endif // INCLUDED_FILE__TUNNELEX__Listener_h__0801171332
