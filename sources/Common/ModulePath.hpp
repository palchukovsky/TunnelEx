/**************************************************************************
 *   Created: 2008/01/04 18:46
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.com
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__ModulePath_h__0801041846
#define INCLUDED_FILE__TUNNELEX__ModulePath_h__0801041846

// include "Xml.hpp" in file, that includes this file

#include "CompileWarningsBoost.h"
#	include <boost/filesystem.hpp>
#include "CompileWarningsBoost.h"
#include <Windows.h>

namespace TunnelEx { namespace Helpers {

	inline boost::filesystem::wpath GetModuleFilePath() {
		typedef std::vector<wchar_t> Buffer;
		Buffer buffer(_MAX_PATH + 1, 0);
		GetModuleFileNameW(NULL, &buffer[0], DWORD(buffer.size()) - 1);
		return boost::filesystem::wpath(&buffer[0]);
	}

	inline boost::filesystem::path GetModuleFilePathA() {
		typedef std::vector<char> Buffer;
		Buffer buffer(_MAX_PATH + 1, 0);
		GetModuleFileNameA(NULL, &buffer[0], DWORD(buffer.size()) - 1);
		return boost::filesystem::path(&buffer[0]);
	}

} }

#endif // INCLUDED_FILE__TUNNELEX__ModulePath_h__0801041846