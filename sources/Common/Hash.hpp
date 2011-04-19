/**************************************************************************
 *   Created: 2008/10/26 3:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Hash_hpp__0810260328
#define INCLUDED_FILE__TUNNELEX__Hash_hpp__0810260328

#include <functional>

namespace TunnelEx {

	namespace Helpers {

		template <class T>
		struct StringHasher : std::unary_function<T, size_t> {
			size_t operator ()(const T &val) const {
				return val.GetHash();
			}
		};

	}

}

#endif // INCLUDED_FILE__TUNNELEX__Hash_hpp__0810260328
