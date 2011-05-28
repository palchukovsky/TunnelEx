/**************************************************************************
 *   Created: 2007/11/15 14:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "MainFrame.hpp"
#include "LicenseDlg.hpp"
#include "LicenseStartDlg.hpp"
#include "Auto.hpp"
#include "Application.hpp"
#include "ServiceAdapter.hpp"
#include "LicensePolicies.hpp"
#include "NewVersionDlg.hpp"

#include "Legacy/LegacySupporter.hpp"

using namespace TunnelEx;
namespace pt = boost::posix_time;

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
	EVT_MENU(MainFrame::CMD_EXIT, MainFrame::OnCmdExit)
	EVT_MENU(MainFrame::CMD_RULE_ADD_CUSTOM, MainFrame::OnCmdRuleAddCustom)
	EVT_MENU(MainFrame::CMD_RULE_ADD_CUSTOM_ADVANCED, MainFrame::OnCmdRuleAddCustomAdvanced)
	EVT_MENU(MainFrame::CMD_RULE_ADD_UPNP, MainFrame::OnCmdRuleAddUpnp)
	EVT_MENU(MainFrame::CMD_RULE_ADD_FTP, MainFrame::OnCmdRuleAddFtp)
	EVT_MENU(MainFrame::CMD_RULE_EDIT, MainFrame::OnCmdRuleEdit)
	EVT_MENU(MainFrame::CMD_RULE_BACKUP_ALL, MainFrame::OnCmdRuleBackupAll)
	EVT_MENU(MainFrame::CMD_RULE_BACKUP_SELECTED, MainFrame::OnCmdRuleBackupSelected)
	EVT_MENU(MainFrame::CMD_RULE_RESTORE, MainFrame::OnCmdRuleRestore)
	EVT_MENU(MainFrame::CMD_RULE_COPY, MainFrame::OnCmdRuleCopy)
	EVT_MENU(MainFrame::CMD_RULE_CUT, MainFrame::OnCmdRuleCut)
	EVT_MENU(MainFrame::CMD_RULE_PASTE, MainFrame::OnCmdRulePaste)
	EVT_MENU(MainFrame::CMD_RULE_CHANGES_APPLY, MainFrame::OnCmdRuleChangesApply)
	EVT_MENU(MainFrame::CMD_RULE_CHANGES_CANCEL, MainFrame::OnCmdRuleChangesCancel)
	EVT_MENU(MainFrame::CMD_RULE_ENABLE, MainFrame::OnCmdRuleEnable)
	EVT_MENU(MainFrame::CMD_RULE_DISABLE, MainFrame::OnCmdRuleDisable)
	EVT_MENU(MainFrame::CMD_RULE_DELETE, MainFrame::OnCmdRuleDelete)
	EVT_MENU(MainFrame::CMD_RULE_DELETE_ALL, MainFrame::OnCmdRuleDeleteAll)
	EVT_MENU(MainFrame::CMD_RULE_SELECT_ALL, MainFrame::OnCmdRuleSelectAll)
	EVT_MENU(MainFrame::CMD_SORT_BY_NAME, MainFrame::OnCmdSortByName)
	EVT_MENU(MainFrame::CMD_SORT_BY_INPUTS, MainFrame::OnCmdSortByInputs)
	EVT_MENU(MainFrame::CMD_SORT_BY_DESTINATIONS, MainFrame::OnCmdSortByDestinations)
	EVT_MENU(MainFrame::CMD_SORT_BY_STATE, MainFrame::OnCmdSortByState)
	EVT_MENU(MainFrame::CMD_SERVER_START, MainFrame::OnCmdServerStart)
	EVT_MENU(MainFrame::CMD_SERVER_STOP, MainFrame::OnCmdServerStop)
	EVT_MENU(MainFrame::CMD_SERVICE_LOG_OPEN_CLOSE, MainFrame::OnCmdServiceLogShowHide)
	EVT_MENU(MainFrame::CMD_HELP_CONTENTS, MainFrame::OnCmdHelpContents)
	EVT_MENU(MainFrame::CMD_WWW_HOME_PAGE, MainFrame::OnCmdWwwHomePage)
	EVT_MENU(MainFrame::CMD_BLOG, MainFrame::OnCmdBlog)
	EVT_MENU(MainFrame::CMD_ABOUT, MainFrame::OnCmdAbout)
	EVT_MENU(MainFrame::CMD_ISSUE_SUBMIT, MainFrame::OnCmdIssueSubmit)
	EVT_MENU(MainFrame::CMD_SERVICE_CHANGES_APPLY, MainFrame::OnCmdServiceChangesApply)
	EVT_MENU(MainFrame::CMD_SERVICE_CHANGES_CANCEL, MainFrame::OnCmdServiceChangesCancel)
	EVT_MENU(MainFrame::CMD_SERVICE_SSL_SERTIFICATE_LIST, MainFrame::OnCmdServiceSslCertificateList)
	EVT_MENU(MainFrame::CMD_LICENSE_INFO_VIEW, MainFrame::OnCmdLicenseInfoView)
	EVT_MENU(MainFrame::CMD_GO_PRO, MainFrame::OnGoPro)
	EVT_TOOL_ENTER(CONTROL_TOOLBAR, MainFrame::OnToolEnter)
	EVT_CLOSE(MainFrame::OnClose)
	EVT_UPDATE_CHECK(
		UpdateChecker::Event::ID_NEW_RELEASE_CHECKED,
		MainFrame::OnCmdHandleReleaseInfo)
	EVT_SERVICE_WINDOW(
		ServiceWindow::Event::ID_SERVICE_RUNING_STATE_CHANGED,
		MainFrame::OnServiceRunningStateChanged)
	EVT_SERVICE_WINDOW(
		ServiceWindow::Event::ID_RULE_SET_CHANGED,
		MainFrame::OnRuleSetChanged)
	EVT_SERVICE_WINDOW(
		ServiceWindow::Event::ID_CONNECTION_TO_SERVICE_STATE_CHANGED,
		MainFrame::OnConnectionToServiceStateChanged)		
END_EVENT_TABLE()

MainFrame::MainFrame()
		: wxFrame(
			NULL,
			wxID_ANY,
			wxGetApp().GetAppName() + TUNNELEX_BUILD_IDENTITY_ADD_W,
			GetFramePosition(),
			GetFrameSize()),
		m_updateCheckerStat(new UpdateChecker::Stat) {

	SetIcon(wxICON(ICON));
    
	CreateStatusBar();

	CreateToolBar(false);
	CreateMenu(false);

	m_window.reset(new ServiceWindow(this, -1));

	Show();
	Enable();
	Update();

}

MainFrame::~MainFrame() {
	{
		const std::auto_ptr<const UpdateChecker> updateChecker = m_updateChecker;
	}
	if (!IsMaximized()) {
		Config &config = wxGetApp().GetConfig();
		const wxPoint pos = GetPosition();
		config.Write(wxT("/Window/Position/X"), pos.x);
		config.Write(wxT("/Window/Position/Y"), pos.y);
		const wxSize size = GetSize();
		config.Write(wxT("/Window/Size/Width"), size.x);
		config.Write(wxT("/Window/Size/Height"), size.y);
	}
}

wxPoint MainFrame::GetFramePosition() {
	//! \todo: check screen size and correct position with it [2008/02/23 15:01]
	wxPoint result(
		wxGetApp().GetConfig().Read(wxT("/Window/Position/X"), -2),
		wxGetApp().GetConfig().Read(wxT("/Window/Position/Y"), -2));
	if (result.x < 0 || result.y < 0) {
		result = wxDefaultPosition;
	}
	return result;
}

wxSize MainFrame::GetFrameSize() {
	wxSize result;
	wxGetApp().GetConfig().Read(wxT("/Window/Size/Width"), &result.x, 0);
	wxGetApp().GetConfig().Read(wxT("/Window/Size/Height"), &result.y, 0);
	if (!result.x || !result.y) {
		result.x = 640;
		result.y = 390;
	}
	return result;
}

void MainFrame::CreateStatusBar() {
	wxFrame::CreateStatusBar(SBP_SIZE);
	int widths[] = {-1, 100, /*60, */300, 200};
	SetStatusWidths(4, widths);
	int styles[] = {wxSB_FLAT, wxSB_FLAT, /*wxSB_FLAT, */wxSB_FLAT, wxSB_FLAT};
	GetStatusBar()->SetStatusStyles(4, styles);
	SetConnectionStatus(wxT("disconnected"));
	SetServiceStatus(wxT("unknown"));
}

void MainFrame::CreateToolBar(bool goPro) {

	wxToolBar &toolBar = *new wxToolBar(
		this,
		CONTROL_TOOLBAR,
		wxDefaultPosition,
		wxDefaultSize,
		wxTB_FLAT | wxBORDER_NONE | wxTB_HORIZONTAL);

	wxBitmap icon;
	wxGetApp().GetTheme().GetAddRuleIcon(icon, true);
	toolBar.AddTool(CMD_RULE_ADD_CUSTOM, icon, wxT("New Rule"));
	toolBar.EnableTool(CMD_RULE_ADD_CUSTOM, false);

	wxGetApp().GetTheme().GetEditRuleIcon(icon, true);
	toolBar.AddTool(CMD_RULE_EDIT, icon, wxT("Edit Rule"));
	toolBar.EnableTool(CMD_RULE_EDIT, false);
	
	toolBar.AddSeparator();

	wxGetApp().GetTheme().GetCutIcon(icon, true);
	toolBar.AddTool(CMD_RULE_CUT, icon, wxT("Cut Rule(s)"));
	toolBar.EnableTool(CMD_RULE_CUT, false);

	wxGetApp().GetTheme().GetCopyIcon(icon, true);
	toolBar.AddTool(CMD_RULE_COPY, icon, wxT("Copy Rule(s)"));
	toolBar.EnableTool(CMD_RULE_COPY, false);

	wxGetApp().GetTheme().GetPasteIcon(icon, true);
	toolBar.AddTool(CMD_RULE_PASTE, icon, wxT("Paste Rule(s)"));
	toolBar.EnableTool(CMD_RULE_PASTE, false);

	wxGetApp().GetTheme().GetRemoveRuleIcon(icon, true);
	toolBar.AddTool(CMD_RULE_DELETE, icon, wxT("Delete Rule(s)"));
	toolBar.EnableTool(CMD_RULE_DELETE, false);
		
	toolBar.AddSeparator();

	wxGetApp().GetTheme().GetGoRuleIcon(icon, true);
	toolBar.AddTool(CMD_RULE_ENABLE, icon, wxT("Enable Rule(s)"));
	toolBar.EnableTool(CMD_RULE_ENABLE, false);

	wxGetApp().GetTheme().GetRuleIcon(icon, true);
	toolBar.AddTool(CMD_RULE_DISABLE, icon, wxT("Disable Rule(s)"));
	toolBar.EnableTool(CMD_RULE_DISABLE, false);
	
	toolBar.AddSeparator();

	wxGetApp().GetTheme().GetLogIcon(icon, true);
	toolBar.AddTool(CMD_SERVICE_LOG_OPEN_CLOSE, icon, wxT("Show/Hide Log"));
	toolBar.EnableTool(CMD_SERVICE_LOG_OPEN_CLOSE, false);

	toolBar.AddSeparator();

	wxGetApp().GetTheme().GetPlayIcon(icon, true);
	toolBar.AddTool(CMD_SERVER_START, icon, wxT("Start ") TUNNELEX_NAME_W);
	toolBar.EnableTool(CMD_SERVER_START, false);

	wxGetApp().GetTheme().GetStopIcon(icon, true);
	toolBar.AddTool(CMD_SERVER_STOP, icon, wxT("Stop ") TUNNELEX_NAME_W);
	toolBar.EnableTool(CMD_SERVER_STOP, false);

	toolBar.AddSeparator();

	wxGetApp().GetTheme().GetHelpIcon(icon, true);
	toolBar.AddTool(CMD_HELP_CONTENTS, icon, wxT("Help Topics"));

	wxGetApp().GetTheme().GetBugIcon(icon, true);
	toolBar.AddTool(CMD_ISSUE_SUBMIT, icon, wxT("Submit Issue"));

	if (goPro) {
		wxGetApp().GetTheme().GetGoProIcon(icon, true);
		toolBar.AddTool(CMD_GO_PRO, icon, wxT("Go Pro!"));
	}

	toolBar.Realize();

	SetToolBar(&toolBar);

}

void MainFrame::CreateMenu(bool goPro) {
    
	std::auto_ptr<wxMenu> file(new wxMenu);
	wxBitmap icon;

	wxMenuItem *item;
	{
		std::auto_ptr<wxMenu> subMenu(new wxMenu);
		item = new wxMenuItem(
			subMenu.get(),
			CMD_RULE_ADD_CUSTOM,
			wxT("&Rule...\tCtrl-N"));
		wxGetApp().GetTheme().GetAddRuleIcon(icon, false);
		item->SetBitmap(icon);
		subMenu->Append(item);
		subMenu->Enable(CMD_RULE_ADD_CUSTOM, false);
		//
		subMenu->Append(
			CMD_RULE_ADD_CUSTOM_ADVANCED,
			wxT("Advanced Rule..."));
		subMenu->Enable(CMD_RULE_ADD_CUSTOM_ADVANCED, false);
		//
		subMenu->Append(CMD_RULE_ADD_UPNP, wxT("External Port Mapping by &UPnP..."));
		subMenu->Enable(CMD_RULE_ADD_UPNP, false);
		//		
		subMenu->Append(CMD_RULE_ADD_FTP, wxT("Advanced Rule for &FTP Tunnel..."));
		subMenu->Enable(CMD_RULE_ADD_FTP, false);
		//		
		file->AppendSubMenu(subMenu.get(), wxT("&New"));
		subMenu.release();
	}
	file->AppendSeparator();
	{
		std::auto_ptr<wxMenu> subMenu(new wxMenu);
		subMenu->Append(CMD_RULE_BACKUP_ALL, wxT("Backup &rule std::set..."));
		subMenu->Enable(CMD_RULE_BACKUP_ALL, false);
		subMenu->Append(CMD_RULE_BACKUP_SELECTED, wxT("Backup &selected rules..."));
		subMenu->Enable(CMD_RULE_BACKUP_SELECTED, false);
		file->AppendSubMenu(subMenu.get(), wxT("&Backup"));
		subMenu.release();
	}
	file->Append(CMD_RULE_RESTORE, wxT("&Restore rules..."));
	file->Enable(CMD_RULE_RESTORE, false);
    file->AppendSeparator();
    file->Append(CMD_EXIT, wxT("E&xit"));

	std::auto_ptr<wxMenu> edit(new wxMenu);
	item = new wxMenuItem(
		edit.get(),
		CMD_RULE_CUT,
		wxT("Cu&t Rule(s)\tCtrl-X"));
	wxGetApp().GetTheme().GetCutIcon(icon, false);
	item->SetBitmap(icon);
	edit->Append(item);
	edit->Enable(CMD_RULE_CUT, false);
	item = new wxMenuItem(edit.get(), CMD_RULE_COPY, wxT("&Copy Rule(s)\tCtrl-C"));
	wxGetApp().GetTheme().GetCopyIcon(icon, false);
	item->SetBitmap(icon);
	edit->Append(item);
	edit->Enable(CMD_RULE_COPY, false);
	item = new wxMenuItem(edit.get(), CMD_RULE_PASTE, wxT("&Paste Rule(s)\tCtrl-V"));
	wxGetApp().GetTheme().GetPasteIcon(icon, false);
	item->SetBitmap(icon);
	edit->Append(item);
	edit->Enable(CMD_RULE_PASTE, false);
	item = new wxMenuItem(file.get(), CMD_RULE_DELETE, wxT("&Delete Rule(s)\tDEL"));
	wxGetApp().GetTheme().GetRemoveRuleIcon(icon, false);
	item->SetBitmap(icon);
	edit->Append(item);
	edit->Enable(CMD_RULE_DELETE, false);
	edit->Append(CMD_RULE_DELETE_ALL, wxT("Delete &All Rules\tCtrl+Shift+DEL"));
	edit->Enable(CMD_RULE_DELETE_ALL, false);
	edit->AppendSeparator();
	edit->Append(CMD_RULE_SELECT_ALL, wxT("&Select All\tCtrl-A"));
	edit->AppendSeparator();
	item = new wxMenuItem(edit.get(), CMD_RULE_EDIT, wxT("&Edit Rule..."));
	wxGetApp().GetTheme().GetEditRuleIcon(icon, false);
	item->SetBitmap(icon);
	edit->Append(item);
	edit->Enable(CMD_RULE_EDIT, false);
	edit->AppendSeparator();
	item = new wxMenuItem(edit.get(), CMD_RULE_ENABLE, wxT("E&nable Rule(s)"));
	wxGetApp().GetTheme().GetGoRuleIcon(icon, false);
	item->SetBitmap(icon);
	edit->Append(item);
	edit->Enable(CMD_RULE_ENABLE, false);
	item = new wxMenuItem(edit.get(), CMD_RULE_DISABLE, wxT("D&isable Rule(s)"));
	wxGetApp().GetTheme().GetRuleIcon(icon, false);
	item->SetBitmap(icon);
	edit->Append(item);
	edit->Enable(CMD_RULE_DISABLE, false);
	edit->AppendSeparator();
	edit->Append(CMD_RULE_CHANGES_APPLY, wxT("App&ly Rule(s) Changes"));
	edit->Enable(CMD_RULE_CHANGES_APPLY, false);
	edit->Append(CMD_RULE_CHANGES_CANCEL, wxT("Cancel &Rule(s) Changes"));
	edit->Enable(CMD_RULE_CHANGES_CANCEL, false);

	std::auto_ptr<wxMenu> view(new wxMenu);
    view->Append(CMD_SORT_BY_NAME, wxT("Sort Rules by &Name"));
	view->Append(CMD_SORT_BY_INPUTS, wxT("Sort Rules by &Inputs"));
	view->Append(CMD_SORT_BY_DESTINATIONS, wxT("Sort Rules by &Destintation"));
	view->Append(CMD_SORT_BY_STATE, wxT("Sort Rules by &State"));

	std::auto_ptr<wxMenu> service(new wxMenu);
	item = new wxMenuItem(service.get(), CMD_SERVICE_LOG_OPEN_CLOSE, wxT("Show/Hide &Log...\tF2"));
	wxGetApp().GetTheme().GetLogIcon(icon, false);
	item->SetBitmap(icon);
	service->Append(item);
	service->Enable(CMD_SERVICE_LOG_OPEN_CLOSE, false);
	service->AppendSeparator();
	service->Append(CMD_SERVICE_CHANGES_APPLY, wxT("&Apply Changes\tCtrl-S"));
	service->Enable(CMD_SERVICE_CHANGES_APPLY, false);
	service->Append(CMD_SERVICE_CHANGES_CANCEL, wxT("&Cancel Changes"));
	service->Enable(CMD_SERVICE_CHANGES_CANCEL, false);
	service->AppendSeparator();
	item = new wxMenuItem(service.get(), CMD_SERVER_START, wxT("&Start ") TUNNELEX_NAME_W wxT("\tF10"));
	wxGetApp().GetTheme().GetPlayIcon(icon, false);
	item->SetBitmap(icon);
	service->Append(item);
	service->Enable(CMD_SERVER_START, false);
	item = new wxMenuItem(service.get(), CMD_SERVER_STOP, wxT("S&top ") TUNNELEX_NAME_W wxT("\tCtrl+F10"));
	wxGetApp().GetTheme().GetStopIcon(icon, false);
	item->SetBitmap(icon);
	service->Append(item);
	service->Enable(CMD_SERVER_STOP, false);
	service->AppendSeparator();
	service->Append(CMD_SERVICE_SSL_SERTIFICATE_LIST, wxT("&Manage SSL Certificates..."));
	service->Enable(CMD_SERVICE_SSL_SERTIFICATE_LIST, false);
 
	std::auto_ptr<wxMenu> help(new wxMenu);
	item = new wxMenuItem(help.get(), CMD_HELP_CONTENTS, wxT("&Help Topics"));
	wxGetApp().GetTheme().GetHelpIcon(icon, false);
	item->SetBitmap(icon);
	help->Append(item);
	help->AppendSeparator();
	help->Append(CMD_WWW_HOME_PAGE, wxT("&Web Site"));
	item = new wxMenuItem(help.get(), CMD_BLOG, wxT("&Blog"));
	wxGetApp().GetTheme().GetBlogIcon(icon);
	item->SetBitmap(icon);
	help->Append(item);
	item = new wxMenuItem(help.get(), CMD_ISSUE_SUBMIT, wxT("&Submit Issue..."));
	wxGetApp().GetTheme().GetBugIcon(icon, false);
	item->SetBitmap(icon);
	help->Append(item);
	help->AppendSeparator();
	if (goPro) {
		item = new wxMenuItem(help.get(), CMD_GO_PRO, wxT("&Go Pro..."));
		wxGetApp().GetTheme().GetGoProIcon(icon, false);
		item->SetBitmap(icon);
		help->Append(item);
	}
	help->Append(CMD_LICENSE_INFO_VIEW, wxT("&License Info..."));
	help->Enable(CMD_LICENSE_INFO_VIEW, false);
	help->AppendSeparator();
	help->Append(CMD_ABOUT, wxT("&About..."));

	std::auto_ptr<wxMenuBar> menubar(new wxMenuBar);
	menubar->Append(file.release(), wxT("&File"));
	menubar->Append(edit.release(), wxT("&Edit"));
	menubar->Append(view.release(), wxT("&View"));
	menubar->Append(service.release(), wxT("&Service"));
	menubar->Append(help.release(), wxT("&Help"));

	SetMenuBar(menubar.get());
	menubar.release();

}

void MainFrame::OnCmdExit(wxCommandEvent &) {
    Close();
}

void MainFrame::OnCmdRuleAddCustom(wxCommandEvent &) {
	m_window->AddCustomRule();
}

void MainFrame::OnCmdRuleAddCustomAdvanced(wxCommandEvent &) {
	m_window->AddCustomRuleAdvanced();
}

void MainFrame::OnCmdRuleAddUpnp(wxCommandEvent &) {
	m_window->AddUpnpRule();
}

void MainFrame::OnCmdRuleAddFtp(wxCommandEvent &) {
	m_window->AddFtpTunnelRule();
}

void MainFrame::BackupRules(const wxString &exportedRuleSet) {
	wxFileDialog fileRequestDlg(
		this,
		wxT("Choose a file to backup rules"),
		wxEmptyString,
		wxEmptyString,
		wxT("ZIP files (*.zip)|*.zip|All files (*.*)|*.*"),
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fileRequestDlg.ShowModal() != wxID_OK) {
		return;
	}
	assert(!fileRequestDlg.GetPath().IsEmpty());
	{
		wxFFileOutputStream out(fileRequestDlg.GetPath());
		assert(out.IsOk());
		if (!out.IsOk()) {
			return;
		}
		wxZipOutputStream zip(out);
		assert(zip.IsOk());
		if (!zip.IsOk()) {
			return;
		}
		zip.SetLevel(9);
		zip.PutNextEntry(wxT("RuleSet.xml"));
		wxStringInputStream exportedRuleSetStream(exportedRuleSet);
		zip.Write(exportedRuleSetStream);
	}
	WFormat message(wxT("Successfully saved %1% rule(s)."));
	message % RuleSet(WString(exportedRuleSet.c_str())).GetSize();
	wxMessageBox(
		message.str().c_str(),
		wxT("Rules backup"),
		wxOK | wxICON_INFORMATION,
		this);
}

void MainFrame::OnCmdRuleBackupAll(wxCommandEvent &) {
	BackupRules(m_window->ExportRules());
}

void MainFrame::OnCmdRuleBackupSelected(wxCommandEvent &) {
	BackupRules(m_window->ExportSelectedRules());
}

void MainFrame::OnCmdRuleRestore(wxCommandEvent &) {
	
	struct Util {
		static void ShowErrorFormatMessage(wxWindow *parent) {
			wxMessageBox(
				wxT("The rules restore has been failed. ")
					wxT("File is empty or has invalid format."),
				wxT("Rules restore"),
				wxOK | wxICON_ERROR,
				parent);
		}
		static void ShowErrorDllMessage(wxWindow *parent) {
			wxMessageBox(
				wxT("The rules restore has been failed. ")
					wxT("Could not load or use one or more components. ")
					wxT("Please reinstall ") TUNNELEX_NAME_W wxT("."),
				wxT("Rules restore"),
				wxOK | wxICON_ERROR,
				parent);
		}
		static void ShowSuccessMessage(
					wxWindow *parent,
					size_t restoredRulesNumber) {
			WFormat message(wxT("Successfully restored %1% rule(s)."));
			message % restoredRulesNumber;
			wxMessageBox(
				message.str().c_str(),
				wxT("Rules restore"),
				wxOK | wxICON_INFORMATION,
				parent);
		}
	};

	wxLogNull logNo;

	WString ruleSetXml;

	wxFileDialog fileRequestDlg(
		this,
		wxT("Choose a file to restore rules"),
		wxEmptyString,
		wxEmptyString,
		wxT("ZIP files (*.zip)|*.zip|XML files (*.xml)|*.xml|All files (*.*)|*.*"),
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fileRequestDlg.ShowModal() != wxID_OK) {
		return;
	}

	wxFFileInputStream in(fileRequestDlg.GetPath());
	assert(in.IsOk());
	assert(in.IsSeekable());
	if (!in.IsOk() || !in.IsSeekable()) {
		return;
	}

	{
		wxZipInputStream zip(in);
		assert(zip.IsOk());
		if (zip.IsOk()) {
			std::auto_ptr<wxZipEntry> entry;
			const wxString entryName = wxZipEntry::GetInternalName(wxT("RuleSet.xml"));
			do {
				entry.reset(zip.GetNextEntry());
			} while (entry.get() != NULL && entry->GetInternalName() != entryName);
			if (entry.get()) {
				wxStringOutputStream ruleSetXmlStream;
				zip.Read(ruleSetXmlStream);
				ruleSetXml = ruleSetXmlStream.GetString().c_str();
			}
			if (ruleSetXml.IsEmpty() && zip.GetTotalEntries()) {
				Util::ShowErrorFormatMessage(this);
				return;
			}
		}
	}

	if (ruleSetXml.IsEmpty()) {
		wxStringOutputStream ruleSetXmlStream;
		in.SeekI(0);
		in.Read(ruleSetXmlStream);
		ruleSetXml = ruleSetXmlStream.GetString().c_str();
		if (ruleSetXml.IsEmpty()) {
			Util::ShowErrorFormatMessage(this);
			return;
		}
	}

	RuleSet importedRuleSet;
	try {
		RuleSet(ruleSetXml).Swap(importedRuleSet);
	} catch (const TunnelEx::InvalidXmlException &) {
		try {
			LegacySupporter().MigrateRuleSet(ruleSetXml, importedRuleSet);
		} catch (const TunnelEx::DllException &) {
			Util::ShowErrorDllMessage(this);
			return;
		}
	}

	if (!importedRuleSet.GetSize()) {
		Util::ShowErrorFormatMessage(this);
		return;
	}
	
	const size_t updatedRulesNumb
		= m_window->ImportRules(importedRuleSet, true);
	Util::ShowSuccessMessage(this, updatedRulesNumb);

}

void MainFrame::OnCmdRuleEdit(wxCommandEvent &) {
	m_window->EditSelectedRule();
}

void MainFrame::OnCmdRuleCopy(wxCommandEvent &) {
	m_internalClipboard = m_window->ExportSelectedRules();
	const bool isConnected = m_window->GetService().IsConnected();
	GetMenuBar()->Enable(CMD_RULE_PASTE, isConnected && !m_internalClipboard.IsEmpty());
	GetToolBar()->EnableTool(CMD_RULE_PASTE, isConnected && !m_internalClipboard.IsEmpty());
}

void MainFrame::OnCmdRuleCut(wxCommandEvent &event) {
	OnCmdRuleCopy(event);
	m_window->DeleteSelectedRules();
}

void MainFrame::OnCmdRulePaste(wxCommandEvent &) {
	assert(!m_internalClipboard.IsEmpty());
	if (!m_internalClipboard.IsEmpty()) {
		m_window->ImportRules(m_internalClipboard, false);
	}
}

void MainFrame::OnCmdRuleChangesApply(wxCommandEvent &) {
	m_window->ApplyChangesForSelectedRules();
}

void MainFrame::OnCmdRuleChangesCancel(wxCommandEvent &) {
	m_window->CancelChangesForSelectedRules();
}

void MainFrame::OnCmdRuleEnable(wxCommandEvent &) {
	m_window->EnableSelectedRules();
}

void MainFrame::OnCmdRuleDisable(wxCommandEvent &) {
	m_window->DisableSelectedRules();
}

void MainFrame::OnCmdRuleDelete(wxCommandEvent &) {
	m_window->DeleteSelectedRules();
}

void MainFrame::OnCmdRuleDeleteAll(wxCommandEvent &) {
	m_window->DeleteAllRules();
}

void MainFrame::OnCmdRuleSelectAll(wxCommandEvent &) {
	m_window->SelectAllRules();
}

void MainFrame::OnServiceRunningStateChanged(ServiceWindow::Event &) {

	const bool isStarted = m_window->GetService().IsStarted();

	CheckServiceStatus();
	CheckRuleSetStatus();

	wxMenuBar &menu = *GetMenuBar();
	menu.Enable(CMD_SERVER_START, !isStarted);
	menu.Enable(CMD_SERVER_STOP, isStarted);

	wxToolBar &toolBar = *GetToolBar();
	toolBar.EnableTool(CMD_SERVER_START, !isStarted);
	toolBar.EnableTool(CMD_SERVER_STOP, isStarted);

}

void MainFrame::OnCmdSortByName(wxCommandEvent &) {
	m_window->SortRuleListViewByName();
}

void MainFrame::OnCmdSortByInputs(wxCommandEvent &) {
	m_window->SortRuleListViewByInputs();
}

void MainFrame::OnCmdSortByDestinations(wxCommandEvent &) {
	m_window->SortRuleListViewByDestinations();
}

void MainFrame::OnCmdSortByState(wxCommandEvent &) {
	m_window->SortRuleListViewByState();
}

void MainFrame::OnCmdServerStart(wxCommandEvent &) {
	m_window->GetService().Start();
}

void MainFrame::OnCmdServerStop(wxCommandEvent &) {
	m_window->GetService().Stop();
}

void MainFrame::OnCmdServiceLogShowHide(wxCommandEvent &) {
	if (m_window->IsLogOpened()) {
		m_window->CloseServiceLog();	
	} else {
		m_window->OpenServiceLog();
	}
}

void MainFrame::OnCmdLicenseInfoView(wxCommandEvent &) {
	LicenseDlg(*m_window, this).ShowModal();
}

void MainFrame::OnCmdHelpContents(wxCommandEvent &) {
	wxGetApp().DisplayHelp();
}

void MainFrame::OnCmdWwwHomePage(wxCommandEvent &) {
	ShellExecute(
		NULL,
		wxT("open"),
		wxT("http://") TUNNELEX_DOMAIN_W wxT("/?about"),
		NULL,
		NULL,
		SW_SHOWNORMAL);
}

void MainFrame::OnCmdBlog(wxCommandEvent &) {
	ShellExecute(
		NULL,
		wxT("open"),
		wxT("http://blog.") TUNNELEX_DOMAIN_W wxT("/?about"),
		NULL,
		NULL,
		SW_SHOWNORMAL);
}

void MainFrame::OnCmdIssueSubmit(wxCommandEvent &) {
	ShellExecute(
		NULL,
		wxT("open"),
		wxT("http://") TUNNELEX_DOMAIN_W wxT("/issue/submit?about"),
		NULL,
		NULL,
		SW_SHOWNORMAL);
}

void MainFrame::OnCmdAbout(wxCommandEvent &) {
	//! \todo: hardcode: name, e-mail [2008/01/03 3:28]
	wxAboutDialogInfo info;
	info.SetIcon(wxICON(ICON));
	//! @todo: replace product name after CC will be independent product, and remove edition name from here
	{
		std::string productName = TUNNELEX_NAME;
		if (GetMenuBar()->IsEnabled(CMD_LICENSE_INFO_VIEW)) {
			productName += " " + Licensing::InfoLicense(LicenseState(m_window->GetService())).GetEditionName();
		}
		info.SetName(wxString::FromAscii(productName.c_str()));
	}
	info.SetVersion(TUNNELEX_VERSION_FULL_W TUNNELEX_BUILD_IDENTITY_ADD_W);
	info.SetDescription(wxT("See License topic in help for details."));
	info.SetCopyright(TUNNELEX_COPYRIGHT_W);
	info.SetWebSite(
		wxT("http://") TUNNELEX_DOMAIN_W wxT("/?about"),
		wxT("http://") TUNNELEX_DOMAIN_W);
	wxAboutBox(info);
}

void MainFrame::OnCmdHandleReleaseInfo(UpdateChecker::Event &) {
	const std::auto_ptr<const UpdateChecker> updateChecker = m_updateChecker;
	if (	!updateChecker.get()
			|| !updateChecker->IsReleaseNew()
			|| !wxGetApp().GetConfig().Read(wxT("/CheckNewVersion"), true)) {
		return;
	}
	NewVersionDlg dlg(updateChecker->GetCurrentVersion(), this);
	dlg.ShowModal();
	if (!dlg.IsNewVersionCheckingOn()) {
		// will be reseted at next migration
		wxGetApp().GetConfig().Write(wxT("/CheckNewVersion"), false);
	}
}

void MainFrame::OnCmdServiceChangesApply(wxCommandEvent &) {
	m_window->ApplyChanges();
}

void MainFrame::OnCmdServiceChangesCancel(wxCommandEvent &) {
	m_window->CancelChanges();
}

void MainFrame::OnCmdServiceSslCertificateList(wxCommandEvent &) {
	m_window->ShowSslCertficateList();
}

void MainFrame::OnRuleSetChanged(ServiceWindow::Event &) {

	CheckRuleSetStatus();

	wxToolBar &toolBar = *GetToolBar();
	wxMenuBar &menu = *GetMenuBar();

	const bool isConnected = m_window->GetService().IsConnected();
	const size_t selected = isConnected
		?	m_window->GetSelectedRulesCount()
		:	0;
	const size_t rulesCount = isConnected
		?	m_window->GetRulesCount()
		:	0; 
	const bool hasModifiedRules = isConnected
		?	m_window->HasModifiedRules()
		:	0; 
	const bool isEditableSelected
		= selected > 0 && m_window->GetEditableSelectedRulesCount() > 0;
	const bool isModifedSelected
		= selected > 0 && m_window->GetModifiedSelectedRulesCount() > 0;
	const bool isDisabledSelected
		= selected > 0 && m_window->GetDisabledSelectedRulesCount() > 0;
	const bool isEnabledSelected
		= selected > 0 && m_window->GetEnabledSelectedRulesCount() > 0;

	toolBar.EnableTool(CMD_RULE_ADD_CUSTOM, isConnected);
	menu.Enable(CMD_RULE_ADD_CUSTOM, isConnected);

	menu.Enable(CMD_RULE_ADD_CUSTOM_ADVANCED, isConnected);

	menu.Enable(CMD_RULE_ADD_UPNP, isConnected);

	menu.Enable(CMD_RULE_ADD_FTP, isConnected);

	menu.Enable(CMD_RULE_BACKUP_SELECTED, selected > 0);
	menu.Enable(CMD_RULE_BACKUP_ALL, rulesCount > 0);

	toolBar.EnableTool(CMD_RULE_CUT, isEditableSelected);
	menu.Enable(CMD_RULE_CUT, isEditableSelected);

	toolBar.EnableTool(CMD_RULE_COPY, selected > 0);
	menu.Enable(CMD_RULE_COPY, selected > 0);

	toolBar.EnableTool(CMD_RULE_ENABLE, isDisabledSelected);
	menu.Enable(CMD_RULE_ENABLE, isDisabledSelected);

	toolBar.EnableTool(CMD_RULE_DISABLE, isEnabledSelected);
	menu.Enable(CMD_RULE_DISABLE, isEnabledSelected);

	toolBar.EnableTool(CMD_RULE_DELETE, isEditableSelected);
	menu.Enable(CMD_RULE_DELETE, isEditableSelected);

	menu.Enable(CMD_RULE_DELETE_ALL, rulesCount > 0);

	toolBar.EnableTool(CMD_RULE_EDIT, selected == 1 && isEditableSelected);
	menu.Enable(CMD_RULE_EDIT, selected == 1 && isEditableSelected);

	menu.Enable(CMD_RULE_CHANGES_APPLY, isModifedSelected);
	menu.Enable(CMD_RULE_CHANGES_CANCEL, isModifedSelected);

	menu.Enable(CMD_SERVICE_CHANGES_APPLY, hasModifiedRules);
	menu.Enable(CMD_SERVICE_CHANGES_CANCEL, hasModifiedRules);
	
	toolBar.EnableTool(CMD_SERVER_START, isConnected && !m_window->GetService().IsStarted());
	menu.Enable(CMD_SERVER_START, isConnected && !m_window->GetService().IsStarted());
	
	toolBar.EnableTool(CMD_SERVER_STOP, isConnected && m_window->GetService().IsStarted());
	menu.Enable(CMD_SERVER_STOP, isConnected && m_window->GetService().IsStarted());

	if (	m_updateCheckerStat.get()
			&& m_window->GetService().IsConnected()
			&& !m_updateCheckerStat->licenseKey.empty()) {
		wxString uuidStr = wxGetApp().GetConfig().Read(wxT("/Uuid"));
		if (uuidStr.IsEmpty()) {
			uuidStr = TunnelEx::Helpers::Uuid().GetAsString();
			wxGetApp().GetConfig().Write(wxT("/Uuid"), uuidStr);
		}
		m_updateCheckerStat->installationUuid = uuidStr.c_str();
		m_updateCheckerStat->rulesCount = m_window->GetRuleCountStat();
		m_updateChecker.reset(new UpdateChecker(this, *m_updateCheckerStat));
		m_updateChecker->RunCheck();
		m_updateCheckerStat.reset();
	}

}

void MainFrame::SetConnectionStatus(const wxString &state) {
	wxString connectionState = wxT("control center status: ");
	connectionState += state;
	SetStatusText(connectionState, SBP_CONNECTION_STATE);
}

void MainFrame::SetServiceStatus(const wxString &state) {
	wxString serviceState = wxT("service status: ");
	serviceState += state;
	SetStatusText(serviceState, SBP_SERVICE_STATE);
}

void MainFrame::CheckServiceStatus() {
	SetServiceStatus(
		m_window->GetService().IsConnected()
			?	m_window->GetService().IsStarted() ? wxT("started") : wxT("stopped")
			:	wxT("unknown"));
}

void MainFrame::CheckRuleSetStatus() {
	if (m_window->GetService().IsConnected()) {
		WFormat rulesCount(L"rules: %1%");
		// WFormat activeRulesCount(L"active: %1%");
		rulesCount %
			(m_window->GetRulesCount()
				- m_window->GetAddedRulesCount()
				- m_window->GetDeletedRulesCount());
		/* if (m_window->GetService().IsStarted()) {
			activeRulesCount % m_window->GetEnabledRulesCount();
		} else {
			activeRulesCount % 0;
		} */
		SetStatusText(rulesCount.str(), SBP_RULES_COUNT);
		// SetStatusText(activeRulesCount.str(), SBP_ACTIVE_RULES_COUNT);
	} else {
		SetStatusText(wxEmptyString, SBP_RULES_COUNT);
		// SetStatusText(wxEmptyString, SBP_ACTIVE_RULES_COUNT);
	}
}

template<typename License>
void MainFrame::Check(std::auto_ptr<License> &license) {

	using namespace Licensing;

	const boost::optional<pt::ptime> updateTo = license->GetUpdateTimeTo();
	const std::string licenseStr = license->GetLicense();

	if (updateTo && *updateTo < pt::second_clock::universal_time() + pt::hours(24 * 6)) {
		OnlineKeyRequest request(licenseStr, LicenseState(m_window->GetService()));
		request.Send();
		if (request.TestKey<InfoDlgLicense>()) {
			request.Accept();
			license.reset(new ExeLicense(LicenseState(m_window->GetService())));
		}
	}

	const bool isStandard = license->GetEdition() == Licensing::EDITION_STANDARD;
	const bool isTrial = license->IsTrial();

	if (m_updateCheckerStat.get()) {
		m_updateCheckerStat->licenseKey = isStandard
			?	L"0"
			:	wxString::FromAscii(licenseStr.c_str()).c_str();
		m_updateCheckerStat->isTrialMode = isTrial;
	}

	const bool goPro = isTrial || isStandard;

	if (goPro != (GetMenuBar()->FindItem(CMD_GO_PRO) != 0)) {
		wxMenuBar *const oldMenu = GetMenuBar();
		CreateMenu(goPro);
		delete oldMenu;
		wxToolBar *const oldBar = GetToolBar();
		CreateToolBar(goPro);
		delete oldBar;
	}
	
	license->UpdateCache();
	
	switch (license->GetUnactivityReason()) {
		default:
		case UR_NO:
			break;
		case UR_UPDATE:
			{
				const int answer = wxMessageBox(
					wxT("Currently ") TUNNELEX_NAME_W wxT(" works with limited functionality.")
						wxT(" To remove these restrictions please activate it")
						wxT(" with your license. Would you do it now?"),
					wxT("Product activation needed"),
					wxYES_NO | wxCENTER | wxICON_EXCLAMATION,
					this);
				if (answer == wxYES) {
					LicenseDlg(*m_window, this).ShowModal();
				}
			}
			break;
		case UR_TIME:
			if (	license->IsTrial()
					&& (!wxGetApp().GetConfig().Exists(wxT("/License/TrialLicense"))
						|| wxGetApp().GetConfig().Read(wxT("/License/TrialLicense"))
							!= wxString::FromAscii(license->GetLicense().c_str()))) {
				wxGetApp().GetConfig().Write(
					wxT("/License/TrialLicense"),
					wxString::FromAscii(license->GetLicense().c_str()));
				const int answer = wxMessageBox(
					wxT("Time use of your free trial has come to an end.")
						wxT(" To continue to use the product please purchase")
						wxT(" the professional version of the product.")
						wxT(" Would you go to the order page?"),
					wxT("Free trial has come to an end"),
					wxYES_NO | wxCENTER | wxICON_EXCLAMATION,
					this);
				if (answer == wxYES) {
					wxGetApp().OpenOrderPage();
				}
			}
			break;
	}

}

void MainFrame::OnConnectionToServiceStateChanged(ServiceWindow::Event &event) {

	const bool isConnected = m_window->GetService().IsConnected();

	if (isConnected) {
		using namespace Licensing;
		std::auto_ptr<ExeLicense> license(new ExeLicense(LicenseState(m_window->GetService())));
		while (!license->IsFeatureAvailable(true)) {
			if (LicenseStartDlg(*m_window, this).ShowModal() != wxID_OK) {
				if (!wxGetApp().IsUnlimitedModeActive()) {
					Close();
					return;
				} else {
					break;
				}
			}
			license.reset(new ExeLicense(LicenseState(m_window->GetService())));
		}
		Check(license);
	}

	OnRuleSetChanged(event);

	wxToolBar &toolBar = *GetToolBar();
	wxMenuBar &menu = *GetMenuBar();

	SetConnectionStatus(isConnected ? wxT("connected") : wxT("disconnected"));
	CheckServiceStatus();
	CheckRuleSetStatus();

	menu.Enable(CMD_RULE_RESTORE, isConnected);

	toolBar.EnableTool(CMD_RULE_ADD_CUSTOM, isConnected);
	menu.Enable(CMD_RULE_ADD_CUSTOM, isConnected);
	menu.Enable(CMD_RULE_ADD_CUSTOM_ADVANCED, isConnected);
	menu.Enable(CMD_RULE_ADD_UPNP, isConnected);
	menu.Enable(CMD_RULE_ADD_FTP, isConnected);

	toolBar.EnableTool(CMD_SERVICE_LOG_OPEN_CLOSE, isConnected);
	menu.Enable(CMD_SERVICE_LOG_OPEN_CLOSE, isConnected);

	menu.Enable(CMD_LICENSE_INFO_VIEW, isConnected);

	toolBar.EnableTool(CMD_SERVER_START, isConnected && !m_window->GetService().IsStarted());
	menu.Enable(CMD_SERVER_START, isConnected && !m_window->GetService().IsStarted());
	toolBar.EnableTool(CMD_SERVER_STOP, isConnected && m_window->GetService().IsStarted());
	menu.Enable(CMD_SERVER_STOP, isConnected && m_window->GetService().IsStarted());
	
	menu.Enable(CMD_SERVICE_SSL_SERTIFICATE_LIST, isConnected);

	const bool hasModifiedRules = isConnected ? m_window->HasModifiedRules() : 0; 
	menu.Enable(CMD_SERVICE_CHANGES_APPLY, hasModifiedRules);
	menu.Enable(CMD_SERVICE_CHANGES_CANCEL, hasModifiedRules);

	const size_t selected = isConnected ? m_window->GetSelectedRulesCount() : 0;
	const bool isEditableSelected
		= selected > 0 && m_window->GetEditableSelectedRulesCount() > 0;
	const bool isModifedSelected
		= selected > 0 && m_window->GetModifiedSelectedRulesCount() > 0;
	const bool isDisabledSelected
		= selected > 0 && m_window->GetDisabledSelectedRulesCount() > 0;
	const bool isEnabledSelected
		= selected > 0 && m_window->GetEnabledSelectedRulesCount() > 0;

	menu.Enable(CMD_RULE_CHANGES_APPLY, isModifedSelected);
	menu.Enable(CMD_RULE_CHANGES_CANCEL, isModifedSelected);

	toolBar.EnableTool(CMD_RULE_ENABLE, isDisabledSelected);
	menu.Enable(CMD_RULE_ENABLE, isDisabledSelected);
	toolBar.EnableTool(CMD_RULE_DISABLE, isEnabledSelected);
	menu.Enable(CMD_RULE_DISABLE, isEnabledSelected);

	toolBar.EnableTool(CMD_RULE_CUT, isEditableSelected);
	menu.Enable(CMD_RULE_CUT, isEditableSelected);

	toolBar.EnableTool(CMD_RULE_DELETE, isEditableSelected);
	menu.Enable(CMD_RULE_DELETE, isEditableSelected);

	toolBar.EnableTool(CMD_RULE_EDIT, selected == 1 && isEditableSelected);
	menu.Enable(CMD_RULE_EDIT, selected == 1 && isEditableSelected);

	toolBar.EnableTool(CMD_RULE_COPY, selected > 0);
	menu.Enable(CMD_RULE_COPY, selected > 0);

	toolBar.EnableTool(CMD_RULE_PASTE, isConnected && !m_internalClipboard.IsEmpty());
	menu.Enable(CMD_RULE_PASTE, isConnected && !m_internalClipboard.IsEmpty());

	menu.Enable(CMD_RULE_DELETE_ALL, isConnected && m_window->GetRulesCount() > 0);

	m_window->EnableList(isConnected);

}

void MainFrame::OnClose(wxCloseEvent &closeEvent) {
	
	if (!closeEvent.CanVeto()) {
		closeEvent.Skip();
		return;
	}

	struct Dialogs {
		
		static bool Show(
				wxWindow &parent,
				wxString message,
				const wxString &configVarName) {
		
			const wxString configVarFullName = wxT("/Show/Main/") + configVarName;
			
			bool showThisDialog;
			wxGetApp().GetConfig().Read(configVarFullName, &showThisDialog, true);
			if (!showThisDialog) {
				return true;
			}

			const int border = 10;
			const wxSize buttonSize(75, 25);
		
			message += wxT("\n\nDo you want to close Control Center?");

			wxDialog dlg(
				&parent, wxID_ANY,
				wxT("Control Center closing"),
				wxDefaultPosition, wxDefaultSize,
				wxCAPTION);
			wxStaticText &text = *(new wxStaticText(&dlg, wxID_ANY, message));
			wxCheckBox &checkBox = *(new wxCheckBox(
				&dlg, wxID_ANY, wxT("Do not show this message in the future")));

			wxFont checkBoxFont(checkBox.GetFont());
			checkBoxFont.SetPointSize(7);
			checkBox.SetFont(checkBoxFont);
			text.Wrap(
				checkBox.GetSize().GetWidth()
					+ buttonSize.GetWidth() * 2
					+ border * 4);
			
			dlg.SetClientSize(
				wxSize(
					text.GetBestSize().GetWidth() + border * 2,
					text.GetBestSize().GetHeight()
						+ border * 4
						+ buttonSize.GetHeight()
						+ checkBox.GetSize().GetHeight()));
			
			text.SetPosition(wxPoint(border, border));
			text.SetSize(text.GetBestSize());
			checkBox.SetPosition(
				wxPoint(
					border,
					dlg.GetClientSize().GetHeight()
						- border
						- buttonSize.GetHeight()
						+ checkBox.GetSize().GetHeight() / 2));

			new wxButton(
				&dlg, wxID_OK, wxT("Yes"),
				wxPoint(
					dlg.GetClientSize().GetWidth() - border * 2 - buttonSize.GetWidth() * 2,
					dlg.GetClientSize().GetHeight() - border - buttonSize.GetHeight()),
				buttonSize);
			new wxButton(
				&dlg, wxID_CANCEL, wxT("No"),
				wxPoint(
					dlg.GetClientSize().GetWidth() - border - buttonSize.GetWidth(),
					dlg.GetClientSize().GetHeight() - border - buttonSize.GetHeight()),
				buttonSize);
			
			dlg.ShowModal();
			wxGetApp().GetConfig().Write(configVarFullName, !checkBox.IsChecked());

			return dlg.GetReturnCode() == wxID_OK;
			
		}
		
	};
	
	if (m_window->HasModifiedRules()) {
		const wxChar *const message
			=	wxT("You are closing Control Center,")
				wxT(" but one or more rules still modified and not saved.")
				wxT(" If you do not want to lose these changes,")
				wxT(" please apply it first.");
		if (!Dialogs::Show(*this, message, wxT("ClosingWhenHasNotSavedRules"))) {
			return;
		}
	}
	if (m_window->GetService().IsStarted()) {
		const wxChar *const  message
			=	wxT("After Control Center will be closed,")
				wxT(" the ") TUNNELEX_NAME_W wxT(" service will continue working in the  background.")
				wxT(" If you want to stop it, you should choose")
				wxT(" \"Stop ") TUNNELEX_NAME_W wxT("\" into \"Service\" menu.");
		if (!Dialogs::Show(*this, message, wxT("ClosingWhenServiceIsActive"))) {
			return;
		}
	}
	
	closeEvent.Skip();
	
}

void MainFrame::OnToolEnter(wxCommandEvent &event) {
	wxString toolHelp;
	if (event.GetSelection() > -1) {
		const wxToolBar &toolBar = *GetToolBar();
		toolHelp = toolBar.GetToolShortHelp(event.GetSelection());
	}
	SetStatusText(toolHelp, SBP_COMMON);
}

void MainFrame::OnGoPro(wxCommandEvent &) {
	wxGetApp().OpenOrderPage();
}
