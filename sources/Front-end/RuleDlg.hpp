/**************************************************************************
 *   Created: 2010/05/23 5:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__RuleDlg_hpp__1005230541
#define INCLUDED_FILE__TUNNELEX__RuleDlg_hpp__1005230541

#include "Core/Rule.hpp"

class ServiceAdapter;
class ServiceWindow;

class RuleDlg : public wxDialog {

public:

	typedef wxDialog Base;

protected:

	enum ControlId {
		CONTROL_ID_RULE_NAME,
		CONTROL_ID_IS_ENABLED,
		CONTROL_ID_ERROS_TREATMENT,
		CONTROL_ID_LAST
	};

public:

	explicit RuleDlg(
			const wxString &title,
			ServiceWindow &service,
			wxWindow *parent);
	explicit RuleDlg(
			const wxString &title,
			ServiceWindow &service,
			wxWindow *parent,
			const TunnelEx::Rule &);
	virtual ~RuleDlg();

	DECLARE_NO_COPY_CLASS(RuleDlg);
	DECLARE_EVENT_TABLE();

public:

	virtual bool Show(bool show = true);
	virtual int ShowModal();

	virtual bool IsLicenseValid() const = 0;

	bool IsChanged() const {
		return m_isChanged && !IsNewRule();
	}

	bool IsNewRule() const {
		return m_isNewRule;
	}

public:

	void OnOk(wxCommandEvent &);
	void OnCancel(wxCommandEvent &);
	void OnHelp(wxCommandEvent &);

protected:

	virtual void Cancel();

	virtual void SaveTemplate() const;

	TunnelEx::Rule & GetRule() {
		return *m_rule;
	}

	void SetRule(std::auto_ptr<TunnelEx::Rule> rule) {
		m_rule = rule;
	}

	const ServiceAdapter & GetService() const;
	ServiceAdapter & GetService();

	const ServiceWindow & GetServiceWindow() const {
		return const_cast<RuleDlg *>(this)->GetServiceWindow();
	}
	ServiceWindow & GetServiceWindow() {
		return m_service;
	}

	void CheckInit();
	virtual void Init();

	void EnableGeneralSettings(const bool enable = true) {
		m_generalSettingsPanel->Enable(enable);
	}

	void ShowGeneralSettings(const bool show = true) {
		m_generalSettingsPanel->Show(show);
	}

	wxSize GetGeneralSettingsSize() const {
		return m_generalSettingsPanel->GetSize();
	}

private:

	virtual wxPanel & CreateControlGeneralSettings();
	virtual std::auto_ptr<wxSizer> CreateControlOptions();
	virtual std::auto_ptr<wxSizer> CreateControlAdditionalOptions();
	virtual std::auto_ptr<wxSizer> CreateControlRuleInfo();
	virtual std::auto_ptr<wxSizer> CreateButtons();
	virtual std::auto_ptr<wxSizer> CreateControlContent() = 0;
	
	virtual const wxChar * GetHelpPath() const = 0;
	virtual bool Save(TunnelEx::Rule &rule) const = 0;

protected:

	void SetChanged() {
		m_isChanged = true;
	}

private:

	std::auto_ptr<TunnelEx::Rule> m_rule;
	ServiceWindow &m_service;
	bool m_isChanged;
	const bool m_isNewRule;
	bool m_isInited;

	wxPanel *m_generalSettingsPanel;

};

#endif // INCLUDED_FILE__TUNNELEX__RuleDlg_hpp__1005230541
