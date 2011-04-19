/**************************************************************************
 *   Created: 2009/04/20 12:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "CompileConfig.h"

#include "Core/Log.hpp"
#include <assert.h> // .h to support old libraries w/o <cassert> - effect is the same
#include <sstream>

namespace boost {

	void assertion_failed(
			char const *expr,
			char const *function,
			char const *file,
			long line) {
		std::ostringstream iss;
		iss
			<< "Assertion failed: \"" << expr << "\""
			<< " in function \"" << function << "\""
			<< " (file " << file << ":" << line << ")";
		TunnelEx::Log::GetInstance().AppendError(iss.str());
		assert(false);
	}

}
