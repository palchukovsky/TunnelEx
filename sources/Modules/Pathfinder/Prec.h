/**************************************************************************
 *   Created: 2010/06/22 13:30
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Prec.h 1047 2010-11-02 17:32:51Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__Prec_h__1006221330
#define INCLUDED_FILE__Prec_h__1006221330

#include "CompileConfig.h"

#if defined(_DEBUG) || defined(TEST)
#	define TEX_PATHFINDER_TEST
#endif

#include <ace/OS_Errno.h>

#include "Licensing/Prec.h"
#include "Licensing/IpHelperWorkstationPropertiesQueryPolicy.hpp"

#include "Hash.hpp"
#include "Foreach.h"
#include "Format.hpp"
#include "StringUtil.hpp"
#include "ModulePath.hpp"

#include <Wininet.h>
#pragma comment(lib, "Wininet.lib")

#include "CompileWarningsAce.h"
#	include <ace/Init_ACE.h>
#	include <ace/Singleton.h>
#	include <ace/Thread_Mutex.h>
#	include <ace/Guard_T.h>
#include "CompileWarningsAce.h"

#include "CompileWarningsBoost.h"
#	include <boost/noncopyable.hpp>
#	include <boost/optional.hpp>
#	include <boost/algorithm/string.hpp>
#	include <boost/cast.hpp>
#	ifdef TEX_PATHFINDER_TEST
#		include <boost/bind.hpp>
#	endif
#include "CompileWarningsBoost.h"

#include <vector>
#include <string>
#include <sstream>
#include <fstream>

#endif
