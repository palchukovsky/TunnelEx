/**************************************************************************
 *   Created: 2008/05/22 21:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "EndpointAddress.hpp"
#include "Log.hpp"

using namespace TunnelEx;

EndpointAddress::EndpointAddress() {
	//...//
}

EndpointAddress::EndpointAddress(const EndpointAddress &) {
	//...//
}

EndpointAddress::~EndpointAddress() {
	//...//
}

EndpointAddress & EndpointAddress::operator =(const EndpointAddress &) {
	return *this;
}

void EndpointAddress::Swap(EndpointAddress &) throw() {
	//...//
}

void EndpointAddress::StatConnectionSetupCompleting() const  throw() {
	//...//
}

void EndpointAddress::StatConnectionSetupCanceling() const throw() {
	//...//
}

void EndpointAddress::StatConnectionSetupCanceling(
			const WString &reason)
		const
		throw() {
	assert(!reason.IsEmpty());
	Log::GetInstance().AppendError(
		ConvertString<String>(reason.GetCStr()).GetCStr());
	StatConnectionSetupCanceling();
}
