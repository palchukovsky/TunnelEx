/**************************************************************************
 *   Created: 2010/12/10 19:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "AceSockDgramCloser.h"

void TunnelEx::Mods::Inet::AceSockDgramCloser(ACE_SOCK_Dgram *const obj) {
	if (obj != 0) {
		obj->close();
	}
	delete obj;
}
