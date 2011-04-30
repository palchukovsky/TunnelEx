/**************************************************************************
 *   Created: 2011/04/19 21:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Assert_h__1104192147
#define INCLUDED_FILE__TUNNELEX__Assert_h__1104192147

#include <cassert>
#ifdef assert
#	undef assert
#endif

#include "CompileWarningsBoost.h"
#	include <boost/assert.hpp>
#include "CompileWarningsBoost.h"

#define assert(expr) BOOST_ASSERT(expr)

#endif // INCLUDED_FILE__TUNNELEX__Assert_h__1104192147
