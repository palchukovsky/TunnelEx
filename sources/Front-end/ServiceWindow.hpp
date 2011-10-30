 /**************************************************************************
 *   Created: 2007/11/15 13:40
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__ServerWindow_h__0711151340
#define INCLUDED_FILE__TUNNELEX__ServerWindow_h__0711151340

#include "Rule.hpp"
#include "ServiceAdapter.hpp"
#include "LogDlg.hpp"

class RuleListCtrl;
enum ServiceState;
class LogDlg;

class wxListEvent;

class ServiceWindow : public wxWindow  {

private:

	struct Licenses;

public:

	class Event : public wxNotifyEvent {
	public:
		enum Id {
			ID_LOG_OPEN,
			ID_SERVICE_RUNING_STATE_CHANGED,
			ID_RULE_SET_CHANGED,
			ID_CONNECTION_TO_SERVICE_STATE_CHANGED
		};
		typedef void (wxEvtHandler::*Handler)(ServiceWindow::Event &);
	public:
		explicit Event(wxEventType, Id);
		virtual ~Event();
	public:
		virtual wxEvent * Clone() const;
	};

public:

	explicit ServiceWindow(
			wxWindow *parent,
			wxWindowID id = -1,
			const wxPoint &pos = wxDefaultPosition,
			const wxSize &size = wxDefaultSize,
			const wxString &name = wxT("ServiceWindow"));
	~ServiceWindow();
	
	DECLARE_NO_COPY_CLASS(ServiceWindow)
	DECLARE_EVENT_TABLE()
	
public:

	void OpenServiceLog();
	void CloseServiceLog();

	void ShowSslCertficateList();

	void ClearState();

	void AddCustomRule();
	void AddCustomRuleAdvanced();
	void AddUpnpRule();
	void AddFtpTunnelRule();
	void EditSelectedRule();

	void ApplyChangesForSelectedRules();
	void CancelChangesForSelectedRules();

	void EnableSelectedRules();
	void DisableSelectedRules();

	void DeleteSelectedRules();
	void DeleteAllRules();

	void SelectAllRules();

	size_t ImportRules(const wxString &xml, bool merge);
	size_t ImportRules(const TunnelEx::RuleSet &, bool merge);
	wxString ExportRules() const;
	wxString ExportSelectedRules() const;

	void SortRuleListViewByName();
	void SortRuleListViewByInputs();
	void SortRuleListViewByDestinations();
	void SortRuleListViewByState();
	
	bool HasModifiedRules() const {
		return m_notAppliedRules.size() > 0;
	}
	size_t GetSelectedRulesCount() const;
	size_t GetEnabledRulesCount() const;
	size_t GetEnabledSelectedRulesCount() const;
	bool IsRuleEnabled(const std::wstring &ruleUuid) const;
	size_t GetDisabledSelectedRulesCount() const;
	size_t GetModifiedSelectedRulesCount() const;
	size_t GetEditableSelectedRulesCount() const;
	size_t GetRulesCount() const {
		return m_rules.size();
	}
	size_t GetAddedRulesCount() const;
	size_t GetDeletedRulesCount() const;

	void OnServiceNewErrors(ServiceAdapter::Event &);
	void OnServiceNewWarnings(ServiceAdapter::Event &);
	void OnServiceNewLogRecord(ServiceAdapter::Event &);
	void OnServiceAdapterConnect(ServiceAdapter::Event &);
	void OnServiceAdapterDisconnect(ServiceAdapter::Event &);
	void OnServiceAdapterConnectionFailed(ServiceAdapter::Event &);
	void OnServiceRunningStateChanged(ServiceAdapter::Event &);
	void OnServiceRuleSetModified(ServiceAdapter::Event &);
	void OnLogDlgClose(LogDlg::Event &);
	void OnSelectionChanged(wxListEvent &);

	void ApplyChanges();
	void CancelChanges();

	size_t GetRuleCountStat() const;

	void OnSize(wxSizeEvent &);

	const ServiceAdapter & GetService() const {
		return const_cast<ServiceWindow *>(this)->GetService();
	}
	ServiceAdapter & GetService();
	
	bool IsLogOpened() const {
		return m_logWindows.get() != 0;
	}

	void EnableList(const bool flag);

	bool Activate();
	bool ActivateTrial();

protected:

	void CreateControlRuleList();
	void CreateControlServiceState();

	void SetServiceState(ServiceState);
	void ChooseActualServiceState();

	wxSize GetRuleListCtrlSize() const;
	wxPoint GetRuleListCtrlPosition() const;

	wxSize GetServiceStateCtrlSize() const;
	wxPoint GetServiceStateCtrlPosition() const;

	void RefreshRuleSet();

	void AddRule(const TunnelEx::Rule &);

	void Apply(const TunnelEx::Rule &);
	void Apply(const TunnelEx::RuleSet &);

	void Delete(const RulesUuids &);

private:

	std::auto_ptr<ServiceAdapter> InitService(const wxString &endpoint);
	void ReinitService(const wxString &endpoint);
	void SaveServiceState(const ServiceAdapter &) throw();
	void CheckService();

	void RepaintRuleSet(bool generateRulesListChangeEvent);

	const Licenses & GetLicenses() const;

	void ShowNewRuleEntryPoint(const std::wstring &);

private:

	enum ControlsIds;
	
	std::auto_ptr<ServiceAdapter> m_service;
	
	RulesMap m_rules;
	NotAppliedRulesUuids m_notAppliedRules;
	
	const unsigned char m_borderWidth;
	
	bool m_hasNewErrors;
	bool m_hasNewWarns;

	std::auto_ptr<LogDlg> m_logWindows;

	std::auto_ptr<Licenses> m_licenses;

	std::string m_trialLicenseCache;
	size_t m_trialLicensePeriodCache;
	boost::posix_time::ptime m_trialLicenseCacheTime;

};

DECLARE_EVENT_TYPE(EVT_SERVICE_WINDOW_ACTION, -1);

#define EVT_SERVICE_WINDOW(id, fn) \
	DECLARE_EVENT_TABLE_ENTRY( \
		EVT_SERVICE_WINDOW_ACTION, \
		id, \
		-1, \
		(wxObjectEventFunction)(wxEventFunction) wxStaticCastEvent(ServiceWindow::Event::Handler, &fn), \
		(wxObject *)NULL),

#endif // INCLUDED_FILE__TUNNELEX__ServerWindow_h__0711151340
