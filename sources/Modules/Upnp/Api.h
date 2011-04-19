/**************************************************************************
 *   Created: 2010/05/12 20:03
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Api_h__1005122003
#define INCLUDED_FILE__TUNNELEX__Api_h__1005122003

#ifdef TUNNELEX_MOD_UPNP
#	define TUNNELEX_MOD_UPNP_API __declspec(dllexport)
#elif __cplusplus_cli
#	define TUNNELEX_MOD_UPNP_API
#else
#	pragma comment(lib, TUNNELEX_MODULE_UPNP_LIB_FILE_NAME)
#	define TUNNELEX_MOD_UPNP_API __declspec(dllimport)
#endif

#endif // INCLUDED_FILE__TUNNELEX__Api_h__1005122003
