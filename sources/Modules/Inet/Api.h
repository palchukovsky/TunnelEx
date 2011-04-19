/**************************************************************************
 *   Created: 2009/12/23 23:21
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Api_h__0912232321
#define INCLUDED_FILE__TUNNELEX__Api_h__0912232321

#ifdef TUNNELEX_MOD_INET
#	define TUNNELEX_MOD_INET_API __declspec(dllexport)
#elif __cplusplus_cli // #ifdef TUNNELEX_MOD_INET
#	define TUNNELEX_MOD_INET_API
#else
#	pragma comment(lib, TUNNELEX_MODULE_INET_LIB_FILE_NAME)
#	define TUNNELEX_MOD_INET_API __declspec(dllimport)
#endif // #ifdef TUNNELEX_MOD_INET

#endif // INCLUDED_FILE__TUNNELEX__Api_h__0912232321
