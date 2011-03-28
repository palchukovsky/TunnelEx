/**************************************************************************
 *   Created: 2008/10/23 10:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Error.cpp 1086 2010-12-07 08:53:15Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "Error.hpp"

using namespace boost;
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
		DWORD bufferSize = ::FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			m_errorNo,
			MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
			(LPWSTR)&buffer,
			0,
			NULL);
		shared_ptr<VOID> bufferPtr(buffer, &::LocalFree);
		BOOST_ASSERT(!bufferSize || bufferSize == wcslen(static_cast<LPCWSTR>(buffer)));
		return bufferSize;
#	else
		// not implemented yet
		BOOST_STATIC_ASSERT(false);
#	endif
}

WString Error::GetString() const {
#	ifdef BOOST_WINDOWS
		LPVOID buffer;
		DWORD bufferSize = ::FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			m_errorNo,
			MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
			(LPWSTR)&buffer,
			0,
			NULL);
		shared_ptr<VOID> bufferPtr(buffer, &::LocalFree);
		BOOST_ASSERT(!bufferSize || bufferSize == wcslen(static_cast<LPCWSTR>(buffer)));
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
		using namespace boost::system;
		ConvertString(system_error(m_errorNo, get_system_category()).what(), result);
		return result;
#	endif // BOOST_WINDOWS
}

int Error::GetErrorNo() const {
	return m_errorNo;
}
