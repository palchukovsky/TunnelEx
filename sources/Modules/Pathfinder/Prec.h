/**************************************************************************
 *   Created: 2010/06/22 13:30
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__Prec_h__1006221330
#define INCLUDED_FILE__Prec_h__1006221330

#include "CompileConfig.h"
#include "LocalAssert.h"

#include <ace/OS_Errno.h>

#include "Licensing/Prec.h"

#include "Hash.hpp"
#include "Foreach.h"
#include "Format.hpp"
#include "StringUtil.hpp"
#include "ModulePath.hpp"
#include "UseUnused.hpp"

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
#include "CompileWarningsBoost.h"

#include <vector>
#include <string>
#include <sstream>
#include <fstream>

#endif

#include "LocalAssert.h"
