/**************************************************************************
 *   Created: 2011/11/01 23:16
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "LicenseState.hpp"

using namespace TunnelEx;
using namespace TunnelEx::Singletons;

////////////////////////////////////////////////////////////////////////////////

class LicenseStatePolicy::Implementation : private boost::noncopyable {

public:

	typedef ACE_RW_Mutex NotificationMutex;
	typedef ACE_Read_Guard<NotificationMutex> NotificationReadLock;
	typedef ACE_Write_Guard<NotificationMutex> NotificationWriteLock;

	typedef std::list<boost::shared_ptr<TunnelEx::Licensing::Error>> Errors;

public:

	NotificationMutex m_notificationMutex;
	Errors m_errors;

};

////////////////////////////////////////////////////////////////////////////////

LicenseStatePolicy::LicenseStatePolicy()
		: m_pimpl(new Implementation) {
	//...//
}

LicenseStatePolicy::~LicenseStatePolicy() throw() {
	delete m_pimpl;
}

void LicenseStatePolicy::RegisterError(
			TunnelEx::Licensing::Client client,
			const std::string &license,
			const std::string &time,
			const std::string &point,
			const std::string &errorDesc) {
	boost::shared_ptr<TunnelEx::Licensing::Error> error(new TunnelEx::Licensing::Error);
	error->client = client;
	error->license = license;
	error->time = time;
	error->point = point;
	error->error = errorDesc;
	Implementation::NotificationWriteLock lock(m_pimpl->m_notificationMutex);
	if (m_pimpl->m_errors.size() > 8) {
		Implementation::Errors errors(m_pimpl->m_errors);
		errors.pop_front();
		errors.push_back(error);
		errors.swap(m_pimpl->m_errors);
	} else {
		m_pimpl->m_errors.push_back(error);
	}
}

bool LicenseStatePolicy::GetError(size_t index, TunnelEx::Licensing::Error &result) {
	Implementation::NotificationReadLock lock(m_pimpl->m_notificationMutex);
	if (m_pimpl->m_errors.size() <= index) {
		return false;
	}
	Implementation::Errors::const_iterator i = m_pimpl->m_errors.begin();
	std::advance(i, index);
	result = **i;
	return true;
}

////////////////////////////////////////////////////////////////////////////////

#if TEMPLATES_REQUIRE_SOURCE != 0
#	include "Singleton.cpp"
	namespace {
		//! Only for template instantiation.
		void MakeServerTemplateInstantiation() {
			LicenseState::GetInstance();
		}
	}
#endif // TEMPLATES_REQUIRE_SOURCE
