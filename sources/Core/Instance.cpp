/**************************************************************************
 *   Created: 2008/07/22 6:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.h"

#include "Instance.hpp"

using namespace TunnelEx;

Instance::Instance() {
	/*...*/
}

Instance::~Instance() {
	/*...*/
}

Instance::Id Instance::GetInstanceId() const {
	return reinterpret_cast<Id>(this);
}
