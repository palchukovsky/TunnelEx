/**************************************************************************
 *   Created: 2010/10/31 21:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__StringFwd_h__1010312112
#define INCLUDED_FILE__TUNNELEX__StringFwd_h__1010312112

#include "Core/Api.h"

namespace TunnelEx {

	//////////////////////////////////////////////////////////////////////////

	template<class Element>
	class TUNNELEX_CORE_API BasicString;

	//////////////////////////////////////////////////////////////////////////

	//! ANSI string.
	typedef ::TunnelEx::BasicString<char> String;

	typedef unsigned char Utf8Char;
	//! UTF-8 string.
	typedef ::TunnelEx::BasicString<Utf8Char> UString;

	//! UTF-16 BE string.
	typedef ::TunnelEx::BasicString<wchar_t> WString;

}

#endif
