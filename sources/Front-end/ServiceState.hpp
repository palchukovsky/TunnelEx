/**************************************************************************
 *   Created: 2008/01/06 4:06
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.com
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__ServiceState_h__0801060406
#define INCLUDED_FILE__TUNNELEX__ServiceState_h__0801060406

enum ServiceState {
	TEX_SERVICE_STATE_UNKNOWN,
	TEX_SERVICE_STATE_CHANGED,
	TEX_SERVICE_STATE_STARTED,
	TEX_SERVICE_STATE_STOPPED,
	TEX_SERVICE_STATE_ERROR,
	TEX_SERVICE_STATE_WARNING,
	TEX_SERVICE_STATE_CONNECTING
};

#endif // INCLUDED_FILE__TUNNELEX__ServiceState_h__0801060406
