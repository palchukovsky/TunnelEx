/**************************************************************************
 *   Created: 2008/12/15 2:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Constants_h__0812150237
#define INCLUDED_FILE__TUNNELEX__Constants_h__0812150237

#define _STR(a) #a
#define _XSTR(a) _STR(a)
#define _WSTR(a) L#a
#define _XWSTR(a) _WSTR(a)

#include "Version/Version.h"
#include "CompileConfig.h"

#define TUNNELEX_VERSION	\
	_XSTR(TUNNELEX_VERSION_MAJOR_HIGH) \
	"." _XSTR(TUNNELEX_VERSION_MAJOR_LOW) \
	"." _XSTR(TUNNELEX_VERSION_MINOR_HIGH)
#define TUNNELEX_VERSION_W	\
	_XWSTR(TUNNELEX_VERSION_MAJOR_HIGH) \
	L"." _XWSTR(TUNNELEX_VERSION_MAJOR_LOW) \
	L"." _XWSTR(TUNNELEX_VERSION_MINOR_HIGH)

#define TUNNELEX_VERSION_FULL \
	_XSTR(TUNNELEX_VERSION_MAJOR_HIGH) \
	"." _XSTR(TUNNELEX_VERSION_MAJOR_LOW) \
	"." _XSTR(TUNNELEX_VERSION_MINOR_HIGH) \
	"." _XSTR(TUNNELEX_VERSION_MINOR_LOW)
#define TUNNELEX_VERSION_FULL_W	\
	_XWSTR(TUNNELEX_VERSION_MAJOR_HIGH) \
	L"." _XWSTR(TUNNELEX_VERSION_MAJOR_LOW) \
	L"." _XWSTR(TUNNELEX_VERSION_MINOR_HIGH) \
	L"." _XWSTR(TUNNELEX_VERSION_MINOR_LOW)

#define TUNNELEX_VERSION_GEN_UNIQUE_ID_STR _XSTR(TUNNELEX_VERSION_GEN_UNIQUE_ID)
#define TUNNELEX_VERSION_GEN_UNIQUE_ID_STR_W _XWSTR(TUNNELEX_VERSION_GEN_UNIQUE_ID)

#if defined(_DEBUG)
#	define TUNNELEX_BUILD_IDENTITY \
		"DEBUG " \
		TUNNELEX_VERSION_FULL \
		"." TUNNELEX_VERSION_GEN_UNIQUE_ID_STR
#	define TUNNELEX_BUILD_IDENTITY_W \
		L"DEBUG " \
		TUNNELEX_VERSION_FULL_W \
		L"." TUNNELEX_VERSION_GEN_UNIQUE_ID_STR_W
#elif defined(TEST)
#	define TUNNELEX_BUILD_IDENTITY \
		"TEST " \
		TUNNELEX_VERSION_FULL \
		"." TUNNELEX_VERSION_GEN_UNIQUE_ID_STR
#	define TUNNELEX_BUILD_IDENTITY_W \
		L"TEST " \
		TUNNELEX_VERSION_FULL_W \
		L"." TUNNELEX_VERSION_GEN_UNIQUE_ID_STR_W
#elif defined(TEST)
#	define TUNNELEX_BUILD_IDENTITY \
		"RELEASE " \
		TUNNELEX_VERSION_FULL \
		"." TUNNELEX_VERSION_GEN_UNIQUE_ID_STR
#	define TUNNELEX_BUILD_IDENTITY_W \
		L"RELEASE " \
		TUNNELEX_VERSION_FULL_W \
		L"." TUNNELEX_VERSION_GEN_UNIQUE_ID_STR_W
#endif

#ifdef DEV_VER
#	define TUNNELEX_BUILD_IDENTITY_ADD		" [" TUNNELEX_BUILD_IDENTITY "]"
#	define TUNNELEX_BUILD_IDENTITY_ADD_W	L" [" TUNNELEX_BUILD_IDENTITY_W L"]"
#else
#	define TUNNELEX_BUILD_IDENTITY_ADD		""
#	define TUNNELEX_BUILD_IDENTITY_ADD_W	L""
#endif

#ifdef _DEBUG
#	define TUNNELEX_FILE_MODIFICATOR		"_dbg"
#else // #ifdef _DEBUG
#	ifdef TEST
#		define TUNNELEX_FILE_MODIFICATOR	"_test"
#	else // #ifdef TEST
#		define TUNNELEX_FILE_MODIFICATOR
#	endif
#endif // #ifdef _DEBUG

#define TUNNELEX_CORE_DLL_FILE_NAME					TUNNELEX_SERVICE_FILE_NAME TUNNELEX_FILE_MODIFICATOR ".dll"
#define TUNNELEX_SERVICE_EXE_FILE_NAME				TUNNELEX_SERVICE_FILE_NAME TUNNELEX_FILE_MODIFICATOR ".exe"
#define TUNNELEX_CONTROL_CENTER_EXE_FILE_NAME		TUNNELEX_CONTROL_CENTER_FILE_NAME TUNNELEX_FILE_MODIFICATOR ".exe"
#define TUNNELEX_LEGACY_DLL_FILE_NAME				TUNNELEX_LEGACY_FILE_NAME TUNNELEX_FILE_MODIFICATOR ".dll"
#define TUNNELEX_MODULE_PIPE_DLL_FILE_NAME			TUNNELEX_MODULE_PIPE_FILE_NAME TUNNELEX_FILE_MODIFICATOR ".dll"
#define TUNNELEX_MODULE_INET_DLL_FILE_NAME			TUNNELEX_MODULE_INET_FILE_NAME TUNNELEX_FILE_MODIFICATOR ".dll"
#define TUNNELEX_MODULE_INET_LIB_FILE_NAME			TUNNELEX_MODULE_INET_FILE_NAME TUNNELEX_FILE_MODIFICATOR ".lib"
#define TUNNELEX_MODULE_SERIAL_DLL_FILE_NAME		TUNNELEX_MODULE_SERIAL_FILE_NAME TUNNELEX_FILE_MODIFICATOR ".dll"
#define TUNNELEX_MODULE_SERIAL_LIB_FILE_NAME		TUNNELEX_MODULE_SERIAL_FILE_NAME TUNNELEX_FILE_MODIFICATOR ".lib"
#define TUNNELEX_MODULE_PATHFINDER_DLL_FILE_NAME	TUNNELEX_MODULE_PATHFINDER_FILE_NAME TUNNELEX_FILE_MODIFICATOR ".dll"
#define TUNNELEX_MODULE_PATHFINDER_LIB_FILE_NAME	TUNNELEX_MODULE_PATHFINDER_FILE_NAME TUNNELEX_FILE_MODIFICATOR ".lib"
#define TUNNELEX_MODULE_UPNP_DLL_FILE_NAME			TUNNELEX_MODULE_UPNP_FILE_NAME TUNNELEX_FILE_MODIFICATOR ".dll"
#define TUNNELEX_MODULE_UPNP_LIB_FILE_NAME			TUNNELEX_MODULE_UPNP_FILE_NAME TUNNELEX_FILE_MODIFICATOR ".lib"

#define TUNNELEX_MODULE_INET_NAME "Inet Module"
#define TUNNELEX_MODULE_INET_NAME_FULL TUNNELEX_NAME " " TUNNELEX_MODULE_INET_NAME

#define TUNNELEX_FAKE_HTTP_CLIENT \
	"Mozilla/5.0 (Windows NT 6.1; rv:2.0)" \
	" Gecko/20100101 Firefox/4.0"
#define TUNNELEX_FAKE_HTTP_CLIENT_W \
	L"Mozilla/5.0 (Windows NT 6.1; rv:2.0)" \
	L" Gecko/20100101 Firefox/4.0"

#endif // INCLUDED_FILE__TUNNELEX__Constants_h__0812150237
