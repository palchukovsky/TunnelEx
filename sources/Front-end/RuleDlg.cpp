/**************************************************************************
 *   Created: 2010/05/23 5:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "RuleDlg.hpp"
#include "ServiceWindow.hpp"
#include "Application.hpp"

#include "Rule.hpp"

using namespace TunnelEx;

BEGIN_EVENT_TABLE(RuleDlg, wxDialog)
	EVT_BUTTON(wxID_OK, RuleDlg::OnOk)
	EVT_BUTTON(wxID_CANCEL, RuleDlg::OnCancel)
	EVT_BUTTON(wxID_HELP, RuleDlg::OnHelp)
	EVT_COMMAND(wxID_ANY, wxEVT_COMMAND_TEXT_ENTER, RuleDlg::OnOk)
END_EVENT_TABLE()

RuleDlg::RuleDlg(
			const wxString &title,
			ServiceWindow &service,
			wxWindow *parent)
		: Base(parent, wxID_ANY, title),
		m_service(service),
		m_isChanged(false),
		m_isNewRule(true),
		m_isInited(false) {
	//...//
}

RuleDlg::RuleDlg(
			const wxString &title,
			ServiceWindow &service,
			wxWindow *parent,
			const TunnelEx::Rule &rule)
		: Base(parent, wxID_ANY, title),
		m_rule(Clone(rule)),
		m_service(service),
		m_isChanged(false),
		m_isNewRule(false),
		m_isInited(false) {
	//...//
}

RuleDlg::~RuleDlg() {
	//...//
}

const ServiceAdapter & RuleDlg::GetService() const {
	return const_cast<RuleDlg *>(this)->GetService();
}

ServiceAdapter & RuleDlg::GetService() {
	return m_service.GetService();
}

bool RuleDlg::Show(bool show /*= true*/) {
	CheckInit();
	return Base::Show(show);
}

int RuleDlg::ShowModal() {
	CheckInit();
	return Base::ShowModal();
}

void RuleDlg::SaveTemplate() const {

	Config &config = wxGetApp().GetConfig();

	if (IsNewRule()) {
		config.Write(
			wxT("/Rule/Template/IsEnabled"),
			boost::polymorphic_downcast<wxCheckBox *>(
					FindWindow(CONTROL_ID_IS_ENABLED))
				->GetValue());
	}

	if (FindWindow(CONTROL_ID_ERROS_TREATMENT)) {
		config.Write(
			wxT("/Rule/Template/ErrorTreatment"),
			boost::polymorphic_downcast<wxChoice *>(
					FindWindow(CONTROL_ID_ERROS_TREATMENT))
				->GetStringSelection());
	}

}

void RuleDlg::CheckInit() {
	if (!m_isInited) {
		Init();
	}
}

void RuleDlg::Init() {

	assert(!m_isInited);

	const Theme &theme = wxGetApp().GetTheme();

	SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY);

	std::auto_ptr<wxBoxSizer> topSizer(new wxBoxSizer(wxVERTICAL));

	m_generalSettingsPanel = &CreateControlGeneralSettings();
	topSizer->Add(m_generalSettingsPanel, theme.GetTopSizerFlags());

	std::auto_ptr<wxSizer> content = CreateControlContent();
	topSizer->Add(content.get(), theme.GetTopSizerFlags());
	content.release();
	std::auto_ptr<wxSizer> options = CreateControlOptions();
	if (options.get()) {
		topSizer->Add(options.get(), theme.GetTopSizerFlags());
		options.release();
	}
	topSizer->Add(new wxStaticLine(this, wxID_ANY), theme.GetTopSizerFlags());

	std::auto_ptr<wxSizer> ruleInfoBox = CreateControlRuleInfo();
	if (ruleInfoBox.get()) {
		std::auto_ptr<wxBoxSizer> buttonsBox(new wxBoxSizer(wxHORIZONTAL));
		buttonsBox->Add(ruleInfoBox.get(), wxSizerFlags(0).Expand());
		ruleInfoBox.release();
		buttonsBox->AddSpacer(theme.GetDlgBorder() * 2);
		std::auto_ptr<wxSizer> buttons = CreateButtons();
		buttonsBox->Add(buttons.get(), wxSizerFlags(1).Expand().Right());
		buttons.release();
		topSizer->Add(buttonsBox.get(), theme.GetTopSizerFlags());
		buttonsBox.release();
	} else {
		std::auto_ptr<wxSizer> buttons = CreateButtons();
		topSizer->Add(buttons.get(), theme.GetTopSizerFlags().Right());
		buttons.release();
	}

	topSizer->AddSpacer(theme.GetDlgBottomBorder());
	
	topSizer->SetMinSize(GetMinSize());
	SetSizer(topSizer.get());
	topSizer.release()->SetSizeHints(this);
	Center();

	m_isInited = true;

}

std::auto_ptr<wxSizer> RuleDlg::CreateButtons() {
	std::auto_ptr<wxSizer> result(CreateButtonSizer(wxOK | wxCANCEL | wxHELP));
	if (!IsLicenseValid()) {
		FindWindow(wxID_OK)->Enable(false);
	}
	return result;
}

std::auto_ptr<wxSizer> RuleDlg::CreateControlRuleInfo() {
	return std::auto_ptr<wxSizer>();
}

wxPanel & RuleDlg::CreateControlGeneralSettings() {
	
	wxPanel &panel =
		*new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0);
	std::auto_ptr<wxBoxSizer> topBox(new wxBoxSizer(wxVERTICAL));

	wxCheckBox &enabled = *new wxCheckBox(
		&panel,
		CONTROL_ID_IS_ENABLED,
		IsNewRule()
			?	wxT("Activate after creation")
			:	wxT("Enabled"));
	enabled.Enable(IsLicenseValid());
	if (IsNewRule()) {
		bool val = true;
		wxGetApp()
			.GetConfig()
			.Read(wxT("/Rule/Template/IsEnabled"), &val, val);
		enabled.SetValue(val);
	} else {
		enabled.SetValue(GetRule().IsEnabled());
	}
	topBox->Add(&enabled);

	topBox->AddSpacer(wxGetApp().GetTheme().GetDlgBorder());

	std::auto_ptr<wxBoxSizer> box(new wxBoxSizer(wxHORIZONTAL));
	box->Add(
		new wxStaticText(&panel, wxID_ANY, wxT("Name:")),
		wxSizerFlags(0).Center());
	wxTextCtrl &name = *new wxTextCtrl(
		&panel,
		CONTROL_ID_RULE_NAME,
		GetRule().GetName().GetCStr(),
		wxDefaultPosition,
		wxDefaultSize,
		wxTE_NOHIDESEL | wxTE_PROCESS_ENTER);
	name.Enable(IsLicenseValid());
	name.SetToolTip(wxT("Name will be displayed in the rule list, can be empty."));
	box->Add(&name, wxSizerFlags(1).Center());
	topBox->Add(box.get(), wxSizerFlags(0).Expand());
	box.release();

	wxStaticBoxSizer &group
		= *new wxStaticBoxSizer(wxHORIZONTAL,  &panel,  wxT("General"));
	group.Add(topBox.get(), wxGetApp().GetTheme().GetStaticBoxFlags());
	topBox.release();

	panel.SetSizer(&group);
	return panel;

}

std::auto_ptr<wxSizer> RuleDlg::CreateControlOptions() {
		
	std::auto_ptr<wxBoxSizer> treatBox(new wxBoxSizer(wxHORIZONTAL));
	treatBox->Add(
		new wxStaticText(this, wxID_ANY, wxT("Treat errors as:")),
		wxSizerFlags(0).Center());
	
	wxArrayString treatTypes;
	treatTypes.Add(wxT("information"));
	treatTypes.Add(wxT("warning"));
	treatTypes.Add(wxT("error"));
	wxString ctrlVal;
	if (IsNewRule()) {
		wxGetApp()
			.GetConfig()
			.Read(wxT("/Rule/Template/ErrorTreatment"), &ctrlVal);
	}
	assert(treatTypes.Index(ctrlVal) != wxNOT_FOUND || ctrlVal.IsEmpty());
	if (ctrlVal.IsEmpty() || treatTypes.Index(ctrlVal) == wxNOT_FOUND) {
		switch (m_rule->GetErrorsTreatment()) {
			case Rule::ERRORS_TREATMENT_INFO:
				ctrlVal = wxT("information");
				break;
			case Rule::ERRORS_TREATMENT_WARN:
				ctrlVal = wxT("warning");
				break;
			default:
				assert(false);
			case Rule::ERRORS_TREATMENT_ERROR:
				ctrlVal = wxT("error");
				break;
		}
	}
	assert(treatTypes.Index(ctrlVal) != wxNOT_FOUND);
	wxChoice &treatInput = *new wxChoice(
		this,
		CONTROL_ID_ERROS_TREATMENT,
		wxDefaultPosition,
		wxDefaultSize,
		treatTypes,
		wxTE_NOHIDESEL | wxTE_PROCESS_ENTER);
	treatInput.Enable(IsLicenseValid());
	treatInput.SetStringSelection(ctrlVal);
	treatInput.SetToolTip(
		wxT("Allows to specify which type of log error ")
			wxT("will be added if outcomig connection failed."));
	treatBox->Add(&treatInput, wxSizerFlags(0).Center());
	
	std::auto_ptr<wxBoxSizer> topBox(new wxBoxSizer(wxVERTICAL));
	std::auto_ptr<wxSizer> additionalOptions = CreateControlAdditionalOptions();
	
	topBox->Add(treatBox.get(), wxSizerFlags(0).Expand());
	treatBox.release();
	if (additionalOptions.get()) {
		topBox->AddSpacer(wxGetApp().GetTheme().GetDlgBorder());
		topBox->Add(
			additionalOptions.get(), wxSizerFlags(0).Expand());
		additionalOptions.release();
	}

	wxStaticBoxSizer &group = *new wxStaticBoxSizer(wxVERTICAL, this, wxT("Options"));
	group.Add(topBox.get(), wxGetApp().GetTheme().GetStaticBoxFlags());
	topBox.release();

	std::auto_ptr<wxSizer> result(new wxBoxSizer(wxHORIZONTAL));
	result->Add(&group, wxSizerFlags(1).Expand());
	return result;

}

std::auto_ptr<wxSizer> RuleDlg::CreateControlAdditionalOptions() {
	return std::auto_ptr<wxSizer>();
}

void RuleDlg::OnOk(wxCommandEvent &) {

	if (!Validate()) {
		return;
	}

	std::auto_ptr<Rule> newRule = Clone(*m_rule);
	bool isChanged = IsChanged();

	{
		const wxTextCtrl &ctrl
			=  *boost::polymorphic_downcast<wxTextCtrl *>(FindWindow(CONTROL_ID_RULE_NAME));
		if (newRule->GetName() != ctrl.GetValue().c_str()) {
			newRule->SetName(ctrl.GetValue().c_str());
			isChanged = true;
		}
	}

	{
		const wxCheckBox &ctrl
			=  *boost::polymorphic_downcast<wxCheckBox *>(FindWindow(CONTROL_ID_IS_ENABLED));
		if (newRule->IsEnabled() != ctrl.GetValue()) {
			newRule->Enable(ctrl.GetValue());
			isChanged = true;
		}
	}
	
	{
		const wxChoice &ctrl
			= *boost::polymorphic_downcast<wxChoice *>(FindWindow(CONTROL_ID_ERROS_TREATMENT));
		Rule::ErrorsTreatment newErrorsTreatment
			= TunnelRule::ERRORS_TREATMENT_ERROR;
		if (FindWindow(CONTROL_ID_ERROS_TREATMENT)) {
			if (ctrl.GetStringSelection() == wxT("information")) {
				newErrorsTreatment = TunnelRule::ERRORS_TREATMENT_INFO;
			} else  if (ctrl.GetStringSelection() == wxT("warning")) {
				newErrorsTreatment = TunnelRule::ERRORS_TREATMENT_WARN;
			} else {
				assert(ctrl.GetStringSelection() == wxT("error"));
				newErrorsTreatment = TunnelRule::ERRORS_TREATMENT_ERROR;
			}
		}
		isChanged
			= isChanged || newRule->GetErrorsTreatment() != newErrorsTreatment;
		newRule->SetErrorsTreatment(newErrorsTreatment);
	}

	if (Save(*newRule)) {
		isChanged = true;
	}

	if (isChanged) {
		SaveTemplate();
		swap(m_rule, newRule);
		m_isChanged = true;
	}

	EndModal(m_isChanged ? wxID_OK : wxID_CANCEL);

}

void RuleDlg::OnCancel(wxCommandEvent &) {
	Cancel();
}

void RuleDlg::Cancel() {
	EndModal(wxID_CANCEL);
}

void RuleDlg::OnHelp(wxCommandEvent &) {
	wxString path = wxT("rule-set/");
	path += GetHelpPath();
	wxGetApp().DisplayHelp(path);
}
