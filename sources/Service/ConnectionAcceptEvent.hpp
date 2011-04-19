/**************************************************************************
 *   Created: 2008/02/16 23:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__ConnectionAcceptHandle_h__0802162301
#define INCLUDED_FILE__TUNNELEX__ConnectionAcceptHandle_h__0802162301

class ConnectionAcceptEvent {

public:
	
	ConnectionAcceptEvent(SOCKET socket);
	~ConnectionAcceptEvent() throw();

private:
	
	ConnectionAcceptEvent(const ConnectionAcceptEvent &);
	const ConnectionAcceptEvent & operator =(const ConnectionAcceptEvent &);

public:

	HANDLE GetEvent() {
		return m_event;
	}

private:

	HANDLE m_event;

};

#endif
