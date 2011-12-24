/**************************************************************************
 *   Created: 2011/12/24/ 11:26
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "MessageBlockHolder.hpp"

using namespace TunnelEx;

TUNNELEX_OBJECTS_DELETION_CHECK_DEFINITION(
	UniqueMessageBlockHolder::MbImp,
	m_instancesNumber);
