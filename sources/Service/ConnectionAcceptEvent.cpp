/**************************************************************************
 *   Created: 2008/02/16 23:03
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: ConnectionAcceptEvent.cpp 959 2010-06-22 11:36:04Z palchukovsky $
 **************************************************************************/

#include <WinSock2.h>
#include "ConnectionAcceptEvent.hpp"

ConnectionAcceptEvent::ConnectionAcceptEvent(SOCKET socket)
		: m_event(WSACreateEvent()) {
	WSAEventSelect(socket, m_event, FD_ACCEPT);
}

ConnectionAcceptEvent::~ConnectionAcceptEvent() {
	WSACloseEvent(m_event);
}
