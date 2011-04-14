/**************************************************************************
 *   Created: 2009/12/28 12:48
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: StringUtil.hpp 1079 2010-12-01 05:19:27Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__StringUtil_hpp__091228
#define INCLUDED_FILE__TUNNELEX__StringUtil_hpp__091228

#include "Foreach.h"
#include "CompileWarningsBoost.h"
#	include <boost/assert.hpp>
#	include <boost/regex.hpp>
#include "CompileWarningsBoost.h"
#include <string>
#include <vector>
#include <sstream>
#include <locale>

namespace TunnelEx { namespace Helpers {

	struct StringUtil {

	public:

		static void AsciiToBin(
					const char *const source,
					const size_t len,
					std::vector<unsigned char> &result) {
			std::vector<unsigned char> bin;
			bin.reserve(len / 2);
			bool isFirst = true;
			char c;
			char b = 0;
			for (size_t i = 0; i < len; ++i) {
				c = source[i];
				if ((c > 47) && (c < 58)) {
					c -= 48;
				} else  if ((c > 64) && (c < 71)) {
					c -= 65 - 10;
				} else if ((c > 96) && (c < 103)) {
					c -= 97 - 10;
				} else {
					continue;
				}
				if (isFirst) {
					b = c << 4;
				} else {
					bin.push_back(static_cast<unsigned char>(b + c));
				}
				isFirst = !isFirst;
			}
			result.swap(bin);
		}

		static std::string BinToAscii(const unsigned char *source, size_t len) {
			std::string result;
			result.resize(len * 2);
			const char *const hexits = "0123456789ABCDEF";
			for (size_t i = 0; i < len; ++i) {
				result[i * 2] = hexits[source[i] >> 4];
				result[(i * 2) + 1] = hexits[source[i] & 0x0F];
			}
			return result;
		}

		static std::string BinToAscii(const std::vector<unsigned char> &binary) {
			return BinToAscii(&binary[0], binary.size());
		}

		template<typename Char>
		static void EncodeUrl(
					const std::basic_string<typename Char> &source,
					std::basic_string<typename Char> &destination) {
			const Char *const hexChars
				= reinterpret_cast<Char *>("0123456789ABCDEF");
			foreach (const char ch, source) {
				if (ch == ' ') {
					destination.push_back('+');
				} else if (
						(ch < '0' && ch != '-' && ch != '.')
						|| (ch < 'A' && ch > '9')
						|| (ch > 'Z' && ch < 'a' && ch != '_')
						|| (ch > 'z')) {
					destination.push_back('%');
					destination.push_back(hexChars[ch >> 4]);
					destination.push_back(hexChars[ch & 15]);
				} else {
					destination.push_back(ch);
				}
			}
		}
		
		template<>
		static void StringUtil::EncodeUrl<wchar_t>(
					const std::basic_string<wchar_t> &source,
					 std::basic_string<wchar_t> &destination) {
			const wchar_t *const hexChars = L"0123456789ABCDEF";
			foreach (const wchar_t ch, source) {
				if (ch == L' ') {
					destination.push_back(L'+');
				} else if (
						(ch < L'0' && ch != L'-' && ch != L'.')
						|| (ch < L'A' && ch > L'9')
						|| (ch > L'Z' && ch < L'a' && ch != L'_')
						|| (ch > L'z')) {
					destination.push_back(L'%');
					destination.push_back(hexChars[ch >> 4]);
					destination.push_back(hexChars[ch & 15]);
				} else {
					destination.push_back(ch);
				}
			}
		}

		template<typename Char>
		static std::basic_string<typename Char> EncodeUrl(
					const std::basic_string<typename Char> &source) {
			std::basic_string<typename Char> result;
			EncodeUrl(source, result);
			return result;
		}

		static void DecodeUrl(
					const std::wstring::const_iterator begin,
					const std::wstring::const_iterator end,
					std::wostringstream &destination) {
			for (std::wstring::const_iterator i = begin; i != end; ++i) {
				const wchar_t ch = *i;
				if (ch == L'+') {
					destination << L' ';
				} else if (ch == '%') {
					std::wstring::const_iterator nextI = i;
					std::wstring::const_iterator nextNextI = ++nextI;
					if (
							nextI != end
							&& ++nextNextI != i
							&& isxdigit(*nextI)
							&& isxdigit(*nextNextI)) {
						wchar_t ch;
						{
							wchar_t c = *nextI;
							if (isupper(c)) {
#								pragma warning(push)
#								pragma warning(disable: 4244)
								c = tolower(c);
#								pragma warning(pop)
							}
							ch = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;
						}
						{
							wchar_t c = *nextNextI;
							if (isupper(c)) {
#								pragma warning(push)
#								pragma warning(disable: 4244)
								c = tolower(c);
#								pragma warning(pop)
							}
							ch += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;
						}
						destination << ch;
						i = nextNextI;
					} else {
						BOOST_ASSERT(false);
						destination << ch;
					}
				} else {
					destination << ch;
				}
			}
		}

		static void DecodeUrl(
					const std::wstring &source,
					std::wostringstream &destination) {
			DecodeUrl(source.begin(), source.end(), destination);
		}

		static std::wstring DecodeUrl(
					const std::wstring::const_iterator begin,
					const std::wstring::const_iterator end) {
			std::wostringstream destination;
			DecodeUrl(begin, end, destination);
			return destination.str();
		}

		static std::wstring DecodeUrl(const std::wstring &source) {
			std::wostringstream destination;
			DecodeUrl(source, destination);
			return destination.str();
		}

		static bool IsUuid(const std::wstring &str) {
			const boost::wregex exp(
				L"[\\dA-F]{8,8}-[\\dA-F]{4,4}-[\\dA-F]{4,4}-[\\dA-F]{4,4}-[\\dA-F]{12,12}",
				boost::regex_constants::normal | boost::regex_constants::icase);
			return boost::regex_match(str, exp);
		}

	};

} }

#endif // #ifndef INCLUDED_FILE__TUNNELEX__StringUtil_hpp__091228
