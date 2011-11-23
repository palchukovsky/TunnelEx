/**************************************************************************
 *   Created: 2010/06/22 12:21
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__Prec_h__1006221224
#define INCLUDED_FILE__Prec_h__1006221224

#include "CompileConfig.h"
#include "Constants.h"
#include "LocalAssert.hpp"

#include <ace/OS_Errno.h>

#include "Format.hpp"
#include "StringUtil.hpp"
#include "Foreach.h"
#include "Xml.hpp"
#include "Crypto.hpp"
#include "Uuid.hpp"
#include "UseUnused.hpp"

#include <Windows.h>

#ifdef CreateService
#	undef CreateService
#endif

#include <miniupnp/miniupnpc.h>
#include <miniupnp/upnpcommands.h>
#include <miniupnp/upnperrors.h>

#include "CompileWarningsAce.h"
#	include <ace/RW_Mutex.h>
#	include <ace/Guard_T.h>
#	include <ace/Time_Value.h>
#	include <ace/OS_NS_sys_time.h>
#	include <ace/INET_Addr.h>
#	include <ace/SOCK.h>
#	include <ace/SOCK_Acceptor.h> // only for Inet module including
#	include <ace/SOCK_Dgram.h> // only for Inet module including
#	include <ace/SOCK_CODgram.h> // only for Inet module including
#	include <ace/ssl/SSL_SOCK_Acceptor.h> // only for Inet module including
#	include <ace/ssl/SSL_SOCK_Stream.h> // only for Inet module including
#	include <ace/Thread_Mutex.h> // only for Inet module including
#include "CompileWarningsAce.h"

#include "CompileWarningsBoost.h"
#	include <boost/lexical_cast.hpp>
#	include <boost/noncopyable.hpp>
#	include <boost/function.hpp>
#	include <boost/bind.hpp>
#	include <boost/shared_ptr.hpp>
#	include <boost/algorithm/string.hpp>
#	include <boost/cast.hpp>
#	include <boost/regex.hpp>
#include "CompileWarningsBoost.h"

#include <string>
#include <memory>
#include <sstream>
#include <list>
#include <utility>
#include <vector>

#endif

#include "LocalAssert.hpp"
