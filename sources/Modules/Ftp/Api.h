/**************************************************************************
 *   Created: 2012/2/29/ 23:07
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#pragma once

#ifdef TUNNELEX_MOD_FTP
#	define TUNNELEX_MOD_FTP_API __declspec(dllexport)
#elif __cplusplus_cli
#	define TUNNELEX_MOD_FTP_API
#else
#	pragma comment(lib, TUNNELEX_MODULE_FTP_LIB_FILE_NAME)
#	define TUNNELEX_MOD_FTP_API __declspec(dllimport)
#endif // #ifdef TUNNELEX_MOD_INET
