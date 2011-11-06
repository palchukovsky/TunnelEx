/**************************************************************************
 *   Created: 2010/10/25 16:38
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "LicenseStartDlg.hpp"
#include "ServiceWindow.hpp"
#include "Application.hpp"
#include "LicensePolicies.hpp"

using namespace TunnelEx::Licensing;

BEGIN_EVENT_TABLE(LicenseStartDlg, wxDialog)
	EVT_BUTTON(LicenseStartDlg::CTRL_REQUEST_TRIAL, LicenseStartDlg::OnRequestTrial)
	EVT_BUTTON(LicenseStartDlg::CTRL_ACTIVATE, LicenseStartDlg::OnActivate)
	EVT_BUTTON(LicenseStartDlg::CTRL_ORDER, LicenseStartDlg::OnOrder)
END_EVENT_TABLE()

LicenseStartDlg::LicenseStartDlg(
			ServiceWindow &service,
			wxWindow *parent,
			wxWindowID id /*= wxID_ANY*/)
		 : wxDialog(
			 parent,
			 id,
			 wxT("Welcome to ") TUNNELEX_NAME_W,
			 wxDefaultPosition,
			 wxSize(340, 165),
			 wxCAPTION | wxCLOSE_BOX | wxSYSTEM_MENU),
		 m_service(service) {
	CreateControls();
}

LicenseStartDlg::~LicenseStartDlg() {
	//...//
}

void LicenseStartDlg::CreateControls() {

	LicenseState licenseState(m_service.GetService());
	const InfoDlgLicense license(licenseState);

	const Theme &theme = wxGetApp().GetTheme();

	std::auto_ptr<wxBoxSizer> contentBox(new wxBoxSizer(wxHORIZONTAL));

	std::auto_ptr<wxBoxSizer> subBox(new wxBoxSizer(wxVERTICAL));
	subBox->AddSpacer(theme.GetDlgBorder() * 2);
	subBox->Add(
		new wxStaticText(
			this,
			wxID_ANY,
			wxT("Welcome to ") TUNNELEX_NAME_W wxT("!")));
	subBox->AddSpacer(theme.GetDlgBorder());
	{
		const wxString licenseState
			= LicenseState::GetAsString(license, wxEmptyString);
		if (!licenseState.IsEmpty()) {
			subBox->Add(new wxStaticText(this, wxID_ANY, licenseState));
			subBox->AddSpacer(theme.GetDlgBorder());
		}
	}
	subBox->Add(
		new wxStaticText(
			this,
			wxID_ANY,
			wxT("Please activate your product copy.")));
	subBox->AddSpacer(theme.GetDlgBorder());
	subBox->AddStretchSpacer(1);
	subBox->Add(
		new wxHyperlinkCtrl(
			this,
			wxID_ANY,
			wxT("Product Web-site"),
			wxT("http://") TUNNELEX_DOMAIN_W wxT("/?about")));
	subBox->Add(
		new wxHyperlinkCtrl(
			this,
			wxID_ANY,
			wxT("Support request online form"),
			wxT("http://") TUNNELEX_DOMAIN_W wxT("/issue/submit?about")));
	contentBox->Add(subBox.get(), wxSizerFlags(1).Expand());
	subBox.release();

	contentBox->AddSpacer(theme.GetDlgBorder());

	subBox.reset(new wxBoxSizer(wxVERTICAL));
	subBox->Add(
		new wxButton(this, CTRL_ACTIVATE, wxT("Activate")),
		wxSizerFlags(0).Expand());
	subBox->AddSpacer(theme.GetDlgBorder() / 2);
	subBox->Add(
		new wxButton(this, CTRL_ORDER, wxT("Purchase online")),
		wxSizerFlags(0).Expand());
	subBox->AddSpacer(theme.GetDlgBorder() / 2);
	
	subBox->Add(
		new wxButton(this, CTRL_REQUEST_TRIAL, wxT("Get a Free Trial")),
		wxSizerFlags(0).Expand());
	{
		auto &trialRequestButton = *FindWindow(CTRL_REQUEST_TRIAL);
		wxSize size = trialRequestButton.GetSize();
		size.SetHeight(size.GetHeight() * 2);
		trialRequestButton.SetMinSize(size);
	}

	subBox->AddStretchSpacer(1);
	subBox->Add(
		new wxButton(this, wxID_CANCEL, wxT("Close")),
		wxSizerFlags(0).Expand());
	contentBox->Add(subBox.get(), wxSizerFlags(0).Expand().Right());
	subBox.release();

	std::auto_ptr<wxBoxSizer> topBox(new wxBoxSizer(wxVERTICAL));
	topBox->Add(contentBox.get(), theme.GetTopSizerFlags().Proportion(1));
	contentBox.release();
	topBox->AddSpacer(theme.GetDlgBottomBorder());

	topBox->SetMinSize(GetSize());
	SetSizer(topBox.get());
	topBox.release()->SetSizeHints(this);
	Center();

	if (license.IsTrial()) {
		FindWindow(CTRL_REQUEST_TRIAL)->Enable(false);
	}

}

void LicenseStartDlg::OnRequestTrial(wxCommandEvent &) {
	if (m_service.ActivateTrial()) {
		EndModal(wxID_OK);
	}
}

void LicenseStartDlg::OnActivate(wxCommandEvent &) {
	if (m_service.Activate()) {
		EndModal(wxID_OK);
	}
}

void LicenseStartDlg::OnOrder(wxCommandEvent &) {
	wxGetApp().OpenOrderPage();
}
