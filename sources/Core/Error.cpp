/**************************************************************************
 *   Created: 2008/10/23 10:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "Error.hpp"

using namespace TunnelEx;

Error::Error(int errorNo) throw()
		: m_errorNo(errorNo) {
	//...//
}

Error::~Error() throw() {
	//...//
}

bool Error::IsError() const {
	return GetErrorNo() != NOERROR;
}

bool Error::CheckError() const {
#	ifdef BOOST_WINDOWS
		LPVOID buffer;
		const auto formatResult = ::FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			m_errorNo,
			0,
			(LPWSTR)&buffer,
			0,
			NULL);
		boost::shared_ptr<VOID> bufferPtr(buffer, &::LocalFree);
		return formatResult != 0;
#	else
		// not implemented yet
		BOOST_STATIC_ASSERT(false);
#	endif
}

namespace {

	template<typename String>
	const typename String::ValueType * GetUnknownErrorTest() {
		return "Unknown error";
	}
	template<>
	const WString::ValueType * GetUnknownErrorTest<WString>() {
		return L"Unknown error";
	}
	
	template<typename Char>
	bool IsLineEnd(Char ch) {
		return ch == '\n' || ch == '\r';
	}
	template<>
	bool IsLineEnd<WString::ValueType>(WString::ValueType ch) {
		return ch == L'\n' || ch == L'\r';
	}

	template<typename String, typename SysGetter>
	String GetStringFromSys(int errorNo, SysGetter &sysGetter) {
		LPVOID buffer;
		auto size = sysGetter(
			FORMAT_MESSAGE_ALLOCATE_BUFFER
				| FORMAT_MESSAGE_FROM_SYSTEM
				| FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			errorNo,
			0,
			(String::ValueType *)&buffer,
			0,
			NULL);
		boost::shared_ptr<VOID> bufferPtr(buffer, &::LocalFree);
		String::ValueType *const pch = static_cast<String::ValueType *>(buffer);
		for ( ; size > 0 && IsLineEnd(pch[size - 1]) ; --size);
		if (size > 0) {
			pch[size - 1] = 0;
			return String(pch);
		} else {
			return String(GetUnknownErrorTest<String>());
		}
	}

}

WString Error::GetStringW() const {
#	ifdef BOOST_WINDOWS
		return GetStringFromSys<WString>(m_errorNo, ::FormatMessageW);
#	else // BOOST_WINDOWS
		WString result;
		namespace fs::boost::system;
		ConvertString(fs::system_error(m_errorNo, fs::get_system_category()).what(), result);
		return result;
#	endif // BOOST_WINDOWS
}

String Error::GetStringA() const {
#	ifdef BOOST_WINDOWS
		return GetStringFromSys<String>(m_errorNo, ::FormatMessageA);
#	else // BOOST_WINDOWS
		return fs::system_error(m_errorNo, fs::get_system_category()).what();
#	endif // BOOST_WINDOWS
}

int Error::GetErrorNo() const {
	return m_errorNo;
}
