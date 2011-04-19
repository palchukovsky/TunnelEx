/**************************************************************************
 *   Created: 2008/02/13 0:39
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Format_h__0802130039
#define INCLUDED_FILE__TUNNELEX__Format_h__0802130039

#include <boost/format.hpp>

namespace TunnelEx {

	template<class Ch>
	class BasicFormat : public boost::basic_format<Ch> {
	
	public:

		explicit BasicFormat(const Ch* str = NULL)
			: basic_format(str) {
			exceptions(boost::io::no_error_bits);
		}
		explicit BasicFormat(const string_type &s)
			: basic_format(s) {
			exceptions(boost::io::no_error_bits);
		}
		explicit BasicFormat(const Ch *str, const std::locale &loc)
			: basic_format(str, loc) {
			exceptions(boost::io::no_error_bits);
		}
		explicit BasicFormat(const string_type &s, const std::locale &loc)
			: basic_format(s, loc) {
			exceptions(boost::io::no_error_bits);
		}

	};

	typedef BasicFormat<char> Format;
	typedef BasicFormat<wchar_t> WFormat;

}

#endif // INCLUDED_FILE__TUNNELEX__Format_h__0802130039
