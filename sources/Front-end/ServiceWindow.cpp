/**************************************************************************
 *   Created: 2007/11/15 13:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "ServiceWindow.hpp"
#include "RuleListCtrl.hpp"
#include "MainFrame.hpp"
#include "LicenseEnterDlg.hpp"
#include "LicenseRestrictionDlg.hpp"
#include "SslCertificateListDlg.hpp"
#include "TunnelRuleShortDlg.hpp"
#include "TunnelRuleDlg.hpp"
#include "ServiceRuleDlg.hpp"
#include "ServiceStateCtrl.hpp"
#include "ServiceState.hpp"
#include "Application.hpp"
#include "LicensePolicies.hpp"
#include "Auto.hpp"

using namespace TunnelEx;
namespace pt = boost::posix_time;

//////////////////////////////////////////////////////////////////////////

enum ServiceWindow::ControlsIds {
	CTRL_ID_RULES_LIST,
	CTRL_ID_SERVER_STATE,
	CTRL_ID_APPLY,
	CTRL_ID_CANCEL
};

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(ServiceWindow, wxWindow)
	EVT_SIZE(ServiceWindow::OnSize)
	EVT_SERVICE_ADAPTER(
		ServiceAdapter::Event::ID_NEW_ERROR,
		ServiceWindow::OnServiceNewErrors)
	EVT_SERVICE_ADAPTER(
		ServiceAdapter::Event::ID_NEW_WARNING,
		ServiceWindow::OnServiceNewWarnings)
	EVT_SERVICE_ADAPTER(
		ServiceAdapter::Event::ID_NEW_LOG_RECORD,
		ServiceWindow::OnServiceNewLogRecord)
	EVT_SERVICE_ADAPTER(
		ServiceAdapter::Event::ID_CONNECTED,
		ServiceWindow::OnServiceAdapterConnect)
	EVT_SERVICE_ADAPTER(
		ServiceAdapter::Event::ID_DISCONNECTED,
		ServiceWindow::OnServiceAdapterDisconnect)
	EVT_SERVICE_ADAPTER(
		ServiceAdapter::Event::ID_CONNECTION_FAILED,
		ServiceWindow::OnServiceAdapterConnectionFailed)
	EVT_SERVICE_ADAPTER(
		ServiceAdapter::Event::ID_SERVICE_RUNNING_STATE_CHANGED,
		ServiceWindow::OnServiceRunningStateChanged)
	EVT_SERVICE_ADAPTER(
		ServiceAdapter::Event::ID_RULE_SET_MODIFIED,
		ServiceWindow::OnServiceRuleSetModified)
	EVT_LOG_DLG(
		LogDlg::Event::ID_CLOSE,
		ServiceWindow::OnLogDlgClose)
	EVT_LIST_ITEM_SELECTED(CTRL_ID_RULES_LIST, ServiceWindow::OnSelectionChanged)
	EVT_LIST_ITEM_DESELECTED(CTRL_ID_RULES_LIST, ServiceWindow::OnSelectionChanged)
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////

struct ServiceWindow::Licenses {

	explicit Licenses(ServiceAdapter &service)
			: licenseKeyRev(0),
			ruleSet(LicenseState(service, licenseKeyRev)) {
		//...//
	}

	long licenseKeyRev;

	Licensing::RuleSetLicense ruleSet;

};

//////////////////////////////////////////////////////////////////////////

ServiceWindow::ServiceWindow(
			wxWindow *parent,
			wxWindowID id,
			const wxPoint &pos,
			const wxSize &size,
			const wxString &name)
		: wxWindow(parent, id, pos, size, wxHW_NO_SELECTION | wxHW_SCROLLBAR_AUTO, name),
		m_borderWidth(10),
		m_hasNewErrors(false),
		m_hasNewWarns(false) {
	CreateControlRuleList();
	CreateControlServiceState();
	SetServiceState(TEX_SERVICE_STATE_CONNECTING);
	Show();
	Enable();
	Update();
	CheckService();
}

ServiceWindow::~ServiceWindow() {
	if (m_service.get()) {
		SaveServiceState(*m_service);
	}
}

wxSize ServiceWindow::GetRuleListCtrlSize() const {
	const wxSize clientSize(GetClientSize());
	return wxSize(
		clientSize.x - (m_borderWidth * 2),
		clientSize.y - GetServiceStateCtrlSize().y - (m_borderWidth * 3));
}

wxPoint ServiceWindow::GetRuleListCtrlPosition() const {
	return wxPoint(
		m_borderWidth,
		(m_borderWidth * 2) + GetServiceStateCtrlSize().y);
}

wxSize ServiceWindow::GetServiceStateCtrlSize() const {
	return wxSize(
		GetClientSize().x - (m_borderWidth * 2),
		40);
}

wxPoint ServiceWindow::GetServiceStateCtrlPosition() const {
	return wxPoint(m_borderWidth, m_borderWidth);
}

std::auto_ptr<ServiceAdapter> ServiceWindow::InitService(const wxString &endpoint) {
	return std::auto_ptr<ServiceAdapter>(
		new ServiceAdapter(
			*this,
			endpoint,
			wxGetApp().GetConfig().Read(wxT("/LastKnownLogError"), long(0)),
			wxGetApp().GetConfig().Read(wxT("/LastKnownLogWarn"), long(0))));
}

void ServiceWindow::ReinitService(const wxString &endpoint) {
	m_service->Reconnect(
		*this,
		endpoint,
		wxGetApp().GetConfig().Read(wxT("/LastKnownLogError"), long(0)),
		wxGetApp().GetConfig().Read(wxT("/LastKnownLogWarn"), long(0)));
}

void ServiceWindow::SaveServiceState(const ServiceAdapter &service) throw() {
	try {
		wxGetApp().GetConfig().Write(
			wxT("/LastKnownLogError"),
			service.GetLastKnownErrorCount());
	} catch (...) {
		//...//
	}
	try {
		wxGetApp().GetConfig().Write(
			wxT("/LastKnownLogWarn"),
			service.GetLastKnownWarnCount());
	} catch (...) {
		//...//
	}
}

ServiceAdapter & ServiceWindow::GetService() {
	CheckService();
	return *m_service;
}

const ServiceWindow::Licenses & ServiceWindow::GetLicenses() const {
	if (!m_licenses.get()) {
		const_cast<ServiceWindow *>(this)->m_licenses.reset(
			new Licenses(const_cast<ServiceWindow *>(this)->GetService()));
	}
	return *m_licenses;
}

void ServiceWindow::CheckService() {
	if (m_service.get() && m_service->IsConnected()) {
		return;
	}
	wxString serviceEndpoint;
	wxGetApp().GetServiceEndpoint(serviceEndpoint);
	if (!m_service.get()) {
		m_licenses.reset();
		m_service = InitService(serviceEndpoint);
	} else if (serviceEndpoint != m_service->GetEndpoint()) {
		SaveServiceState(*m_service);
		m_licenses.reset();
		ReinitService(serviceEndpoint);
	}
}

void ServiceWindow::OnServiceRuleSetModified(ServiceAdapter::Event &) {
	const ServiceState currentState
		= boost::polymorphic_downcast<ServiceStateCtrl *>(FindWindow(CTRL_ID_SERVER_STATE))
			->GetStateCode();
	if (currentState == TEX_SERVICE_STATE_CHANGED) {
		ChooseActualServiceState();
	}
	RefreshRuleSet();
	ServiceWindow::Event event(
		EVT_SERVICE_WINDOW_ACTION,
		Event::ID_RULE_SET_CHANGED);
	wxPostEvent(GetParent(), event);
}

namespace {

	template<class RuleSet>
	void MergeRuleSet(
				const RuleSet &serviceRuleSet,
				const RulesMap &currentRuleSet,
				NotAppliedRulesUuids &notAppliedRules,
				RulesMap &destionationRuleSet) {
		const size_t serviceRulesNumb = serviceRuleSet.GetSize();
		for (size_t i = 0; i < serviceRulesNumb; ++i) {
			const typename RuleSet::ItemType &serviceRule = serviceRuleSet[i];
			std::auto_ptr<RuleSet::ItemType> insertRule;
			const std::wstring id = serviceRule.GetUuid().GetCStr();
			const NotAppliedRulesUuids::iterator posInNotAppliedRules
				= notAppliedRules.find(id);
			if (posInNotAppliedRules == notAppliedRules.end()) {
				insertRule.reset(new typename RuleSet::ItemType(serviceRule));
			} else {
				const RulesMap::const_iterator pos = currentRuleSet.find(id);
				assert(pos != currentRuleSet.end());
				assert(IsRule<RuleSet::ItemType>(pos->second));
				if (	pos != currentRuleSet.end()
						&& IsRule<RuleSet::ItemType>(pos->second)) {
					insertRule.reset(
						new typename RuleSet::ItemType(
							*boost::polymorphic_downcast<const RuleSet::ItemType *>(pos->second)));
					insertRule->Enable(serviceRule.IsEnabled());
				} else {
					insertRule.reset(new typename RuleSet::ItemType(serviceRule));
				}
				notAppliedRules.erase(posInNotAppliedRules);
			}
			destionationRuleSet.insert(id, insertRule);
		}
	}

}


void ServiceWindow::RefreshRuleSet() {
	
	RuleSet newServiceRules;
	GetService().GetRuleSet(newServiceRules);

	NotAppliedRulesUuids tmpNotAppliedRules(m_notAppliedRules);
	RulesMap newRuleSet;
	MergeRuleSet(newServiceRules.GetServices(), m_rules, tmpNotAppliedRules, newRuleSet);
	MergeRuleSet(newServiceRules.GetTunnels(), m_rules, tmpNotAppliedRules, newRuleSet);

	foreach (const NotAppliedRulesUuids::value_type &r, tmpNotAppliedRules) {
		const RulesMap::const_iterator pos = m_rules.find(r.first);
		assert(pos != m_rules.end());
		if (pos != m_rules.end()) {
			std::auto_ptr<Rule> rule = Clone(pos->second);
			newRuleSet.insert(r.first, rule);
		}
	}

	m_rules.swap(newRuleSet);

	RepaintRuleSet(true);

}


void ServiceWindow::RepaintRuleSet(bool generateRulesListChangeEvent) {
	RuleListCtrl &ruleListCtrl
		= *boost::polymorphic_downcast<RuleListCtrl *>(FindWindow(CTRL_ID_RULES_LIST));
	ruleListCtrl.RefreshList(GetService().IsStarted());
	if (generateRulesListChangeEvent) {
		ServiceWindow::Event event(
			EVT_SERVICE_WINDOW_ACTION,
			Event::ID_RULE_SET_CHANGED);
		wxPostEvent(GetParent(), event);
	}
}

void ServiceWindow::OnSelectionChanged(wxListEvent &) {
	ServiceWindow::Event event(
		EVT_SERVICE_WINDOW_ACTION,
		Event::ID_RULE_SET_CHANGED);
	wxPostEvent(GetParent(), event);
}

void ServiceWindow::AddCustomRule() {
	std::auto_ptr<TunnelRule> newRule;
	TunnelRuleShortDlg simpleDlg(*this, this);
	if (simpleDlg.ShowModal() == wxID_OK) {
		newRule.reset(
			new TunnelRule(
				const_cast<const TunnelRuleShortDlg &>(simpleDlg).GetRule()));
	}
	if (simpleDlg.IsAdvancdeMode()) {
		std::auto_ptr<TunnelRuleDlg> fullDlg(!newRule.get()
			?	new TunnelRuleDlg(*this, this, simpleDlg.IsFtp())
			:	new TunnelRuleDlg(*this, this, *newRule));
		if (fullDlg->ShowModal() == wxID_OK) {
			newRule.reset(
				new TunnelRule(
					const_cast<const TunnelRuleDlg &>(*fullDlg).GetRule()));
		}
	}
	if (newRule.get()) {
		AddRule(*newRule);
	}
}

void ServiceWindow::AddCustomRuleAdvanced() {
	TunnelRuleDlg ruleDlg(*this, this, false);
	if (ruleDlg.ShowModal() == wxID_OK) {
		AddRule(const_cast<const TunnelRuleDlg &>(ruleDlg).GetRule());
	}
}

void ServiceWindow::AddUpnpRule() {
	UpnpServiceRuleDlg ruleDlg(*this, this);
	if (ruleDlg.ShowModal() == wxID_OK) {
		AddRule(const_cast<const UpnpServiceRuleDlg &>(ruleDlg).GetRule());
	}
}

void ServiceWindow::AddFtpTunnelRule() {
	TunnelRuleDlg ruleDlg(*this, this, true);
	if (ruleDlg.ShowModal() == wxID_OK) {
		AddRule(const_cast<const TunnelRuleDlg &>(ruleDlg).GetRule());
	}
}

void ServiceWindow::AddRule(const Rule &rule) {
	std::auto_ptr<Rule> newRule = Clone(rule);
	if (	!GetLicenses().ruleSet.IsFeatureAvailable(GetEnabledRulesCount() + 1)
			&& !wxGetApp().IsUnlimitedModeActive()) {
		newRule->Enable(false);
	}
	assert(m_rules.find(newRule->GetUuid().GetCStr()) == m_rules.end());
	Apply(*newRule);
}

void ServiceWindow::EditSelectedRule() {
	
	 RuleListCtrl &ruleListCtrl
		 = *boost::polymorphic_downcast<RuleListCtrl *>(FindWindow(CTRL_ID_RULES_LIST));

	const Rule *const selectedRule = ruleListCtrl.GetFirstSelectedRule();
	if (!selectedRule) {
		return;
	}
	
	std::auto_ptr<Rule> selectedRuleCopy = Clone(selectedRule);
	if (IsTunnel(selectedRuleCopy.get())) {
		const TunnelRule *rule
			= boost::polymorphic_downcast<TunnelRule *>(selectedRuleCopy.get());
		if (!TunnelRuleShortDlg::CheckRule(*rule)) {
			TunnelRuleDlg ruleDlg(*this, this, *rule);
			if (ruleDlg.ShowModal() != wxID_OK) {
				return;
			}
			selectedRuleCopy.reset(
				new TunnelRule(
					const_cast<const TunnelRuleDlg &>(ruleDlg).GetRule()));
		} else {
			TunnelRuleShortDlg simpleDlg(*this, this, *rule);
			const bool isShortDlgOk = simpleDlg.ShowModal() == wxID_OK;
			if (isShortDlgOk) {
				selectedRuleCopy.reset(
					new TunnelRule(
						const_cast<const TunnelRuleShortDlg &>(simpleDlg).GetRule()));
				rule = boost::polymorphic_downcast<TunnelRule *>(selectedRuleCopy.get());
			}
			if (simpleDlg.IsAdvancdeMode()) {
				TunnelRuleDlg fullDlg(*this, this, *rule);
				if (fullDlg.ShowModal() == wxID_OK) {
					selectedRuleCopy.reset(
						new TunnelRule(
							const_cast<const TunnelRuleDlg &>(fullDlg).GetRule()));
				}
			} else if (!isShortDlgOk) {
				return;
			}
		}
	} else {
		assert(IsService(selectedRuleCopy.get()));
		selectedRuleCopy = ServiceRuleDlg::EditRule(
			*this,
			*this,
			*boost::polymorphic_downcast<ServiceRule *>(selectedRuleCopy.get()));
		if (!selectedRuleCopy.get()) {
			return;
		}
	}

	const WString id = selectedRuleCopy->GetUuid();
	const RulesMap::iterator pos(m_rules.find(id.GetCStr()));
	assert(pos != m_rules.end());
	if (pos != m_rules.end()) {
		if (m_notAppliedRules.find(id.GetCStr()) != m_notAppliedRules.end()) {
			RulesMap rules;
			rules = m_rules.clone();
			const RulesMap::iterator pos(rules.find(id.GetCStr()));
			assert(pos != rules.end());
			rules.erase(pos);
			rules.insert(id.GetCStr(), selectedRuleCopy);
			m_rules.swap(rules);
			ChooseActualServiceState();
			RepaintRuleSet(true);
		} else {
			Apply(*selectedRuleCopy);
		}
	}

}

void ServiceWindow::EnableSelectedRules() {

	RulesUuids selectedUuids;
	boost::polymorphic_downcast<RuleListCtrl *>(FindWindow(CTRL_ID_RULES_LIST))
		->GetSelected(selectedUuids);
	
	{
		size_t rulesCountToEnable = 0;
		foreach (const std::wstring &ruleUuid, selectedUuids) {
			if (!IsRuleEnabled(ruleUuid)) {
				++rulesCountToEnable;
			}
		}
		if (rulesCountToEnable > 0) {
			rulesCountToEnable += GetEnabledRulesCount();
			if (!GetLicenses().ruleSet.IsFeatureAvailable(rulesCountToEnable)) {
				LicenseRestrictionDlg(*this, this, GetLicenses().ruleSet, true).ShowModal();
			}
		}
	}

	GetService().EnableRules(selectedUuids);

}

void ServiceWindow::DisableSelectedRules() {
	RulesUuids selectedUuids;
	boost::polymorphic_downcast<RuleListCtrl *>(FindWindow(CTRL_ID_RULES_LIST))
		->GetSelected(selectedUuids);
	GetService().DisableRules(selectedUuids);
}

void ServiceWindow::DeleteSelectedRules() {
	const int answer = wxMessageBox(
		wxT("Delete selected rules?"),
		wxT("Confirm delete"),
		wxYES_NO | wxNO_DEFAULT | wxCENTER | wxICON_WARNING,
		this);
	if (answer != wxYES) {
		return;
	}

	RulesUuids selectedUuids;
	NotAppliedRulesUuids tmpNotAppliedRules(m_notAppliedRules);
	RulesUuids deleteRules;
	boost::polymorphic_downcast<RuleListCtrl *>(FindWindow(CTRL_ID_RULES_LIST))
		->GetSelected(selectedUuids);
	foreach (const RulesUuids::value_type &id, selectedUuids) {
		const NotAppliedRulesUuids::iterator notAppliedPos
			= tmpNotAppliedRules.find(id);
		const bool isModified = notAppliedPos != tmpNotAppliedRules.end();
		if (!isModified) {
			deleteRules.insert(id);
		} else if (notAppliedPos->second == NARS_ADDED) {
			tmpNotAppliedRules.erase(notAppliedPos);
		} else {
			tmpNotAppliedRules[id] = NARS_DELETED;
		}
	}
	ChooseActualServiceState();
	RepaintRuleSet(true);
	tmpNotAppliedRules.swap(m_notAppliedRules);
	if (deleteRules.size() > 0) {
		Delete(deleteRules);
	}
}

void ServiceWindow::DeleteAllRules() {
	const int answer = wxMessageBox(
		wxT("Delete ALL rules in the rule set?"),
		wxT("Confirm delete"),
		wxYES_NO | wxNO_DEFAULT | wxCENTER | wxICON_WARNING,
		this);
	if (answer != wxYES) {
		return;
	}
	NotAppliedRulesUuids tmpNotAppliedRules(m_notAppliedRules);
	RulesUuids deleteRules;
	foreach (const RulesMap::value_type &r, m_rules) {
		const std::wstring &id = r.first;
		const NotAppliedRulesUuids::iterator notAppliedPos
			= tmpNotAppliedRules.find(id);
		const bool isModified = notAppliedPos != tmpNotAppliedRules.end();
		if (!isModified) {
			deleteRules.insert(id);
		} else if (notAppliedPos->second == NARS_ADDED) {
			tmpNotAppliedRules.erase(notAppliedPos);
		} else {
			tmpNotAppliedRules[id] = NARS_DELETED;
		}
	}
	ChooseActualServiceState();
	RepaintRuleSet(true);
	tmpNotAppliedRules.swap(m_notAppliedRules);
	if (deleteRules.size() > 0) {
		Delete(deleteRules);
	}
}

void ServiceWindow::SelectAllRules() {
	boost::polymorphic_downcast<RuleListCtrl *>(FindWindow(CTRL_ID_RULES_LIST))
		->SelectAll();
}

void ServiceWindow::SortRuleListViewByName() {
	boost::polymorphic_downcast<RuleListCtrl *>(FindWindow(CTRL_ID_RULES_LIST))
		->SortByName();
}

void ServiceWindow::SortRuleListViewByInputs() {
	boost::polymorphic_downcast<RuleListCtrl *>(FindWindow(CTRL_ID_RULES_LIST))
		->SortByInputs();
}

void ServiceWindow::SortRuleListViewByDestinations() 	{
	boost::polymorphic_downcast<RuleListCtrl *>(FindWindow(CTRL_ID_RULES_LIST))
		->SortByDestinations();
}

void ServiceWindow::SortRuleListViewByState() 	{
	boost::polymorphic_downcast<RuleListCtrl *>(FindWindow(CTRL_ID_RULES_LIST))
		->SortByState();
}

void ServiceWindow::ChooseActualServiceState() {
	const ServiceState state = !GetService().IsConnected()
		?	TEX_SERVICE_STATE_UNKNOWN
		:	m_notAppliedRules.size() > 0
			?	TEX_SERVICE_STATE_CHANGED
			:	m_hasNewErrors
				?	TEX_SERVICE_STATE_ERROR
				:	m_hasNewWarns
					?	TEX_SERVICE_STATE_WARNING
					:	GetService().IsStarted()
						?	TEX_SERVICE_STATE_STARTED
						:	TEX_SERVICE_STATE_STOPPED;
	SetServiceState(state);
}

void ServiceWindow::SetServiceState(ServiceState state) {
	wxString description;
	switch (state) {
		case TEX_SERVICE_STATE_STARTED:
			description = TUNNELEX_NAME_W wxT(" is on-line");
			break;
		case TEX_SERVICE_STATE_STOPPED:
			description = TUNNELEX_NAME_W wxT(" is off-line, all rules deactivated");
			break;
		case TEX_SERVICE_STATE_ERROR:
		case TEX_SERVICE_STATE_WARNING: 
			description = wxT("Warning, ") TUNNELEX_NAME_W wxT(" working with errors, see log for details");
			break;
		case TEX_SERVICE_STATE_CHANGED: 
			description = wxT("Some rules has been modified, but not saved");
			break;
		case TEX_SERVICE_STATE_CONNECTING:
			description = wxT("Connecting to ") TUNNELEX_NAME_W wxT(", please wait...");
			break;
		default:
			description = wxT("Connection lost, ") TUNNELEX_NAME_W wxT(" status is unknown");
			break;
	}
	boost::polymorphic_downcast<ServiceStateCtrl *>(FindWindow(CTRL_ID_SERVER_STATE))
			->SetState(state, description);
}

void ServiceWindow::OpenServiceLog() {

	assert(!m_logWindows.get());
	if (m_logWindows.get()) {
		return;
	}

	m_hasNewErrors = m_hasNewWarns = false;
	ChooseActualServiceState();
	m_logWindows.reset(new  LogDlg(this, wxID_ANY));
	m_logWindows->Show(true);

	Event event(EVT_SERVICE_WINDOW_ACTION, Event::ID_LOG_OPEN);
	wxPostEvent(GetParent(), event);

}

void ServiceWindow::CloseServiceLog() {
	assert(m_logWindows.get());
	m_logWindows.reset();
}

void ServiceWindow::ShowSslCertficateList() {
	SslCertificateListDlg(*this, this).ShowModal();
}

void ServiceWindow::OnServiceNewErrors(ServiceAdapter::Event &) {
	if (IsLogOpened()) {
		return;
	}
	m_hasNewWarns = false;
	m_hasNewErrors = true;
	const ServiceState currentState
		= boost::polymorphic_downcast<ServiceStateCtrl *>(FindWindow(CTRL_ID_SERVER_STATE))
			->GetStateCode();
	switch (currentState) {
		case TEX_SERVICE_STATE_CONNECTING:
		case TEX_SERVICE_STATE_CHANGED:
		case TEX_SERVICE_STATE_ERROR:
			return;
	}
	SetServiceState(TEX_SERVICE_STATE_ERROR);
	boost::polymorphic_cast<wxFrame *>(GetParent())
		->RequestUserAttention(wxUSER_ATTENTION_ERROR);
}

void ServiceWindow::OnServiceNewWarnings(ServiceAdapter::Event &) {
	if (IsLogOpened()) {
		return;
	}
	if (!m_hasNewErrors) {
		m_hasNewWarns = true;
	}
	const ServiceState currentState
		= boost::polymorphic_downcast<ServiceStateCtrl *>(FindWindow(CTRL_ID_SERVER_STATE))
			->GetStateCode();
	switch (currentState) {
		case TEX_SERVICE_STATE_CONNECTING:
		case TEX_SERVICE_STATE_CHANGED:
		case TEX_SERVICE_STATE_ERROR:
		case TEX_SERVICE_STATE_WARNING:
			return;
	}
	SetServiceState(TEX_SERVICE_STATE_WARNING);
	boost::polymorphic_cast<wxFrame *>(GetParent())
		->RequestUserAttention(wxUSER_ATTENTION_ERROR);
}

void ServiceWindow::OnServiceNewLogRecord(ServiceAdapter::Event &event) {
	if (m_logWindows.get()) {
		wxPostEvent(m_logWindows.get(), event);
	}
}

void ServiceWindow::OnServiceAdapterConnect(ServiceAdapter::Event &) {
	ChooseActualServiceState();
	std::list<texs__NetworkAdapterInfo> serviceNetworkAdapters;
	GetService().GetNetworkAdapters(true, serviceNetworkAdapters);
	ServiceWindow::Event event(
		EVT_SERVICE_WINDOW_ACTION,
		Event::ID_CONNECTION_TO_SERVICE_STATE_CHANGED);
	wxPostEvent(GetParent(), event);
}

void ServiceWindow::OnServiceAdapterDisconnect(ServiceAdapter::Event &) {
	ChooseActualServiceState();
	ServiceWindow::Event event(
		EVT_SERVICE_WINDOW_ACTION,
		Event::ID_CONNECTION_TO_SERVICE_STATE_CHANGED);
	wxPostEvent(GetParent(), event);
	boost::polymorphic_cast<wxFrame *>(GetParent())
		->RequestUserAttention(wxUSER_ATTENTION_ERROR);
}

void ServiceWindow::OnServiceAdapterConnectionFailed(ServiceAdapter::Event &) {
	const ServiceState currentState
		= boost::polymorphic_downcast<ServiceStateCtrl *>(FindWindow(CTRL_ID_SERVER_STATE))
			->GetStateCode();
	if (currentState == TEX_SERVICE_STATE_CONNECTING) {
		ChooseActualServiceState();
		RefreshRuleSet();
	}
	CheckService();
}

void ServiceWindow::OnServiceRunningStateChanged(ServiceAdapter::Event &) {
	{
		std::list<texs__NetworkAdapterInfo> serviceNetworkAdapters;
		GetService().GetNetworkAdapters(true, serviceNetworkAdapters);
	}
	RepaintRuleSet(false);
	ServiceWindow::Event myEvent(
		EVT_SERVICE_WINDOW_ACTION,
		Event::ID_SERVICE_RUNING_STATE_CHANGED);
	wxPostEvent(GetParent(), myEvent);
	const ServiceState currentState
		= boost::polymorphic_downcast<ServiceStateCtrl *>(FindWindow(CTRL_ID_SERVER_STATE))
			->GetStateCode();
	switch (currentState) {
		case TEX_SERVICE_STATE_CONNECTING:
		case TEX_SERVICE_STATE_CHANGED:
		case TEX_SERVICE_STATE_ERROR:
		case TEX_SERVICE_STATE_WARNING:
			return;
	}
	SetServiceState(
		GetService().IsStarted() ? TEX_SERVICE_STATE_STARTED : TEX_SERVICE_STATE_STOPPED);
}

void ServiceWindow::ClearState() {
	m_hasNewErrors = m_hasNewWarns = false;
	ChooseActualServiceState();
}

void ServiceWindow::Apply(const Rule &rule) {
	RuleSet ruleSet;
	Append(&rule, ruleSet);
	Apply(ruleSet);
}

void ServiceWindow::Apply(const RuleSet &ruleSet) {
	GetService().UpdateRules(ruleSet);	
}

void ServiceWindow::ApplyChanges() {
	try {
		RuleSet changedRules;
		RulesUuids deletedRules;
		foreach (const NotAppliedRulesUuids::value_type &r, m_notAppliedRules) {
			if (r.second != NARS_DELETED) {
				const RulesMap::const_iterator pos = m_rules.find(r.first);
				assert(pos != m_rules.end());
				if (pos != m_rules.end()) {
					Append(pos->second, changedRules);
				}
			} else {
				deletedRules.insert(r.first);
			}
		}
		assert(changedRules.GetSize() || deletedRules.size());
		NotAppliedRulesUuids().swap(m_notAppliedRules);
		if (changedRules.GetSize() > 0 || deletedRules.size() > 0) {
			if (deletedRules.size() > 0) {
				Delete(deletedRules);
			}
			if (changedRules.GetSize() > 0) {
				Apply(changedRules);
			}
		}
		ChooseActualServiceState();
	} catch (const ::TunnelEx::LocalException &ex) {
		::wxLogError(ex.GetWhat());
	}
}

void ServiceWindow::Delete(const RulesUuids &ruleSet) {
	GetService().DeleteRules(ruleSet);
}

void ServiceWindow::CancelChanges() {
	NotAppliedRulesUuids().swap(m_notAppliedRules);
	ChooseActualServiceState();
	RefreshRuleSet();
}

void ServiceWindow::ApplyChangesForSelectedRules() {

	NotAppliedRulesUuids tmpNotAppliedRules(m_notAppliedRules);
	RulesUuids selectedUuids;
	boost::polymorphic_downcast<RuleListCtrl *>(FindWindow(CTRL_ID_RULES_LIST))
		->GetSelected(selectedUuids);
	RuleSet changedRules;
	RulesUuids deletedRules;
	const RulesUuids::const_iterator end = selectedUuids.end();
	foreach (const RulesUuids::value_type &id, selectedUuids) {
		const NotAppliedRulesUuids::iterator notAppliedPos
			= tmpNotAppliedRules.find(id);
		if (notAppliedPos != tmpNotAppliedRules.end()) {
			if (notAppliedPos->second != NARS_DELETED) {
				const RulesMap::const_iterator rulePos = m_rules.find(id);
				assert(rulePos != m_rules.end());
				if (rulePos != m_rules.end()) {
					Append(rulePos->second, changedRules);
				}
			} else {
				deletedRules.insert(notAppliedPos->first);
			}
			tmpNotAppliedRules.erase(notAppliedPos);
		}
	}

	assert(changedRules.GetSize() || deletedRules.size());
	m_notAppliedRules.swap(tmpNotAppliedRules);
	if (changedRules.GetSize() || deletedRules.size()) {
		if (deletedRules.size()) {
			GetService().DeleteRules(deletedRules);
		}
		if (changedRules.GetSize()) {
			GetService().UpdateRules(changedRules);
		}
	} else {
		ChooseActualServiceState();
	}
		
}

void ServiceWindow::CancelChangesForSelectedRules() {
	NotAppliedRulesUuids tmpNotAppliedRules(m_notAppliedRules);
	RulesUuids selectedUuids;
	boost::polymorphic_downcast<RuleListCtrl *>(FindWindow(CTRL_ID_RULES_LIST))
		->GetSelected(selectedUuids);
	const RulesUuids::const_iterator end = selectedUuids.end();
	for (RulesUuids::const_iterator i = selectedUuids.begin(); i != end; ++i) {
		const NotAppliedRulesUuids::iterator pos = tmpNotAppliedRules.find(*i);
		if (pos != tmpNotAppliedRules.end()) {
			tmpNotAppliedRules.erase(pos);
		}
	}
	m_notAppliedRules.swap(tmpNotAppliedRules);
	RefreshRuleSet();
}

void ServiceWindow::CreateControlRuleList() {
	new RuleListCtrl(
		m_rules,
		m_notAppliedRules,
		this,
		CTRL_ID_RULES_LIST,
		GetRuleListCtrlPosition(),
		GetRuleListCtrlSize(),
		wxLC_REPORT | wxLC_HRULES | wxLC_VRULES);
}

void ServiceWindow::CreateControlServiceState() {
	new ServiceStateCtrl(
		this,
		CTRL_ID_SERVER_STATE,
		GetServiceStateCtrlPosition(),
		GetServiceStateCtrlSize());
}

void ServiceWindow::OnSize(wxSizeEvent &event) {
	FindWindow(CTRL_ID_SERVER_STATE)->SetSize(GetServiceStateCtrlSize());
	FindWindow(CTRL_ID_RULES_LIST)->SetSize(GetRuleListCtrlSize());
	event.Skip();
}

size_t ServiceWindow::GetRuleCountStat() const {
	return GetRulesCount();
}

void ServiceWindow::OnLogDlgClose(LogDlg::Event &) {
	m_logWindows.reset();
}

size_t ServiceWindow::ImportRules(const wxString &xml, bool merge) {
	RuleSet importedRules;
	try {
		RuleSet(xml.c_str()).Swap(importedRules);
	} catch (const ::TunnelEx::LocalException &ex) {
		::wxLogError(ex.GetWhat());
		return 0;
	}
	return ImportRules(importedRules, merge);
}

namespace {


	bool CheckImportNameExists(
				const RulesMap &rules,
				const WString &nameToCheck) {
		foreach (const RulesMap::const_iterator::value_type r, rules) {
			if (r.second->GetName() == nameToCheck) {
				return true;
			}
		}
		return false;
	}

	void CheckImportName(
				Rule &rule,
				const RulesMap &rules,
				const wchar_t *const newNameTemplate) {
		const WString oldName = rule.GetName();
		if (!CheckImportNameExists(rules, oldName)) {
			return;
		}
		unsigned int count = 2;
		do {
			WFormat newName(newNameTemplate);
			newName % oldName.GetCStr() % count++;
			rule.SetName(newName.str().c_str());
		} while (CheckImportNameExists(rules, rule.GetName()));
	}

	template<class RuleSet>
	void ImportWithMerge(
				const typename RuleSet &importedRules,
				RulesMap &rules,
				NotAppliedRulesUuids &notAppliedRules) {
		for (size_t i = 0; i < importedRules.GetSize(); ++i) {
			const RulesMap::iterator pos
				= rules.find(importedRules[i].GetUuid().GetCStr());
			if (pos == rules.end()) {
				std::auto_ptr<RuleSet::ItemType> rule(
					new typename RuleSet::ItemType(importedRules[i]));
				CheckImportName(*rule, rules, L"%1% (%2%)");
				rules.insert(rule->GetUuid().GetCStr(), rule);
				notAppliedRules[importedRules[i].GetUuid().GetCStr()] = NARS_ADDED;
			} else {
				*pos->second = importedRules[i];
				notAppliedRules[importedRules[i].GetUuid().GetCStr()] = NARS_MODIFIED;
			}
		}
	}

	template<class RuleSet>
	void ImportWithAdd(
				const typename RuleSet &importedRules,
				RulesMap &rules,
				NotAppliedRulesUuids &notAppliedRules) {
		for (size_t i = 0; i < importedRules.GetSize(); ++i) {
			std::auto_ptr<RuleSet::ItemType> rule(
				new typename RuleSet::ItemType(importedRules[i].MakeCopy()));
			CheckImportName(*rule, rules, L"%1% (%2%)");
			const WString id = rule->GetUuid();
			rules.insert(id.GetCStr(), rule);
			notAppliedRules[id.GetCStr()] = NARS_ADDED;
		}
	}

}

size_t ServiceWindow::ImportRules(const RuleSet &importedRules, bool merge) {

	if (!importedRules.GetSize()) {
		return 0;
	}

	RulesMap rules;
	rules = m_rules.clone();
	NotAppliedRulesUuids notAppliedRules(m_notAppliedRules);
	if (merge) {
		ImportWithMerge(importedRules.GetServices(), rules, notAppliedRules);
		ImportWithMerge(importedRules.GetTunnels(), rules, notAppliedRules);
	} else {
		ImportWithAdd(importedRules.GetServices(), rules, notAppliedRules);
		ImportWithAdd(importedRules.GetTunnels(), rules, notAppliedRules);
	}

	rules.swap(m_rules);
	notAppliedRules.swap(m_notAppliedRules);

	ChooseActualServiceState();
	RepaintRuleSet(true);
		
	return importedRules.GetSize();

}

wxString ServiceWindow::ExportRules() const {
	WString buffer;
	RuleSet rulesToExport;
	foreach (const RulesMap::const_iterator::value_type r, m_rules) {
		Append(r.second, rulesToExport);
	}
	rulesToExport.GetXml(buffer);
	return wxString(buffer.GetCStr());
}

wxString ServiceWindow::ExportSelectedRules() const {
	RulesUuids selectedUuids;
	boost::polymorphic_downcast<RuleListCtrl *>(FindWindow(CTRL_ID_RULES_LIST))
		->GetSelected(selectedUuids);
	RuleSet rulesToExport;
	foreach (const RulesUuids::value_type &id, selectedUuids) {
		const RulesMap::const_iterator pos = m_rules.find(id);
		assert(pos != m_rules.end());
		if (pos == m_rules.end()) {
			continue;
		}
		Append(pos->second, rulesToExport);
	}
	WString buffer;
	rulesToExport.GetXml(buffer);
	return wxString(buffer.GetCStr());
}

size_t ServiceWindow::GetSelectedRulesCount() const {
	return boost::polymorphic_downcast<RuleListCtrl *>(FindWindow(CTRL_ID_RULES_LIST))
		->GetSelectedItemCount();
}

size_t ServiceWindow::GetEnabledRulesCount() const {
	size_t result = 0;
	foreach (const RulesMap::const_iterator::value_type &rule, m_rules) {
		if (IsRuleEnabled(rule.first)) {
			++result;
		}
	}
	return result;
}

size_t ServiceWindow::GetEnabledSelectedRulesCount() const {
	size_t result = 0;
	RulesUuids selectedUuids;
	RuleListCtrl &ctrl
		= *boost::polymorphic_downcast<RuleListCtrl *>(FindWindow(CTRL_ID_RULES_LIST));
	ctrl.GetSelected(selectedUuids);
	foreach (const std::wstring &selectedRuleUuid, selectedUuids) {
		if (IsRuleEnabled(selectedRuleUuid)) {
			++result;
		}
	}
	return result;
}

bool ServiceWindow::IsRuleEnabled(const std::wstring &ruleUuid) const {
	const RulesMap::const_iterator rule = m_rules.find(ruleUuid);
	assert(rule != m_rules.end());
	if (rule != m_rules.end() && rule->second->IsEnabled()) {
		const NotAppliedRulesUuids::const_iterator notAppliedPos
			= m_notAppliedRules.find(ruleUuid);
		if (	notAppliedPos == m_notAppliedRules.end()
				|| notAppliedPos->second != NARS_ADDED) {
			return true;
		}
	}
	return false;
}

size_t ServiceWindow::GetDisabledSelectedRulesCount() const {
	size_t result = 0;
	RulesUuids selectedUuids;
	RuleListCtrl &ctrl
		= *boost::polymorphic_downcast<RuleListCtrl *>(FindWindow(CTRL_ID_RULES_LIST));
	ctrl.GetSelected(selectedUuids);
	const NotAppliedRulesUuids::const_iterator notAppliedEnd = m_notAppliedRules.end();
	foreach (const RulesUuids::value_type &id, selectedUuids) {
		const RulesMap::const_iterator rule = m_rules.find(id);
		assert(rule != m_rules.end());
		if (rule != m_rules.end() && !rule->second->IsEnabled()) {
			const NotAppliedRulesUuids::const_iterator notAppliedPos
				= m_notAppliedRules.find(id);
			if (	notAppliedPos == notAppliedEnd
					|| notAppliedPos->second != NARS_ADDED) {
				++result;
			}
		}
	}
	return result;
}

size_t ServiceWindow::GetEditableSelectedRulesCount() const {
	size_t result = 0;
	RulesUuids selectedUuids;
	RuleListCtrl &ctrl
		= *boost::polymorphic_downcast<RuleListCtrl *>(FindWindow(CTRL_ID_RULES_LIST));
	ctrl.GetSelected(selectedUuids);
	const NotAppliedRulesUuids::const_iterator notAppliedEnd = m_notAppliedRules.end();
	foreach (const RulesUuids::value_type id, selectedUuids) {
		const NotAppliedRulesUuids::const_iterator notAppliedPos
			= m_notAppliedRules.find(id);
		if (notAppliedPos == notAppliedEnd || notAppliedPos->second != NARS_DELETED) {
			++result;
		}
	}
	return result;
}

size_t ServiceWindow::GetModifiedSelectedRulesCount() const {
	size_t result = 0;
	RulesUuids selectedUuids;
	RuleListCtrl &ctrl
		= *boost::polymorphic_downcast<RuleListCtrl *>(FindWindow(CTRL_ID_RULES_LIST));
	ctrl.GetSelected(selectedUuids);
	const NotAppliedRulesUuids::const_iterator notAppliedEnd = m_notAppliedRules.end();
	foreach (const RulesUuids::value_type &id, selectedUuids) {
		const NotAppliedRulesUuids::const_iterator notAppliedPos
			= m_notAppliedRules.find(id);
		if (notAppliedPos != notAppliedEnd) {
			++result;
		}
	}
	return result;
}

size_t ServiceWindow::GetAddedRulesCount() const {
	size_t result = 0;
	foreach (const NotAppliedRulesUuids::value_type &i, m_notAppliedRules) {
		if (i.second == NARS_ADDED) {
			++result;
		}
	}
	return result;
}

size_t ServiceWindow::GetDeletedRulesCount() const {
	size_t result = 0;
	foreach (const NotAppliedRulesUuids::value_type &i, m_notAppliedRules) {
		if (i.second == NARS_DELETED) {
			++result;
		}
	}
	return result;
}

void ServiceWindow::EnableList(const bool flag) {
	boost::polymorphic_downcast<RuleListCtrl *>(FindWindow(CTRL_ID_RULES_LIST))
		->Enable(flag);
}

namespace {

	template<class License>
	class TrialLicenseRequestingThread : public wxThread {

	private:

		struct Handles {
			Handles()
					: inet(0),
					connect(0),
					request(0) {
				//...//
			}
			~Handles() {
				if (request) {
					InternetCloseHandle(request);
				}
				if (connect) {
					InternetCloseHandle(connect);
				}
				if (inet) {
					InternetCloseHandle(inet);
				}
			}
			HINTERNET inet;
			HINTERNET connect;
			HINTERNET request;
		};
	
	public:
	
		explicit TrialLicenseRequestingThread(
					const License &currentLicense)
				: wxThread(wxTHREAD_JOINABLE),
				m_currentLicense(currentLicense),
				m_period(0) {
			//...//
		}

		virtual ~TrialLicenseRequestingThread() {
			//...//
		}

	public:

		const std::string & GetLicense() const {
			return m_license;
		}

		size_t GetPeriod() const {
			return m_period;
		}
	
	public:
	
		virtual ExitCode Entry() {

			static time_t lastOperationTime = 0;
			if (lastOperationTime > 0) {
				const time_t timeFromLastOperation = time(0) - lastOperationTime;
				time_t sleepTime = 0;
				if (timeFromLastOperation < 10) {
					sleepTime = 120;
				} else if (timeFromLastOperation < 30) {
					sleepTime = 30;
				} else if (timeFromLastOperation < 60) {
					sleepTime = 20;
				}
				if (sleepTime > 0) {
					wxSleep(sleepTime);
				}
			}

			m_license.clear();
			m_period = 0;

			Handles handles;
			handles.inet
				= InternetOpenA(TUNNELEX_NAME, INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, 0);
			assert(handles.inet);
			if (!handles.inet) {
				return 0;
			}
			handles.connect = InternetConnectA(
				handles.inet,
				TUNNELEX_LICENSE_SERVICE_SUBDOMAIN "." TUNNELEX_DOMAIN,
				80,
				0,
				0,
				INTERNET_SERVICE_HTTP,
				0,
				0);
			assert(handles.connect);
			if (!handles.connect) {
				m_currentLicense.RegisterError(
					"BA6F275B-B631-43BD-970A-A1BBEF7649D0",
					GetLastError());
				return 0;
			}
			LPCSTR accept[2] = {
				"*/*",
				NULL
			};
			handles.request = HttpOpenRequestA(
				handles.connect,
				"POST",
				"license/create/trial",
				NULL,
				NULL,
				accept, 
				INTERNET_FLAG_NO_CACHE_WRITE
					| INTERNET_FLAG_NO_UI
					| INTERNET_FLAG_PRAGMA_NOCACHE
					| INTERNET_FLAG_RELOAD,
				1);
			assert(handles.request);
			if (!handles.request) {
				m_currentLicense.RegisterError(
					"61D0DF02-89A4-4914-935C-C385D200A60C",
					GetLastError());
				return 0;
			}
			
			std::ostringstream postData;
			{
				std::string key = m_currentLicense.GetKey();
				if (key.empty()) {
					key = (Format("%1%%1%%2%%1%%2%%1%%2%%1%%2%%1%%1%%1%") % "0000" % "-").str();
				}
				postData << "lk=" << Helpers::StringUtil::EncodeUrl(key);
			}
			postData
				<< "&v="
				<< Helpers::StringUtil::EncodeUrl<char>(
					ConvertString<String>(TUNNELEX_VERSION_FULL_W).GetCStr());
			const char *const headers
				= "Content-Type: application/x-www-form-urlencoded";
			const BOOL sendResult = HttpSendRequestA(
				handles.request,
				headers,
				DWORD(strlen(headers)),
				const_cast<char *>(postData.str().c_str()),
				DWORD(postData.str().size()));
			if (!sendResult) {
				m_currentLicense.RegisterError(
					"A2AE4864-F1B4-4E4C-8271-8D6F6B97AE32",
					GetLastError());
				return 0;
			}

			std::string licenseTmp;
			size_t periodTmp = 0;
			{
				std::vector<char> answer;
				answer.resize(256);
				size_t realAnswerSize = 0;
				for ( ; ; ) {
					DWORD bytesRead = 0;
					BOOL readResult = InternetReadFile(
						handles.request,
						&answer[realAnswerSize],
						DWORD(answer.size() - realAnswerSize),
						&bytesRead);
					assert(readResult);
					if (!readResult) {
						m_currentLicense.RegisterError(
							"9B11B828-CBAE-407D-B262-3F59CDE659C3",
							GetLastError());
						return 0;
					} if (bytesRead == 0) {
						break;
					}
					realAnswerSize += bytesRead;
					if (realAnswerSize >= answer.size()) {
						answer.resize(answer.size() + 256);
					}
				}
				assert(answer.size() >= realAnswerSize);
				answer.resize(realAnswerSize + 1);
				answer[realAnswerSize] = 0;
#				ifdef DEV_VER
				{
					std::ofstream f("TrialRequestResult.txt", std::ios::trunc);
					f << &answer[0];
				}
#				endif
				typedef boost::split_iterator<std::vector<char>::const_iterator> Iterator;
				for (	Iterator i = boost::make_split_iterator(
							const_cast<const std::vector<char> &>(answer),
							boost::first_finder("\n", boost::is_iequal()));
						i != Iterator();
						++i) {
					std::string line = boost::copy_range<std::string>(*i);
					boost::trim_if(line, boost::is_space() || boost::is_cntrl());
					assert(!line.empty());
					if (line.empty()) {
						continue;
					} else if (licenseTmp.empty()) {
						line.swap(licenseTmp);
					} else {
						try {
							periodTmp = boost::lexical_cast<size_t>(line);
						} catch (const std::bad_cast &) {
							m_currentLicense.RegisterError("98E3301F-7F17-43E6-B6C7-6C7C2F1D0BFA");
							assert(false);
						}
						break;
					}
				}
			}
			
			assert(!licenseTmp.empty());
			assert(periodTmp != 0);
			licenseTmp.swap(m_license);
			m_period = periodTmp;

			return 0;

		}

	private:

		const License &m_currentLicense;
		std::string m_license;
		size_t m_period;

	};


}

bool ServiceWindow::Activate() {
	if (wxGetApp().GetConfig().Exists(wxT("/License/OfflineActivation/State"))) {
		const int cmd = wxMessageBox(
			wxT("Previously the request for the offline activation has been generated.")
				wxT(" Would you like to continue activation and enter an activation code?")
				wxT("\n\nClick Yes to continue current offline activation,")
				wxT(" or click No to stop current and start another activation.")
				wxT(" WARNING: If you stop the offline activation, the request")
				wxT(" that you have already sent, will become null and void."),
			wxT("License activation"),
			wxYES_NO | wxCANCEL | wxCENTER | wxICON_EXCLAMATION,
			this);
		if (cmd == wxNO) {
			wxGetApp().GetConfig().DeleteEntry(
				wxT("/License/OfflineActivation/State"));
		} else {
			if (cmd == wxYES && LicenseKeyDlg(*m_service, this).ShowModal() == wxID_OK) {
				wxGetApp().GetConfig().DeleteEntry(
					wxT("/License/OfflineActivation/State"));
				return true;
			} else {
				return false;
			}
		}
	}
	return LicenseEnterDlg(*m_service, this).ShowModal() == wxID_OK;
	
}

bool ServiceWindow::ActivateTrial() {

	assert(m_trialLicenseCache.empty() || !m_trialLicenseCacheTime.is_not_a_date_time());
	if (	m_trialLicenseCache.empty()
			||  pt::second_clock::universal_time() - m_trialLicenseCacheTime >= pt::hours(24)) {

		using namespace Licensing;

		std::unique_ptr<ExeLicense> currentLicense(new ExeLicense(LicenseState(GetService())));
		assert(!currentLicense->IsTrial());
		if (currentLicense->IsTrial()) {
			wxMessageBox(
				wxT("Product trial already activated."),
				wxT("Product trial activation"),
				wxOK,
				this);
			return false;
		}

		TrialLicenseRequestingThread<ExeLicense> requesting(*currentLicense);
		requesting.Create();
		requesting.Run();
		wxProgressDialog progress(
			wxT("Requesting..."),
			wxT("Requesting trial license, please wait..."),
			100,
			NULL,
			wxPD_APP_MODAL | wxPD_SMOOTH);
		for ( ; requesting.IsRunning(); progress.Pulse(), wxMilliSleep(25));
		std::string license = requesting.GetLicense();
		const size_t period = requesting.GetPeriod();
		if (license.empty() || !period) {
			currentLicense->RegisterError("641B3D52-E40E-4D3B-93E7-392DDA90163C");
			wxLogError(
				wxT("Unknown error at trial license requesting.")
				wxT(" Please check an Internet connection and Internet Explorer proxy server settings")
				wxT(" or try offline activation."));
			wxGetApp().OpenTrialRequestPage();
			return false;
		}

		license.swap(m_trialLicenseCache);
		m_trialLicenseCacheTime = pt::second_clock::universal_time();
		m_trialLicensePeriodCache = period;

	} else {
		assert(m_trialLicensePeriodCache != 0);
	}

	WFormat message(
		L"You are granted rights to use one copy of the " TUNNELEX_NAME_W
			L" with the unlimited functionality for evaluation purposes only."
			L" You are NOT allowed to use it for any commercial purpose."
			L" After the expiration of the %1%-day trial period You must stop"
			L" using " TUNNELEX_NAME_W L" or purchase the Single User License."
			L" Please, see the End-User License Agreement for details."
			L" A %1%-day trial period starts upon this activation."
			L"\n\nBy clicking Yes you agree to be bound by the terms of the Trial License."
			L" Do you want to continue and start %1%-day trial period now?");
	message % m_trialLicensePeriodCache;
	const int answer = wxMessageBox(
		message.str().c_str(),
		wxT("Product trial activation"),
		wxYES_NO | wxICON_EXCLAMATION,
		this);
	if (answer != wxYES) {
		TunnelEx::Licensing::ExeLicense::RegisterError(
			"B35B8A8C-800F-4ECC-806B-A36DD24AECDF",
			answer,
			LicenseState(GetService()));
		return false;
	}

	OnlineActivation activation;
	activation.Activate(m_trialLicenseCache, GetService());
	m_trialLicenseCache.clear();
	if (!activation.GetActivationResult()) {
		wxGetApp().OpenTrialRequestPage(true);
		return false;
	}

	{
		using namespace Licensing;
		std::unique_ptr<ExeLicense> license(new ExeLicense(LicenseState(GetService())));
		if (	!license->IsFeatureAvailable(true)
				&& license->IsTrial()
				&& license->GetUnactivityReason() == UR_TIME) {
			std::wstringstream ss;
			ss << L"Time use of your free trial has come to an end";
			const boost::optional<pt::ptime> timeToLimit
				= license->GetLimitationTimeTo();
			if (timeToLimit) {
				ss << L" on ";
				std::auto_ptr<pt::wtime_facet> facet(new pt::wtime_facet(L"%B, %d %Y"));
				std::locale locTo(std::cout.getloc(), facet.get());
				facet.release();
				ss.imbue(locTo);
				ss << *timeToLimit;
			}
			ss	<< L"."
				wxT(" To continue to use the product please purchase")
				<< L" the professional version of the product."
				<< L" Would you go to the order page?";
			const int answer = wxMessageBox(
				ss.str().c_str(),
				wxT("Free trial has come to an end"),
				wxYES_NO | wxCENTER | wxICON_EXCLAMATION,
				this);
			if (answer == wxYES) {
				wxGetApp().OpenOrderPage();
			} else {
				license->RegisterError(
					"87DCF214-48A7-4766-9EEC-16A51DB22A42",
					answer);
			}
		}
	}

	return true;

}

//////////////////////////////////////////////////////////////////////////

DEFINE_EVENT_TYPE(EVT_SERVICE_WINDOW_ACTION);

ServiceWindow::Event::Event(wxEventType type, Id id)
		: wxNotifyEvent(type, id) {
	//...//
}

ServiceWindow::Event::~Event() {
	//...//
}

wxEvent * ServiceWindow::Event::Clone() const {
	return new Event(*this);
}

//////////////////////////////////////////////////////////////////////////
