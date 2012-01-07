/**************************************************************************
 *   Created: 2008/02/13 0:39
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Format_h__0802130039
#define INCLUDED_FILE__TUNNELEX__Format_h__0802130039

#include "StringFwd.hpp"
#include <boost/format.hpp>

namespace TunnelEx {

	template<typename Ch>
	class BasicFormat : public boost::basic_format<Ch> {

	public:
		
		typedef boost::basic_format<Ch> Base;
		typedef typename Base::CharT ValueType;
		typedef typename Base::string_type::traits_type TraitsType;
		typedef std::basic_ostream<ValueType, TraitsType> OutStream;

	public:

		explicit BasicFormat(const Ch* str = NULL)
				: Base(str) {
			exceptions(boost::io::no_error_bits);
		}
		explicit BasicFormat(const string_type &s)
				: Base(s) {
			exceptions(boost::io::no_error_bits);
		}
		explicit BasicFormat(const Ch *str, const std::locale &loc)
				: Base(str, loc) {
			exceptions(boost::io::no_error_bits);
		}
		explicit BasicFormat(const string_type &s, const std::locale &loc)
				: Base(s, loc) {
			exceptions(boost::io::no_error_bits);
		}

	};

	typedef BasicFormat<char> Format;
	typedef BasicFormat<wchar_t> WFormat;

}

namespace std {

	::TunnelEx::Format::OutStream & operator <<(
			::TunnelEx::Format::OutStream &,
			const std::wstring &);
	::TunnelEx::Format::OutStream & operator <<(
			::TunnelEx::Format::OutStream &,
			const ::TunnelEx::String &);
	::TunnelEx::Format::OutStream & operator <<(
			::TunnelEx::Format::OutStream &,
			const ::TunnelEx::WString &);

	::TunnelEx::WFormat::OutStream & operator <<(
			::TunnelEx::WFormat::OutStream &,
			const std::string &);
	::TunnelEx::WFormat::OutStream & operator <<(
			::TunnelEx::WFormat::OutStream &,
			const ::TunnelEx::String &);
	::TunnelEx::WFormat::OutStream & operator <<(
			::TunnelEx::WFormat::OutStream &,
			const ::TunnelEx::WString &);

}

#endif // INCLUDED_FILE__TUNNELEX__Format_h__0802130039
