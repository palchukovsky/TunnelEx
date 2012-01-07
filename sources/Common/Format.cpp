/**************************************************************************
 *   Created: 2012/1/7/ 3:11
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "Core/String.hpp"

using namespace TunnelEx;

Format::OutStream & std::operator <<(Format::OutStream &out, const std::wstring &str) {
	String buffer;
	ConvertString(str.c_str(), buffer);
	return operator <<(out, buffer);
}

Format::OutStream & std::operator <<(Format::OutStream &out, const String &str) {
	out << str.GetCStr();
	return out;
}

Format::OutStream & std::operator <<(Format::OutStream &out, const WString &str) {
	String buffer;
	ConvertString(str, buffer);
	return operator <<(out, buffer);
}

WFormat::OutStream & std::operator <<(WFormat::OutStream &out, const std::string &str) {
	WString buffer;
	ConvertString(str.c_str(), buffer);
	return operator <<(out, buffer);
}

WFormat::OutStream & std::operator <<(WFormat::OutStream &out, const String &str) {
	WString buffer;
	ConvertString(str, buffer);
	return operator <<(out, buffer);
}

WFormat::OutStream & std::operator <<(WFormat::OutStream &out, const WString &str) {
	out << str.GetCStr();
	return out;
}
