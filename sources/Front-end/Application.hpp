/**************************************************************************
 *   Created: 2007/11/15 15:21
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.com
 * -------------------------------------------------------------------
 *       $Id: Application.hpp 1033 2010-10-15 20:27:41Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Application_h__0711151521
#define INCLUDED_FILE__TUNNELEX__Application_h__0711151521

#include "Config.h"
#include "Theme.hpp"

class wxCHMHelpController;
class ServiceAdapter;

class Application : public wxApp {

public:
	
	Application();
	virtual ~Application();

public:

	Config & GetConfig();
	void GetServiceEndpoint(wxString &serviceEndpoint) const;
	const Theme & GetTheme() const;
	void DisplayHelp() const;
	void DisplayHelp(const wxString &topic) const;
	void OpenOrderPage() const;
	void OpenTrialRequestPage(bool isError = false) const;

public:

	bool Migrate();

#	if defined(_DEBUG) || defined(TEST)
		bool ActivateUnlimitedEditionMode() {
			m_isUnlimitedModeActive = true;
			return true;
		}
		bool IsUnlimitedModeActive() const {
			return m_isUnlimitedModeActive;
		}
#	else // #if defined(_DEBUG) || defined(TEST)
		inline bool IsUnlimitedModeActive() const {
			return false;
		}
#	endif // #if defined(_DEBUG) || defined(TEST)

public:

	virtual bool OnInit();
	virtual int OnExit();

private:

	Config m_config;
	Theme m_theme;

#	if defined(_DEBUG) || defined(TEST)
		bool m_isUnlimitedModeActive;
#	endif // #if defined(_DEBUG) || defined(TEST)

};

DECLARE_APP(Application)

#endif // INCLUDED_FILE__TUNNELEX__Application_h__0711151521
