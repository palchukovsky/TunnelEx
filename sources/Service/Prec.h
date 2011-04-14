/**************************************************************************
 *   Created: 2010/06/22 14:39
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Prec.h 1107 2010-12-20 12:24:07Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__Prec_h__1006221439
#define INCLUDED_FILE__Prec_h__1006221439

#include "CompileConfig.h"
#include "Constants.h"

#define BOOST_FILESYSTEM_VERSION 2

#include "Licensing/Prec.h"

#include "Crypto.hpp"
#include "ModulePath.hpp"
#include "StringUtil.hpp"
#include "Format.hpp"
#include "Foreach.h"

#include "SoapServiceInterface/soapStub.h"
#include "SoapServiceInterface/TunnelExService.nsmap"

#include <stdsoap2.h>

#include <Windows.h>
#include <ShellAPI.h>
#include <Aclapi.h>

// adapters info getting
#include <IPHlpApi.h>
#pragma comment(lib, "IPHLPAPI.lib")

#include "CompileWarningsBoost.h"
#	include <boost/noncopyable.hpp>
#	include <boost/shared_ptr.hpp>
#	include <boost/regex.hpp>
#	include <boost/thread.hpp>
#	include <boost/filesystem.hpp>
#	include <boost/regex.hpp>
#	include <boost/function.hpp>
#	include <boost/bind.hpp>
#	include <boost/cast.hpp>
#	include <boost/cstdint.hpp>
#include "CompileWarningsBoost.h"

#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <vector>
#include <set>
#include <map>

#endif
