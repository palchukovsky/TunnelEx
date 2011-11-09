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
		::FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			m_errorNo,
			0,
			(LPWSTR)&buffer,
			0,
			NULL);
		boost::shared_ptr<VOID> bufferPtr(buffer, &::LocalFree);
		return GetLastError() != ERROR_RESOURCE_LANG_NOT_FOUND;
#	else
		// not implemented yet
		BOOST_STATIC_ASSERT(false);
#	endif
}

WString Error::GetString() const {
#	ifdef BOOST_WINDOWS
		LPVOID buffer;
		auto bufferSize = ::FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			m_errorNo,
			0,
			(LPWSTR)&buffer,
			0,
			NULL);
		boost::shared_ptr<VOID> bufferPtr(buffer, &::LocalFree);
		assert(GetLastError() != ERROR_RESOURCE_LANG_NOT_FOUND);
		assert(!bufferSize || bufferSize == wcslen(static_cast<LPCWSTR>(buffer)));
		for (
				; bufferSize > 0
					&& (static_cast<LPCWSTR>(buffer)[bufferSize - 1] == L'\n'
						|| static_cast<LPCWSTR>(buffer)[bufferSize - 1] == L'\r')
				; --bufferSize);
		if (bufferSize) {
			static_cast<LPWSTR>(buffer)[bufferSize - 1] = 0;
			return WString(static_cast<LPCWSTR>(buffer));
		} else {
			return WString(L"Unknown error");
		}
#	else // BOOST_WINDOWS
		WString result;
		namespace fs::boost::system;
		ConvertString(fs::system_error(m_errorNo, fs::get_system_category()).what(), result);
		return result;
#	endif // BOOST_WINDOWS
}

int Error::GetErrorNo() const {
	return m_errorNo;
}
