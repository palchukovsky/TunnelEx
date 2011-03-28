/**************************************************************************
 *   Created: 2007/11/15 14:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: MainFrame.hpp 1043 2010-10-25 10:22:29Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__MainFrame_h__0711151427
#define INCLUDED_FILE__TUNNELEX__MainFrame_h__0711151427

#include "UpdateChecker.hpp"
#include "ServiceWindow.hpp"

class ServiceWindow;

class MainFrame : public wxFrame {

public:

	MainFrame();
	virtual ~MainFrame();
	
	DECLARE_NO_COPY_CLASS(MainFrame)
	DECLARE_EVENT_TABLE()

public:

	enum Commands {
		CMD_EXIT					= wxID_EXIT,
		CMD_ABOUT					= wxID_ABOUT,
		CMD_HELP_CONTENTS			= wxID_HELP_CONTENTS,
		CMD_RULE_SELECT_ALL			= wxID_SELECTALL,
		CMD_LICENSE_INFO_VIEW		= wxID_HIGHEST,
		CMD_WWW_HOME_PAGE,
		CMD_ISSUE_SUBMIT,
		CMD_BLOG,
		CMD_RULE_ADD_CUSTOM,
		CMD_RULE_ADD_CUSTOM_ADVANCED,
		CMD_RULE_ADD_FTP,
		CMD_RULE_ADD_UPNP,
		CMD_RULE_BACKUP_ALL,
		CMD_RULE_BACKUP_SELECTED,
		CMD_RULE_RESTORE,
		CMD_RULE_ENABLE,
		CMD_RULE_DISABLE,
		CMD_RULE_EDIT,
		CMD_RULE_COPY,
		CMD_RULE_PASTE,
		CMD_RULE_CUT,
		CMD_RULE_CHANGES_APPLY,
		CMD_RULE_CHANGES_CANCEL,
		CMD_RULE_DELETE,
		CMD_RULE_DELETE_ALL,
		CMD_SORT_BY_NAME,
		CMD_SORT_BY_INPUTS,
		CMD_SORT_BY_DESTINATIONS,
		CMD_SORT_BY_STATE,
		CMD_SERVER_START,
		CMD_SERVER_STOP,
		CMD_SERVICE_LOG_OPEN_CLOSE,
		CMD_SERVICE_CHANGES_APPLY,
		CMD_SERVICE_CHANGES_CANCEL,
		CMD_SERVICE_SSL_SERTIFICATE_LIST,
		CMD_GO_PRO
	};

private:

	enum Control {
		CONTROL_TOOLBAR
	};

	enum StatusBarPosition {
		SBP_COMMON = 0,
		SBP_RULES_COUNT, 
		// SBP_ACTIVE_RULES_COUNT, 
		SBP_CONNECTION_STATE,
		SBP_SERVICE_STATE,
		SBP_SIZE
	};

private:

	void CreateStatusBar();
	void CreateMenu(bool goPro);
	void CreateToolBar(bool goPro);
	void BackupRules(const wxString &content);

public:

	void OnServiceRunningStateChanged(ServiceWindow::Event &);
	void OnRuleSetChanged(ServiceWindow::Event &);
	void OnConnectionToServiceStateChanged(ServiceWindow::Event &);

public:

	void OnCmdExit(wxCommandEvent &);

	void OnCmdRuleAddCustom(wxCommandEvent &);
	void OnCmdRuleAddCustomAdvanced(wxCommandEvent &);
	void OnCmdRuleAddUpnp(wxCommandEvent &);
	void OnCmdRuleAddFtp(wxCommandEvent &);
	void OnCmdRuleBackupAll(wxCommandEvent &);
	void OnCmdRuleBackupSelected(wxCommandEvent &);
	void OnCmdRuleRestore(wxCommandEvent &);
	void OnCmdRuleEdit(wxCommandEvent &);
	void OnCmdRuleCopy(wxCommandEvent &);
	void OnCmdRuleCut(wxCommandEvent &);
	void OnCmdRulePaste(wxCommandEvent &);
	void OnCmdRuleChangesApply(wxCommandEvent &);
	void OnCmdRuleChangesCancel(wxCommandEvent &);
	void OnCmdRuleEnable(wxCommandEvent &);
	void OnCmdRuleDisable(wxCommandEvent &);
	void OnCmdRuleDelete(wxCommandEvent &);
	void OnCmdRuleDeleteAll(wxCommandEvent &);
	void OnCmdRuleSelectAll(wxCommandEvent &);

	void OnCmdSortByName(wxCommandEvent &);
	void OnCmdSortByInputs(wxCommandEvent &);
	void OnCmdSortByDestinations(wxCommandEvent &);
	void OnCmdSortByState(wxCommandEvent &);

	void OnCmdHelpContents(wxCommandEvent &);
	void OnCmdWwwHomePage(wxCommandEvent &);
	void OnCmdBlog(wxCommandEvent &);
	void OnCmdAbout(wxCommandEvent &);
	void OnCmdIssueSubmit(wxCommandEvent &);

	void OnCmdServerStart(wxCommandEvent &);
	void OnCmdServerStop(wxCommandEvent &);
	void OnCmdServiceLogShowHide(wxCommandEvent &);

	void OnCmdHandleReleaseInfo(UpdateChecker::Event &);

	void OnCmdServiceChangesApply(wxCommandEvent &);
	void OnCmdServiceChangesCancel(wxCommandEvent &);

	void OnCmdServiceSslCertificateList(wxCommandEvent &);

	void OnCmdLicenseInfoView(wxCommandEvent &);

	void OnToolEnter(wxCommandEvent &);

	void OnGoPro(wxCommandEvent &);

public:

	size_t GetRuleCountStat() const;
	const wxString & GetInternalClipboardContent() const {
		return m_internalClipboard;
	}

protected:

	static wxPoint GetFramePosition();
	static wxSize GetFrameSize();
	
private:

	void OnClose(wxCloseEvent &);

private:

	void SetConnectionStatus(const wxString &);
	void SetServiceStatus(const wxString &);
	void CheckServiceStatus();

	void CheckRuleSetStatus();

	template<typename License>
	void Check(std::auto_ptr<License> &);

private:

	std::auto_ptr<ServiceWindow> m_window;

	std::auto_ptr<UpdateChecker> m_updateChecker;
	bool m_isUpdateCheckThreadLaunched;
	std::auto_ptr<UpdateChecker::Stat> m_updateCheckerStat;

	wxString m_internalClipboard;

};

#endif // INCLUDED_FILE__TUNNELEX__MainFrame_h__0711151427