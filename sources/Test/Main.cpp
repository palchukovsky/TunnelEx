/**************************************************************************
 *   Created: 2007/12/15 15:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.com
 **************************************************************************/

#include "Prec.h"

#include "Fixtures.hpp"

namespace {
	struct CloseStopper {
		~CloseStopper() {
			getchar();
		}
	};
	static CloseStopper closeStopper;
}

namespace Test {
	BOOST_GLOBAL_FIXTURE(GlobalFixture);
}
