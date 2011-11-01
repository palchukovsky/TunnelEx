/**************************************************************************
 *   Created: 2011/11/01 23:07
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__ServerNotificationPolicy_hpp__1111012307
#define INCLUDED_FILE__TUNNELEX__ServerNotificationPolicy_hpp__1111012307

#include "Core/LicenseState.hpp"

namespace TunnelEx { namespace Licensing {

	////////////////////////////////////////////////////////////////////////////////

	template<typename ClientTrait>
	struct NotificationPolicy {

		static void RegisterError(
				TunnelEx::Licensing::Client client,
				const std::string &license,
				const std::string &time,
				const std::string &point,
				const std::string &error,
				const boost::any &) {
			TunnelEx::LicenseState::GetInstance().RegisterError(
				client,
				license,
				time,
				point,
				error);
		}

		static bool GetError(size_t index, TunnelEx::Licensing::Error &result) {
			return TunnelEx::LicenseState::GetInstance().GetError(index, result);
		}

	};


} }

#endif // INCLUDED_FILE__TUNNELEX__ServerNotificationPolicy_hpp__1111012307
