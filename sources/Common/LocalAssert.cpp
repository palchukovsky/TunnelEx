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

#		if defined(_DEBUG)
		{
			oss << std::endl << std::endl << "Stop program execution?";
			const int userAns = MessageBoxA(
				0,
				oss.str().c_str(),
				"Assertion failed",
				MB_YESNO | MB_ICONSTOP);
			if (userAns == IDYES) {
				_wassert(L"Execution stopped", _CRT_WIDE(__FILE__), __LINE__);
			}
		}
#		else
		{
			MessageBoxA(
				0,
				oss.str().c_str(),
				"Assertion failed",
				MB_OK | MB_ICONSTOP);
		}
#		endif
		
	}

}
