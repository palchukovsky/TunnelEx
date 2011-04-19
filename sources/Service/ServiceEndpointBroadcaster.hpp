/**************************************************************************
 *   Created: 2008/02/24 4:24
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__ServiceEnpoint_h__0802240424
#define INCLUDED_FILE__TUNNELEX__ServiceEnpoint_h__0802240424

class ServiceEndpointBroadcaster : private boost::noncopyable {

public:

	ServiceEndpointBroadcaster();
	~ServiceEndpointBroadcaster(); /* throw() */

	void Broadcast(const char* host, int port, bool isSecured);
	void CallbackAll();

protected:

	void HandleError(const char*, DWORD);

private:

	HKEY m_key;
	const wchar_t *const m_serviceSubKeyName;
	const wchar_t *const m_endpointSubKeyName;

};

#endif // INCLUDED_FILE__TUNNELEX__ServiceEnpoint_h__0802240424