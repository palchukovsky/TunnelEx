/**************************************************************************
 *   Created: 2009/09/30 12:53
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2009 Eugene V. Palchukovsky
 **************************************************************************/

#ifndef foreach
#	include "CompileWarningsBoost.h"
#		include <boost/foreach.hpp>
#	include "CompileWarningsBoost.h"
#	define foreach BOOST_FOREACH
#endif // #ifndef foreach
