/**************************************************************************
 *   Created: 2010/01/02 3:23
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Api_h__1001020323
#define INCLUDED_FILE__TUNNELEX__Api_h__1001020323

#ifdef TUNNELEX_MOD_SERIAL
#	define TUNNELEX_MOD_SERIAL_API __declspec(dllexport)
#elif __cplusplus_cli // #ifdef TUNNELEX_MOD_SERIAL
#	define TUNNELEX_MOD_SERIAL_API
#else
#	pragma comment(lib, TUNNELEX_MODULE_SERIAL_LIB_FILE_NAME)
#	define TUNNELEX_MOD_SERIAL_API __declspec(dllimport)
#endif // #ifdef TUNNELEX_MOD_SERIAL

#endif // INCLUDED_FILE__TUNNELEX__Api_h__1001020323
