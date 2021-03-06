/**************************************************************************
 *   Created: 2010/06/22 10:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__Prec_h__1006221019
#define INCLUDED_FILE__Prec_h__1006221019

#include "CompileConfig.h"
#include "Constants.h"
#include "LocalAssert.hpp"

#include <ace/OS_Errno.h>

#include "Foreach.h"
#include "MultipleStream.hpp"
#include "ModulePath.hpp"
#include "Hash.hpp"
#include "Format.hpp"
#include "ObjectsDeletionCheck.h"
#include "Uuid.hpp"
#include "Crypto.hpp"
#include "UseUnused.hpp"

#include "Licensing/Prec.h"

#include <Windows.h>

#ifdef CreateService
#	undef CreateService
#endif

#include "CompileWarningsAce.h"
#	include <ace/Init_ACE.h>
#	include <ace/Barrier.h>
#	include <ace/Guard_T.h>
#	include <ace/Thread_Mutex.h>
#	include <ace/Null_Mutex.h>
#	include <ace/RW_Mutex.h>
#	include <ace/Recursive_Thread_Mutex.h>
#	include <ace/Condition_T.h>
#	include <ace/Thread_Manager.h>
#	include <ace/Message_Block.h>
#	include <ace/Malloc_T.h>
#	include <ace/Event_Handler.h>
#	include <ace/Asynch_IO.h>
#	include <ace/Proactor.h>
#	include <ace/WIN32_Proactor.h>
#	include <ace/Reactor.h>
#	include <ace/OS_NS_unistd.h>
#	include <ace/Atomic_Op.h>
#	include <ace/INET_Addr.h>
#include "CompileWarningsAce.h"

#include "CompileWarningsBoost.h"
#	include <boost/shared_ptr.hpp>
#	include <boost/noncopyable.hpp>
#	include <boost/filesystem.hpp>
#	include <boost/bind.hpp>
#	include <boost/function.hpp>
#	include <boost/optional.hpp>
#	include <boost/signals2.hpp>
#	include <boost/cast.hpp>
#	include <boost/regex.hpp>
#	include <boost/multi_index_container.hpp>
#	include <boost/multi_index/ordered_index.hpp>
#	include <boost/multi_index/member.hpp>
#	include <boost/multi_index/hashed_index.hpp>
#	include <boost/multi_index/mem_fun.hpp>
#	include <boost/system/system_error.hpp>
#	include <boost/detail/interlocked.hpp>
#	include <boost/functional/hash.hpp>
#	include <boost/filesystem.hpp>
#	include <boost/ptr_container/ptr_vector.hpp>
#	include <boost/date_time.hpp>
#	include <boost/unordered_map.hpp>
#	include <boost/static_assert.hpp>
#	include <boost/accumulators/accumulators.hpp>
#	include <boost/accumulators/statistics/stats.hpp>
#	include <boost/accumulators/statistics/mean.hpp>
#	include <boost/accumulators/statistics/min.hpp>
#	include <boost/accumulators/statistics/max.hpp>
#include "CompileWarningsBoost.h"

#include <mbstring.h>
#include <string.h>
#include <malloc.h>

#include <sstream>
#include <fstream>
#include <memory>
#include <vector>
#include <set>
#include <map>
#include <string>
#include <numeric>
#include <exception>

#endif

#include "LocalAssert.hpp"
