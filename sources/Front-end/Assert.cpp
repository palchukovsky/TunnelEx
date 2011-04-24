/**************************************************************************
 *   Created: 2009/04/20 13:09
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "CompileConfig.h"
#include <wx/log.h>
#include "Core/String.hpp"
#if !defined(__cplusplus) || !defined(__WXDEBUG__)
#	include <cassert>
#endif

namespace boost
{
	void assertion_failed(
			char const *expr,
			char const *function,
			char const *file,
			long line) {
		using namespace TunnelEx;
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
#		endif
	}

}
