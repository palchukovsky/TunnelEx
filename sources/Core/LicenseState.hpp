/**************************************************************************
 *   Created: 2011/11/01 23:11
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__LicenseState_hpp__1111012311
#define INCLUDED_FILE__TUNNELEX__LicenseState_hpp__1111012311

#include "Licensing/Types.hpp"
#include "Singleton.hpp"
#include "Api.h"

namespace TunnelEx { namespace Singletons {

	class TUNNELEX_CORE_API LicenseStatePolicy {

		template<typename T, template<class> class L, template<class> class Th>
		friend class ::TunnelEx::Singletons::Holder;

	private:

		LicenseStatePolicy();
		~LicenseStatePolicy() throw();
		LicenseStatePolicy(const LicenseStatePolicy &);
		const LicenseStatePolicy & operator =(const LicenseStatePolicy &);

	public:

		void RegisterError(
					TunnelEx::Licensing::Client client,
					const std::string &license,
					const std::string &time,
					const std::string &point,
					const std::string &error);

		size_t GetErrorCount() const;

		bool GetError(size_t index, TunnelEx::Licensing::Error &result);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

} }

namespace TunnelEx {

	typedef ::TunnelEx::Singletons::Holder<
			::TunnelEx::Singletons::LicenseStatePolicy,
			::TunnelEx::Singletons::DefaultLifetime>
		LicenseState;

}

#endif // INCLUDED_FILE__TUNNELEX__LicenseState_hpp__1111012311
