/**************************************************************************
 *   Created: 2010/06/22 10:50
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Prec.h 1133 2011-02-22 23:18:56Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__Prec_h__1006221050
#define INCLUDED_FILE__Prec_h__1006221050

#include "CompileConfig.h"

#include <ace/OS_Errno.h>

// adapters info getting
#include <IPHlpApi.h>
#pragma comment(lib, "IPHLPAPI.lib")
#ifdef X509_EXTENSIONS
#	undef X509_EXTENSIONS
#endif

#include "Licensing/Prec.h"

#include "Format.hpp"
#include "Foreach.h"
#include "Crypto.hpp"
#include "ModulePath.hpp"
#include "StringUtil.hpp"
#include "Constants.h"

#include "CompileWarningsAce.h"
#	include <ace/Init_ACE.h>
#	include <ace/SOCK_Acceptor.h>
#	include <ace/SOCK_Connector.h>
#	include <ace/SOCK_Dgram.h>
#	include <ace/SOCK_CODgram.h>
#	include <ace/SOCK.h>
#	include <ace/SOCK_Stream.h>
#	include <ace/INET_Addr.h>
#	include <ace/ssl/SSL_SOCK_Acceptor.h>
#	include <ace/ssl/SSL_SOCK_Stream.h>
#	include <ace/Time_Value.h>
#	include <ace/OS_NS_netdb.h>
#	include <ace/OS_NS_sys_time.h>
#	include <ace/Ping_Socket.h>
#	include <ace/Guard_T.h>
#	include <ace/Thread_Mutex.h>
#	include <ace/Reactor.h>
#	include <ace/Thread_Manager.h>
#	include <ace/Truncate.h>
#include "CompileWarningsAce.h"

#include "CompileWarningsBoost.h"
#	include <boost/noncopyable.hpp>
#	include <boost/shared_ptr.hpp>
#	include <boost/ref.hpp>
#	include <boost/bind.hpp>
#	include <boost/function.hpp>
#	include <boost/cast.hpp>
#	include <boost/regex.hpp>
#	include <boost/algorithm/string.hpp>
#	include <boost/lexical_cast.hpp>
#	include <boost/static_assert.hpp>
#	include <boost/cast.hpp>
#	include <boost/type_traits/integral_constant.hpp>
#	include <boost/type_traits/is_same.hpp>
#	include <boost/mpl/assert.hpp>
#include "CompileWarningsBoost.h"

#include <numeric>
#include <sstream>
#include <fstream>
#include <string>
#include <list>
#include <map>
#include <set>
#include <memory>
#include <vector>

#endif
