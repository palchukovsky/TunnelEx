/**************************************************************************
 *   Created: 2007/02/11 6:51
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__CommonDefinition_h__0702110651
#define INCLUDED_FILE__CommonDefinition_h__0702110651

#ifdef _WINDOWS
#	define WINVER 0x0501
#	define _WIN32_WINNT WINVER
#	define WIN32_LEAN_AND_MEAN
#	define NOMINMAX
#endif // _WINDOWS

#if defined(_MSC_VER)

#	if _MSC_VER <= 1600 // The Visual C++ 2010 compiler version is 1600.
#		if !defined(__cplusplus_cli) 
			//! Compiler's template mechanism must see source code.
#			define TEMPLATES_REQUIRE_SOURCE 1
#		endif
#	endif

//! @todo: atexit check in VS 1011
#	if _MSC_VER <= 1600
		/*	\todo:	Please, check how work atexit on your compiler.
			Example:
				void Bar() {}
				void Foo() {std::atexit(Bar);}
				int main() {std::atexit(Bar);}
			If it will work wrong - comment the ATEXIT_FIXED-define
			and vice versa. */
		//! For PhoenixSingleton template in Loki
#		define ATEXIT_FIXED 1
#	endif

	// 'identifier' : decorated name length exceeded, name was truncated
#	pragma warning(disable: 4503)

	//! @todo: remove 
#	pragma warning(disable: 4996)

	//! @todo: remove after boost 1.40.0 [2009/10/25 12:48]
#	pragma warning(disable: 4702)

#endif // defined(_MSC_VER)

#if !defined(__cplusplus_cli) && defined(_MSC_VER) && _MSC_VER <= 1400 // The Visual C++ 2005 compiler version is 1400.
	//!  Visual C++ accepts exception specification but does not implement.
#	pragma warning(disable : 4290)
#endif // !defined(__cplusplus_cli) && defined(_MSC_VER) && _MSC_VER <= 1400

#if !defined(__cplusplus_cli) && defined(_MSC_VER) && _MSC_VER == 1400 // The Visual C++ 2005 compiler version is 1400.
#	define _CRT_SECURE_NO_WARNINGS
#	define _SCL_SECURE_NO_WARNINGS
#endif // !defined(__cplusplus_cli) && defined(_MSC_VER) && _MSC_VER == 1400

#define NULL 0

#if defined(_DEBUG) || defined(_TEST)
#	define DEV_VER
#endif

#endif
