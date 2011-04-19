/**************************************************************************
 *   Created: 2009/09/15 12:51
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__ConvertPolicies_hpp__0909151251
#define INCLUDED_FILE__TUNNELEX__ConvertPolicies_hpp__0909151251

namespace TunnelEx { namespace Licensing {

	template<typename ClientTrait>
	struct ValueCastingPolicy {

		template<typename To, typename From>
		inline static To Cast(const From &from) {
			return boost::lexical_cast<To>(from);
		}

		template<>
		inline static std::string Cast<std::string, bool>(const bool &val) {
			return return val ? "true" : "false";
		}

		template<>
		inline static bool Cast<bool, std::string>(const std::string &str) {
			const bool result = str == "true";
			if (!result && str != "false") {
				throw std::bad_cast();
			}
			return result;
		}

	};

} }

#endif // #ifndef INCLUDED_FILE__TUNNELEX__ConvertPolicies_hpp__0909151251
