/**************************************************************************
 *   Created: 2010/03/14 17:51
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Api_h__1003141751
#define INCLUDED_FILE__TUNNELEX__Api_h__1003141751

#ifdef TUNNELEX_MOD_PATHFINDER
#	define TUNNELEX_MOD_PATHFINDER_API __declspec(dllexport)
#elif __cplusplus_cli // #ifdef TUNNELEX_MOD_PATHFINDER
#	define TUNNELEX_MOD_PATHFINDER_API
#else
#	pragma comment(lib, TUNNELEX_MODULE_PATHFINDER_LIB_FILE_NAME)
#	define TUNNELEX_MOD_PATHFINDER_API __declspec(dllimport)
#endif // #ifdef TUNNELEX_MOD_PATHFINDER

#endif // INCLUDED_FILE__TUNNELEX__Api_h__1003141751
