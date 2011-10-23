/**************************************************************************
 *   Created: 2009/04/20 12:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "Core/Log.hpp"

namespace boost {

	void assertion_failed(
			char const *expr,
			char const *function,
			char const *file,
			long line) {

		std::ostringstream oss;
		oss
			<< "Assertion failed: \"" << expr << "\""
			<< " in function \"" << function << "\""
			<< " (file " << file << ":" << line << ")";
		TunnelEx::Log::GetInstance().AppendFatalError(oss.str());

#		ifdef _DEBUG
			_wassert(L"Execution stopped", _CRT_WIDE(__FILE__), __LINE__);
#		elif defined(_TEST)
			DebugBreak();
#		else
#			error "Failed to find assert-break method."
#		endif
		
	}

}
