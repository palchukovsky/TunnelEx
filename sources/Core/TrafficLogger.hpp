/**************************************************************************
 *   Created: 2008/01/17 14:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.com
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 * -------------------------------------------------------------------
 *       $Id: TrafficLogger.hpp 959 2010-06-22 11:36:04Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__TrafficLogger_h__0801171415
#define INCLUDED_FILE__TUNNELEX__TrafficLogger_h__0801171415

#include "Listener.hpp"
#include "Rule.hpp"
#include "Server.hpp"

namespace TunnelEx {

	class Connection;

	//! @todo: Make it post-listener [2008/08/19 16:38]
	class TrafficLogger : public PreListener {

	public:

		TrafficLogger(	Server::Ref,
						const ::TunnelEx::RuleEndpoint::ListenerInfo &,
						const ::TunnelEx::TunnelRule &,
						const ::TunnelEx::Connection &,
						const ::TunnelEx::Connection &);
		virtual ~TrafficLogger();

	private:

		//! @todo: reimplement with async
		std::ofstream m_file;

	public:

		virtual DataTransferCommand OnNewMessageBlock(MessageBlock &);

	};

}

#endif // INCLUDED_FILE__TUNNELEX__TrafficLogger_h__0801171415