/**************************************************************************
 *   Created: 2008/01/18 1:26
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 * -------------------------------------------------------------------
 *       $Id: Listener.cpp 959 2010-06-22 11:36:04Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "Listener.hpp"

using namespace TunnelEx;

//////////////////////////////////////////////////////////////////////////

Listener::Listener() {
	/*...*/
}

Listener::~Listener() {
	/*...*/
}

//////////////////////////////////////////////////////////////////////////

PreListener::PreListener() {
	/*...*/
}
PreListener::~PreListener() throw() {
	/*...*/
}

//////////////////////////////////////////////////////////////////////////

PostListener::PostListener() {
	/*...*/
}

PostListener::~PostListener() throw() {
	/*...*/
}

//////////////////////////////////////////////////////////////////////////