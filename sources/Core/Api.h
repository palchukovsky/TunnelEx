/**************************************************************************
 *   Created: 2007/03/10 22:03
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__Api_h__0703102203
#define INCLUDED_FILE__Api_h__0703102203

#ifdef TUNNELEX_CORE
#     define TUNNELEX_CORE_API __declspec(dllexport)
#elif __cplusplus_cli
#     define TUNNELEX_CORE_API
#else
#     define TUNNELEX_CORE_API __declspec(dllimport)
#endif // TUNNELEX_CORE

#if defined(_MSC_VER) && _MSC_VER <= 1400 // The Visual C++ 2005 compiler version is 1400.
#     pragma warning(disable: 4251)
#endif // defined(_MSC_VER) && _MSC_VER <= 1400

#endif // INCLUDED_FILE__Api_h__0703102203
