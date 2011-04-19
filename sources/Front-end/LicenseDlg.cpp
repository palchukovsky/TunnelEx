/**************************************************************************
 *   Created: 2009/11/29 21:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
  **************************************************************************/

#include "Prec.h"

#include "LicenseDlg.hpp"
#include "Auto.hpp"
#include "ServiceWindow.hpp"
#include "ServiceAdapter.hpp"
#include "Application.hpp"
#include "LicensePolicies.hpp"

using namespace TunnelEx::Licensing;

BEGIN_EVENT_TABLE(LicenseDlg, wxDialog)
	EVT_BUTTON(LicenseDlg::CTRL_ACTIVATE, LicenseDlg::OnActivate)
	EVT_BUTTON(LicenseDlg::CTRL_REQUEST_TRIAL, LicenseDlg::OnRequestTrial)
	EVT_BUTTON(LicenseDlg::CTRL_ORDER, LicenseDlg::OnOrderPage)
END_EVENT_TABLE()

LicenseDlg::LicenseDlg(ServiceWindow &service, wxWindow *parent, wxWindowID id)
		: wxDialog(
			parent,
			id,
			wxT("License Info"),
			wxDefaultPosition,
			wxDefaultSize,
			wxCAPTION | wxCLOSE_BOX | wxSYSTEM_MENU),
		m_service(service) {
		
	std::auto_ptr<wxBoxSizer> topBox(new wxBoxSizer(wxVERTICAL));

	m_licensedTo = new wxStaticText(this, wxID_ANY, wxEmptyString);
	m_licensedTo->SetMinSize(wxSize(500, 30));
	topBox->Add(m_licensedTo, wxGetApp().GetTheme().GetTopSizerFlags().Proportion(1));

	topBox->Add(new wxStaticLine(this), wxGetApp().GetTheme().GetTopSizerFlags());

	wxSizerFlags buttonFlags
		= wxSizerFlags(0).Border(wxLEFT, wxGetApp().GetTheme().GetDlgBorder());
	std::auto_ptr<wxBoxSizer> buttonBox(new wxBoxSizer(wxHORIZONTAL));
	buttonBox->AddStretchSpacer(1);
	buttonBox->Add(new wxButton(this, CTRL_ACTIVATE, wxT("Activate")), buttonFlags);
	buttonBox->Add(new wxButton(this, CTRL_ORDER, wxT("Purchase new")), buttonFlags);
	m_trialButton = new wxButton(this, CTRL_REQUEST_TRIAL, wxT("Get a free trial"));
	buttonBox->Add(m_trialButton, buttonFlags);
	wxButton &okButton = *new wxButton(this, wxID_OK, wxT("Close"));
	buttonBox->Add(&okButton, buttonFlags);
	okButton.SetDefault();
	okButton.SetFocus();
	SetAffirmativeId(wxID_OK);

	topBox->Add(
		buttonBox.get(),
		wxGetApp().GetTheme().GetTopSizerFlags());
	buttonBox.release();

	topBox->AddSpacer(wxGetApp().GetTheme().GetDlgBottomBorder());

	topBox->SetMinSize(GetMinSize());
	SetSizer(topBox.get());
	topBox.release()->SetSizeHints(this);
	Center();

	Reinit();

}

LicenseDlg::~LicenseDlg() {
	//...//
}

void LicenseDlg::Reinit() {
	LicenseState licenseState(m_service.GetService());
	const InfoDlgLicense license(licenseState);
	m_licensedTo->SetLabel(
		LicenseState::GetAsString(license, wxT("Free limited version")));
	m_trialButton->Enable(!license.IsTrial());
}

void LicenseDlg::OnActivate(wxCommandEvent &) {
	if (m_service.Activate()) {
		Reinit();
	}
}

void LicenseDlg::OnRequestTrial(wxCommandEvent &) {
	if (m_service.ActivateTrial()) {
		Reinit();
	}
}

void LicenseDlg::OnOrderPage(wxCommandEvent &) {
	wxGetApp().OpenOrderPage();
}
