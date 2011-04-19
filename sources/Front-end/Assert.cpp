/**************************************************************************
 *   Created: 2009/04/20 13:09
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.h"

#include "Core/String.hpp"
#if !defined(__cplusplus) || !defined(__WXDEBUG__)
#	include <assert.h>
#endif // !defined(__cplusplus) || !defined(__WXDEBUG__)

using namespace TunnelEx;

namespace boost
{
	void assertion_failed(
			char const *expr,
			char const *function,
			char const *file,
			long line) {
#		if defined(__cplusplus) && defined(__WXDEBUG__)
			wxOnAssert(
				ConvertString<WString>(file).GetCStr(),
				line,
				function,
				ConvertString<WString>(expr).GetCStr(),
				wxT("Assertion failed"));
#		else
			wxLogError(
				wxT("Assertion failed: \"%s\" in %s (file %s:%d)."),
				ConvertString<WString>(expr).GetCStr(),
				ConvertString<WString>(function).GetCStr(),
				ConvertString<WString>(file).GetCStr(),
				line);
#		endif // if defined(__cplusplus) && defined(__WXDEBUG__)			
	}

} // namespace boost
