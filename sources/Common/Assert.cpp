/**************************************************************************
 *   Created: 2009/04/20 12:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Assert.cpp 1046 2010-11-02 12:07:07Z palchukovsky $
 **************************************************************************/

#include "CompileConfig.h"

#ifdef TUNNELEX_CORE
#	include "Core/Log.hpp"
#else
#	include <TunnelEx/Log.hpp>
#endif
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
