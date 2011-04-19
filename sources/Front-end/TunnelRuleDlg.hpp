/**************************************************************************
 *   Created: 2008/01/09 16:05
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__RuleDlg_h__0801091605
#define INCLUDED_FILE__TUNNELEX__RuleDlg_h__0801091605

#include "RuleDlg.hpp"

class ServiceAdapter;

class TunnelRuleDlg : public RuleDlg {

public:

	typedef RuleDlg Base;

protected:

	enum ControlId {
		CONTROL_ID_RULE_NAME = RuleDlg::CONTROL_ID_LAST,
		CONTROL_ID_INPUTS,
		CONTROL_ID_DESTINATIONS,
		CONTROL_ID_SORT_DESTINATIONS_BY_PING,
		CONTROL_ID_ADD_INPUT,
		CONTROL_ID_ADD_DESTINATION,
		CONTROL_ID_EDIT_INPUT,
		CONTROL_ID_EDIT_DESTINATION,
		CONTROL_ID_REMOVE_SELECTED_INPUTS,
		CONTROL_ID_REMOVE_SELECTED_DESTINATIONS,
		CONTROL_ID_MOVE_SELECTED_DESTINATIONS_UP,
		CONTROL_ID_MOVE_SELECTED_DESTINATIONS_DOWN
	};

public:

	explicit TunnelRuleDlg(
			ServiceWindow &service,
			wxWindow *parent,
			bool isFtpTunnel);
	explicit TunnelRuleDlg(
			ServiceWindow &service,
			wxWindow *parent,
			const TunnelEx::TunnelRule &);
	~TunnelRuleDlg();

	DECLARE_NO_COPY_CLASS(TunnelRuleDlg);

public:

	const TunnelEx::TunnelRule & GetRule() const {
		return const_cast<TunnelRuleDlg *>(this)->GetRule();
	}

	virtual bool IsLicenseValid() const;

public:

	void OnEditInput(wxCommandEvent &);
	void OnEditDestination(wxCommandEvent &);
	void OnInputSelectionChange(wxCommandEvent &);
	void OnDestinationSelectionChange(wxCommandEvent &);
	void OnAddInput(wxCommandEvent &);
	void OnAddDestination(wxCommandEvent &);
	void OnRemoveSelectedInputs(wxCommandEvent &);
	void OnRemoveSelectedDestinations(wxCommandEvent &);
	void OnMoveSelectedDestinationsUp(wxCommandEvent &);
	void OnMoveSelectedDestinationsDown(wxCommandEvent &);
	void OnFtpTunnelSwitch(wxCommandEvent &);

protected:

	TunnelEx::TunnelRule & GetRule() {
		return *boost::polymorphic_downcast<TunnelEx::TunnelRule *>(&RuleDlg::GetRule());
	}

	void Init();
	virtual void Cancel();

private:

	virtual std::auto_ptr<wxSizer> CreateControlAdditionalOptions();
	virtual std::auto_ptr<wxSizer> CreateControlRuleInfo();
	virtual std::auto_ptr<wxSizer> CreateControlContent();
	virtual const wxChar * GetHelpPath() const;
	virtual bool Save(TunnelEx::Rule &) const;

private:

	wxString GetRuleTitle() const;
	void CheckLicense();
	wxControl & CreateControlEnpointList(
			const wxWindowID,
			TunnelEx::RuleEndpointCollection &,
			const wxString &);
	wxArrayString ConvertEndpointCollectionToStrings(
			const TunnelEx::RuleEndpointCollection &,
			bool)
		const;
	wxString ConvertEndpointToStrings(const TunnelEx::RuleEndpoint &, bool) const;

	void EnableInputEndpointListButtons(const bool, const bool);
	void EnableDestinationEndpointListButtons(const bool, const bool, const bool);
	void EditSelectedInput();
	void EditSelectedDestination();
	std::wstring SearchNetworkAdapter(const std::wstring &) const;
	void AddEndpoint(const wxWindowID ctrlId, TunnelEx::RuleEndpointCollection &);
	void EditSelectedEndpoint(const wxWindowID, TunnelEx::RuleEndpointCollection &);
	void RemoveSelectedEndpoints(
				const wxWindowID,
				const TunnelEx::RuleEndpointCollection &,
				boost::function<void(const TunnelEx::RuleEndpointCollection &)>,
				const wxChar *);
	void MoveSelectedEndpoints(
			const wxWindowID ctrlId,
			bool up,
			const TunnelEx::RuleEndpointCollection &originalEndpoints,
			boost::function<void(const TunnelEx::RuleEndpointCollection &)> saver);

	std::wstring GetExternalIpAddress() const;

private:

	bool m_isLicenseValid;
	const bool m_isFtpTunnel;
	bool m_isUpnpDevRequested;

private:

	DECLARE_EVENT_TABLE();

};

#endif // INCLUDED_FILE__TUNNELEX__RuleDlg_h__0801091605
