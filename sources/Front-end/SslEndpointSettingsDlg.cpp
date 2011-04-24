/**************************************************************************
 *   Created: 2010/11/27 11:02
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "SslEndpointSettingsDlg.hpp"
#include "SslCertificateListDlg.hpp"
#include "ServiceWindow.hpp"
#include "Application.hpp"

#include "Modules/Inet/InetEndpointAddress.hpp"

using namespace TunnelEx;
using TunnelEx::Mods::Inet::TcpEndpointAddress;

enum SslEndpointSettingsDlg::Control {
	CONTROL_SELECT_CERTIFICATE,
	CONTROL_SELECT_REMOTE_CERTIFICATES,
	CONTROL_USE_ANONYMOUS_CERTIFICATE,
	CONTROL_VERIFY_REMOTE_SERTIFICATES
};

BEGIN_EVENT_TABLE(SslEndpointSettingsDlg, wxDialog)
	EVT_BUTTON(wxID_OK,	SslEndpointSettingsDlg::OnOk)
	EVT_BUTTON(wxID_HELP, SslEndpointSettingsDlg::OnHelp)
	EVT_BUTTON(
		CONTROL_SELECT_CERTIFICATE,
		SslEndpointSettingsDlg::OnSelectCertificate)
	EVT_BUTTON(
		CONTROL_SELECT_REMOTE_CERTIFICATES,
		SslEndpointSettingsDlg::OnSelectRemoteCertificates)
	EVT_CHECKBOX(
		CONTROL_USE_ANONYMOUS_CERTIFICATE,
		SslEndpointSettingsDlg::OnUseAnonymousCertificateToggle)
	EVT_CHECKBOX(
		CONTROL_VERIFY_REMOTE_SERTIFICATES,
		SslEndpointSettingsDlg::OnVerifyRemoteCertificatesToggle)
END_EVENT_TABLE()

SslEndpointSettingsDlg::SslEndpointSettingsDlg(
			bool isServer,
			const SslCertificateId &certificate,
			const SslCertificateIdCollection &remoteCertificates,
			wxWindow *parent,
			ServiceWindow &service,
			const bool readOnly)
		: wxDialog(
			parent,
			wxID_ANY,
			wxT("SSL endpoint settings")),
		m_service(service),
		m_certificate(certificate),
		m_remoteCertificates(remoteCertificates),
		m_readOnly(readOnly) {
	if (m_certificate.IsEmpty()) {
		m_certificate = TcpEndpointAddress::GetAnonymousSslCertificateMagicName();
	}
	CreateControls(isServer);
}

SslEndpointSettingsDlg::~SslEndpointSettingsDlg() {
	//...//
}

void SslEndpointSettingsDlg::CreateControls(bool isServer) {

	const Theme &theme = wxGetApp().GetTheme();
	const wxSizerFlags center(wxSizerFlags(0).Center());

	std::auto_ptr<wxBoxSizer> topBox(new wxBoxSizer(wxVERTICAL));

	{

		std::auto_ptr<wxBoxSizer> groupBox(new wxBoxSizer(wxVERTICAL));

		m_useAnonymousCertificateToggle = new wxCheckBox(
			this,
			CONTROL_USE_ANONYMOUS_CERTIFICATE,
			isServer
				?	wxT("Automatically generate anonymous certificate")
				:	wxT("Use anonymous cipher"),
			wxDefaultPosition,
			wxDefaultSize,
			wxCHK_2STATE);
		m_useAnonymousCertificateToggle->Enable(!m_readOnly);
		groupBox->Add(m_useAnonymousCertificateToggle, wxSizerFlags(0).Expand());

		std::auto_ptr<wxBoxSizer> selectCertBox(new wxBoxSizer(wxHORIZONTAL));
		m_certificateLabel
			= new wxStaticText(this, wxID_ANY, wxT("Use certificate:"));
		m_certificateLabel->Enable(!m_readOnly);
		selectCertBox->Add(m_certificateLabel, center);
		m_certificateInfo = new wxTextCtrl(this, wxID_ANY);
		m_certificateInfo->Enable(!m_readOnly);
		m_certificateInfo->SetEditable(false);
		m_certificateInfo->SetMinSize(wxSize(200, -1));
		selectCertBox->Add(m_certificateInfo, wxSizerFlags(center).Proportion(1));
		selectCertBox->Add(theme.GetDlgBorder(), 0);
		m_certificateSelect
			= new wxButton(this, CONTROL_SELECT_CERTIFICATE, wxT("Select..."));
		m_certificateSelect->Enable(!m_readOnly);
		selectCertBox->Add(m_certificateSelect, center);

		groupBox->Add(0, theme.GetDlgBorder());
		groupBox->Add(selectCertBox.get(), wxSizerFlags(center).Expand());
		selectCertBox.release();

		wxStaticBoxSizer &group
			= *new wxStaticBoxSizer(wxHORIZONTAL, this, wxT("Certificate"));
		group.Add(groupBox.get(), theme.GetStaticBoxFlags());
		groupBox.release();
		topBox->Add(&group, theme.GetTopSizerFlags());

	}

	{

		std::auto_ptr<wxBoxSizer> groupBox(new wxBoxSizer(wxVERTICAL));

		m_verifyRemoteCertificatesToggle = new wxCheckBox(
			this,
			CONTROL_VERIFY_REMOTE_SERTIFICATES,
			wxT("Check the validity of a remote certificate"),
			wxDefaultPosition,
			wxDefaultSize,
			wxCHK_2STATE);
		m_verifyRemoteCertificatesToggle->Enable(!m_readOnly);
		groupBox->Add(m_verifyRemoteCertificatesToggle, wxSizerFlags(0).Expand());

		std::auto_ptr<wxBoxSizer> selectCertBox(new wxBoxSizer(wxHORIZONTAL));
		m_remoteCertificatesLabel
			= new wxStaticText(this, wxID_ANY, wxT("Verify with certificates:"));
		m_remoteCertificatesLabel->Enable(!m_readOnly);
		selectCertBox->Add(m_remoteCertificatesLabel, center);
		m_remoteCertificatesInfo = new wxTextCtrl(this, wxID_ANY);
		m_remoteCertificatesInfo->Enable(!m_readOnly);
		m_remoteCertificatesInfo->SetEditable(false);
		m_remoteCertificatesInfo->SetMinSize(wxSize(200, -1));
		selectCertBox->Add(m_remoteCertificatesInfo, wxSizerFlags(center).Proportion(1));
		selectCertBox->Add(theme.GetDlgBorder(), 0);
		m_remoteCertificatesSelect
			= new wxButton(this, CONTROL_SELECT_REMOTE_CERTIFICATES, wxT("Select..."));
		m_remoteCertificatesSelect->Enable(!m_readOnly);
		selectCertBox->Add(m_remoteCertificatesSelect, center);

		groupBox->Add(0, theme.GetDlgBorder());
		groupBox->Add(selectCertBox.get(), wxSizerFlags(center).Expand());
		selectCertBox.release();

		wxStaticBoxSizer &group
			= *new wxStaticBoxSizer(wxHORIZONTAL, this, wxT("Verification"));
		group.Add(groupBox.get(), theme.GetStaticBoxFlags());
		groupBox.release();
		topBox->Add(&group, theme.GetTopSizerFlags());

	}

	topBox->Add(new wxStaticLine(this), theme.GetTopSizerFlags());
	topBox->Add(
		CreateButtonSizer(wxOK | wxCANCEL | wxHELP),
		theme.GetTopSizerFlags().Right());
	if (m_readOnly) {
		FindWindow(wxID_OK)->Enable(false);
	}

	topBox->Add(0, theme.GetDlgBottomBorder());

	SetSizer(topBox.get());
	topBox.release()->SetSizeHints(this);
	Center();

	UpdateState();

}

void SslEndpointSettingsDlg::OnOk(wxCommandEvent &) {
	EndModal(wxID_OK);
}

void SslEndpointSettingsDlg::OnHelp(wxCommandEvent &) {
	wxGetApp().DisplayHelp(wxT("dialogs/ssl-endpoint-settings"));
}

void SslEndpointSettingsDlg::SelectCertificate() {
	SslCertificateListDlg dlg(m_service, this, SslCertificateListDlg::MODE_SELECT_PRIVATE);
	dlg.Select(m_certificate);
	if (dlg.ShowModal() != wxID_OK) {
		return;
	}
	SslCertificateIdCollection ids;
	dlg.GetSelected(ids);
	assert(ids.GetSize() == 1);
	assert(ids.GetSize() == 0 || !ids[0].IsEmpty());
	if (ids.GetSize() > 0 && !ids[0].IsEmpty()) {
		m_certificate = ids[0];
	} else {
		m_certificate = TcpEndpointAddress::GetAnonymousSslCertificateMagicName();
	}
	UpdateState();
}

void SslEndpointSettingsDlg::SelectRemoteCertificates() {
	SslCertificateListDlg dlg(m_service, this, SslCertificateListDlg::MODE_SELECT);
	dlg.Select(m_remoteCertificates);
	if (dlg.ShowModal() != wxID_OK) {
		return;
	}
	dlg.GetSelected(m_remoteCertificates);
	assert(m_remoteCertificates.GetSize() > 0);
	UpdateState();
}

void SslEndpointSettingsDlg::OnSelectCertificate(wxCommandEvent &) {
	SelectCertificate();
}

void SslEndpointSettingsDlg::OnSelectRemoteCertificates(wxCommandEvent &) {
	SelectRemoteCertificates();
}


void SslEndpointSettingsDlg::OnUseAnonymousCertificateToggle(wxCommandEvent &) {
	if (!m_useAnonymousCertificateToggle->GetValue()) {
		if (m_certificate == TcpEndpointAddress::GetAnonymousSslCertificateMagicName()) {
			SelectCertificate();
		}
	} else {
		m_certificate = TcpEndpointAddress::GetAnonymousSslCertificateMagicName();
	}
	UpdateState();
}

void SslEndpointSettingsDlg::OnVerifyRemoteCertificatesToggle(wxCommandEvent &) {
	if (m_verifyRemoteCertificatesToggle->GetValue()) {
		if (m_remoteCertificates.GetSize() == 0) {
			SelectRemoteCertificates();
		}
	} else {
		m_remoteCertificates.SetSize(0);
	}
	UpdateState();
}

void SslEndpointSettingsDlg::UpdateState() {

	assert(!m_certificate.IsEmpty());

	if (m_certificate != TcpEndpointAddress::GetAnonymousSslCertificateMagicName()) {
		texs__SslCertificateInfo info;
		m_service.GetService().GetSslCertificate(m_certificate.GetCStr(), info);
		if (!info.id.empty()) {
			m_certificateInfo->SetValue(GetCertificateStringInfo(info));
		} else {
			wxLogError(L"Could not get SSL certificate.");
			m_certificateInfo->SetValue(wxT("<unknown>"));
		}
		m_useAnonymousCertificateToggle->SetValue(false);
	} else {
		m_useAnonymousCertificateToggle->SetValue(true);
		m_certificateInfo->SetValue(wxEmptyString);
	}

	if (m_remoteCertificates.GetSize() > 0) {
		std::list<wxString> strInfos;
		const size_t remoteCertificatesNumb = m_remoteCertificates.GetSize();
		for (size_t i = 0; i < remoteCertificatesNumb; ++i) {
			texs__SslCertificateInfo info;
			m_service.GetService().GetSslCertificate(
				m_remoteCertificates[i].GetCStr(),
				info);
			if (!info.id.empty()) {
				strInfos.push_back(GetCertificateStringInfo(info));
			} else {
				wxLogError(L"Could not get SSL verification certificate.");
				strInfos.push_back(wxT("<unknown>"));
			}
		}
		m_verifyRemoteCertificatesToggle->SetValue(true);
		m_remoteCertificatesInfo->SetValue(boost::join(strInfos, wxT("; ")));
	} else {
		m_verifyRemoteCertificatesToggle->SetValue(false);
		m_remoteCertificatesInfo->SetValue(wxEmptyString);
	}

	{
		const bool isActive
			= !m_readOnly && !m_useAnonymousCertificateToggle->GetValue();
		m_certificateLabel->Enable(isActive);
		m_certificateInfo->Enable(isActive);
		m_certificateSelect->Enable(isActive);
	}
	{
		const bool isActive
			= !m_readOnly && m_verifyRemoteCertificatesToggle->GetValue();
		m_remoteCertificatesLabel->Enable(isActive);
		m_remoteCertificatesInfo->Enable(isActive);
		m_remoteCertificatesSelect->Enable(isActive);
	}

}

wxString SslEndpointSettingsDlg::GetCertificateStringInfo(
			const texs__SslCertificateInfo &info) {
	return !info.subjectCommonName.empty()
		?	wxString::FromAscii(info.subjectCommonName.c_str())
		:	!info.subjectOrganization.empty()
			?	wxString::FromAscii(info.subjectOrganization.c_str()) 
			:	!info.serial.empty()
				?	wxString::FromAscii(info.serial.c_str())
				:	wxT("<unknown>");
}
