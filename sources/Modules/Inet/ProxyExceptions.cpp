/**************************************************************************
 *   Created: 2010/03/28 16:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "ProxyExceptions.hpp"

using namespace TunnelEx;
using namespace TunnelEx::Mods::Inet;

ProxyWorkingException::ProxyWorkingException(const wchar_t *what) throw()
		: ConnectionOpeningException(what) {
	//...//
}

ProxyWorkingException::ProxyWorkingException(const ProxyWorkingException &rhs) throw()
		: ConnectionOpeningException(rhs) {
	//...//
}

ProxyWorkingException::~ProxyWorkingException() throw() {
	//...//
}

const ProxyWorkingException & ProxyWorkingException::operator =(
			const ProxyWorkingException &rhs)
		throw() {
	ConnectionOpeningException::operator =(rhs);
	return *this;
}

UniquePtr<LocalException> ProxyWorkingException::Clone() const {
	return UniquePtr<LocalException>(new ProxyWorkingException(*this));
}
