/**************************************************************************
 *   Created: 2008/07/22 6:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 * -------------------------------------------------------------------
 *       $Id: Instance.cpp 959 2010-06-22 11:36:04Z palchukovsky $
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
