/**************************************************************************
 *   Created: 2009/09/16 12:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef TEX_BOOST_COMPILE_WARNINGS_DISABLED
#	define TEX_BOOST_COMPILE_WARNINGS_DISABLED
#	ifndef BOOST_USE_WINDOWS_H
#		define BOOST_USE_WINDOWS_H
#	endif // #ifndef BOOST_USE_WINDOWS_H
#	pragma warning(push, 3)
#	pragma warning(disable: 4180)
#	pragma warning(disable: 4702)
#	pragma warning(disable: 4267)
#	pragma warning(disable: 4244)
#else // #ifndef TEX_BOOST_COMPILE_WARNINGS_DISABLED
#	undef TEX_BOOST_COMPILE_WARNINGS_DISABLED
#	pragma warning(pop)
#endif  // #ifndef TEX_BOOST_COMPILE_WARNINGS_DISABLED
