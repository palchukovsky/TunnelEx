/**************************************************************************
 *   Created: 2007/11/15 15:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Application.cpp 1109 2010-12-26 06:33:37Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "Application.hpp"
#include "MainFrame.hpp"
#include "ServiceAdapter.hpp"

using namespace TunnelEx;

IMPLEMENT_APP(Application)

Application::Application()
#	if defined(_DEBUG) || defined(TEST)
		: m_isUnlimitedModeActive(false)
#	endif // #if defined(_DEBUG) || defined(TEST)
{
	wxImage::AddHandler(new wxGIFHandler);
	wxImage::AddHandler(new wxPNGHandler);
	ERR_load_crypto_strings();
}

Application::~Application() {
	ERR_free_strings();
}

bool Application::OnInit() {

	//! \todo: application name hardcode [2008/02/13 1:54]
	SetAppName(TUNNELEX_NAME_W wxT(" Control Center"));
	SetVendorName(TUNNELEX_VENDOR_W);

	if (argc > 1) {
		typedef std::map<std::wstring, boost::function<bool(void)> > Commands;
		Commands commands;
		commands[L"--migrateLocalService"] = boost::bind(&Application::Migrate, this);
#		if defined(_DEBUG) || defined(TEST)
			commands[L"--unlim"] = boost::bind(&Application::ActivateUnlimitedEditionMode, this);
#		endif // #if defined(_DEBUG) || defined(TEST)
		for (int i = 0; i < argc; ++i) {
			const Commands::const_iterator commandPos = commands.find(argv[i]);
			if (commandPos != commands.end()) {
				const bool toLaunchGui = commandPos->second();
				if (!toLaunchGui) {
					return false;
				}
			}
		}
	}

	std::auto_ptr<MainFrame> frame(new MainFrame);
	frame->Show(true);
	SetTopWindow(frame.release());

	return true;

}

int Application::OnExit() {
	const int result = wxApp::OnExit();
	return result;
}

Config & Application::GetConfig() {
	return m_config;
}

const Theme & Application::GetTheme() const {
	return m_theme;
}

void Application::GetServiceEndpoint(wxString &serviceEndpoint) const {
	wxRegKey serviceEndpointInstanceRegKey(wxT("HKEY_LOCAL_MACHINE\\SOFTWARE\\") TUNNELEX_NAME_W wxT("\\Service"));
	if (	serviceEndpointInstanceRegKey.Exists()
			&& !serviceEndpointInstanceRegKey.IsEmpty()) {
		serviceEndpointInstanceRegKey.QueryValue(wxT("InstanceEndpoint"), serviceEndpoint);
	}
	if (serviceEndpoint.IsEmpty()) {
		serviceEndpoint = wxT("http://localhost:");
		serviceEndpoint += TUNNELEX_SOAP_SERVICE_PORT;
	}
}

bool Application::Migrate() {
	wxGetApp().GetConfig().Write(wxT("/CheckNewVersion"), true);
	for (unsigned int attempts = 0; attempts < 10; ++attempts) {
		wxString serviceEndpoint;
		GetServiceEndpoint(serviceEndpoint);
		if (ServiceAdapter(serviceEndpoint).Migrate()) {
			break;
		}
		wxSleep(3);
	}
	return false;
}

void Application::DisplayHelp() const {
	DisplayHelp(wxEmptyString);
}

void Application::DisplayHelp(const wxString &topic) const {
	wxString url = wxT("http://") TUNNELEX_DOMAIN_W wxT("/knowledge-base/program-help/");
	url += topic;
	url += wxT("?about");
	ShellExecute(NULL, wxT("open"), url, NULL, NULL, SW_SHOWNORMAL);
}

void Application::OpenOrderPage() const {
	ShellExecute(
		NULL,
		wxT("open"),
		wxT("http://") TUNNELEX_DOMAIN_W wxT("/order?about"),
		NULL,
		NULL,
		SW_SHOWNORMAL);
}


void Application::OpenTrialRequestPage(bool isError /*= false*/) const {
	std::ostringstream url;
	url << "http://" TUNNELEX_DOMAIN "/order/trial?" << (isError ? "error" : "about");
	ShellExecuteA(
		NULL,
		"open",
		url.str().c_str(),
		NULL,
		NULL,
		SW_SHOWNORMAL);
}
