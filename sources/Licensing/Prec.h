/**************************************************************************
 *   Created: 2010/07/08 18:49
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Prec_h__1007081849
#define INCLUDED_FILE__TUNNELEX__Prec_h__1007081849

#include "Constants.h"
#include "CompileConfig.h"
#include "LocalAssert.h"

#include "Crypto.hpp"
#include "Foreach.h"
#include "ModulePath.hpp"
#include "StringUtil.hpp"
#include "Xml.hpp"
#include "Uuid.hpp"

#include <windows.h>
#include <ShlObj.h>

#include <Iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")

#include <Wininet.h>
#pragma comment(lib, "Wininet.lib")

#include "CompileWarningsBoost.h"
#	include <boost/any.hpp>
#	include <boost/format.hpp>
#	include <boost/filesystem.hpp>
#	include <boost/lexical_cast.hpp>
#	include <boost/noncopyable.hpp>
#	include <boost/optional.hpp>
#	include <boost/shared_ptr.hpp>
#	include <boost/algorithm/string.hpp>
#	include <boost/date_time.hpp>
#include "CompileWarningsBoost.h"

#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <utility>

#endif
