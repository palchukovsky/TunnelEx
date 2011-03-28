/**************************************************************************
 *   Created: 2008/01/03 4:57
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Config.cpp 1032 2010-10-14 14:20:52Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "Config.h"

#include "Version/Version.h"

using namespace boost::filesystem;

Config::Config()
		: wxFileConfig(
		wxT(""),
		wxT(""),
#		ifdef _WINDOWS
			SetConfFilePath(),
#		else // _WINDOWS
			wxT(""),
#		endif 
		wxT(""),
		wxCONFIG_USE_LOCAL_FILE) {
	//...//
}

Config::~Config() {
	//...//
}

#ifdef _WINDOWS

	const wxString & Config::SetConfFilePath() {
		
		if (!m_confFilePathCache.IsEmpty()) {
			return m_confFilePathCache;
		}

		std::vector<wxChar> buffer(MAX_PATH, 0);
		if (SHGetSpecialFolderPath(NULL, &buffer[0], CSIDL_APPDATA, TRUE)) {
			wpath path(&buffer[0]);
			path /= TUNNELEX_NAME_W;
			path /= L"TexCC.ini";
			try {
				create_directories(path.branch_path());
				m_confFilePathCache = path.string();
			} catch (const boost::filesystem::filesystem_error &) {
				//...//
			}
		}

		return m_confFilePathCache;

	}

	wxString Config::m_confFilePathCache;

#endif // _WINDOWS
