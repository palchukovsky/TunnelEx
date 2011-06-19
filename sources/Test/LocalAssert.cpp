/**************************************************************************
 *   Created: 2009/04/20 12:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

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
		FAIL() << oss.str();
		
	}

}
