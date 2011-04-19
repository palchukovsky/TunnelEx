/**************************************************************************
 *   Created: 2010/10/16 16:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "SslCertificateDlg.hpp"
#include "Application.hpp"

enum SslCertificateDlg::Control {
	CONTROL_COUNTRY,
	CONTROL_BROWSE_SIGN_KEY,
	CONTROL_SAVE_TO_FILE
};

BEGIN_EVENT_TABLE(SslCertificateDlg, wxDialog)
	EVT_BUTTON(wxID_OK,	SslCertificateDlg::OnOk)
	EVT_BUTTON(wxID_HELP, SslCertificateDlg::OnHelp)
	EVT_TEXT(CONTROL_COUNTRY, SslCertificateDlg::OnCountryText)
	EVT_BUTTON(CONTROL_BROWSE_SIGN_KEY, SslCertificateDlg::OnBrowseSignKey)
	EVT_BUTTON(CONTROL_SAVE_TO_FILE, SslCertificateDlg::OnSaveToFile)
END_EVENT_TABLE()

SslCertificateDlg::SslCertificateDlg(
			wxWindow *parent,
			wxWindowID id /*= wxID_ANY*/)
		: wxDialog(parent, id, wxT("New SSL Certificate")),
		m_isGenerateMode(true),
		m_country(0) {
	CreateControls();
}

SslCertificateDlg::SslCertificateDlg(
			const texs__SslCertificateInfo &certificate,
			wxWindow *parent,
			wxWindowID id /*= wxID_ANY*/)
		: wxDialog(parent, id, wxT("SSL Certificate")),
		m_certificate(certificate),
		m_isGenerateMode(false),
		m_validCountry(wxString::FromAscii(certificate.subjectCountry.c_str())),
		m_country(0) {
	CreateControls();
}

SslCertificateDlg::~SslCertificateDlg() {
	//...//
}

void SslCertificateDlg::CreateControls() {

	const Theme &theme = wxGetApp().GetTheme();

	std::auto_ptr<wxFlexGridSizer> contentBox(
		new wxFlexGridSizer(
			m_isGenerateMode ? 7 : 9,
			2,
			theme.GetDlgBorder(), 0));
	contentBox->AddGrowableCol(1, 1);

	const wxSizerFlags labelFlags
		= wxSizerFlags(0).Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
	const wxSizerFlags fieldFlags = wxSizerFlags(1).Expand();
	const wxSize fieldSize(300, -1);

	contentBox->Add(new wxStaticText(this, wxID_ANY, wxT("Key size:")), labelFlags);
	if (m_isGenerateMode) {
		wxArrayString keySizes;
		keySizes.Add(wxT("512"));
		keySizes.Add(wxT("1024"));
		keySizes.Add(wxT("2048"));
		keySizes.Add(wxT("4096"));
		keySizes.Add(wxT("8192"));
		m_keySize = new wxChoice(
			this,
			wxID_ANY,
			wxDefaultPosition,
			wxDefaultSize,
			keySizes,
			wxTE_NOHIDESEL | wxTE_PROCESS_ENTER);
		if (!m_keySize->SetStringSelection(boost::lexical_cast<std::wstring>(m_certificate.keySize).c_str())) {
			m_keySize->SetStringSelection(wxT("2048"));
		}
		contentBox->Add(m_keySize);
	} else {
		std::auto_ptr<wxBoxSizer> box(new wxBoxSizer(wxHORIZONTAL));
		m_keySize = 0;
		wxTextCtrl &keySize = *new wxTextCtrl(
			this,
			wxID_ANY,
			boost::lexical_cast<std::wstring>(m_certificate.keySize).c_str());
		keySize.SetEditable(false);
		box->Add(&keySize);
		box->AddSpacer(theme.GetDlgBorder() / 2);
		box->Add(new wxStaticText(this, wxID_ANY, wxT("Private key:")), labelFlags);
		wxTextCtrl &privateKey = *new wxTextCtrl(
			this,
			wxID_ANY,
			m_certificate.isPrivate ? wxT("yes") : wxT("no"),
			wxDefaultPosition,
			wxDefaultSize,
			wxTE_NOHIDESEL | wxTE_PROCESS_ENTER);
		privateKey.SetEditable(false);
		box->Add(&privateKey, fieldFlags);
		contentBox->Add(box.get(), fieldFlags);
		box.release();
	}

	if (!m_isGenerateMode) {

		contentBox->Add(
			new wxStaticText(this, wxID_ANY, wxT("Serial number:")),
			labelFlags);
		wxTextCtrl &serial = *new wxTextCtrl(
			this,
			wxID_ANY,
			wxString::FromAscii(m_certificate.serial.c_str()),
			wxDefaultPosition,
			wxDefaultSize,
			wxTE_NOHIDESEL | wxTE_PROCESS_ENTER);
		serial.SetEditable(false);
		serial.Enable(!serial.GetValue().IsEmpty());
		serial.SetMinSize(fieldSize);
		contentBox->Add(&serial, fieldFlags);

		contentBox->Add(
			new wxStaticText(this, wxID_ANY, wxT("Validity period:")),
			labelFlags);
		wxString validityStr
			= wxDateTime(m_certificate.validAfterTimeUtc).Format(wxT("%d.%m.%Y %H:%M"));
		validityStr += wxT(" - ");
		validityStr
			+= wxDateTime(m_certificate.validBeforeTimeUtc).Format(wxT("%d.%m.%Y %H:%M"));
		wxTextCtrl &validity = *new wxTextCtrl(
			this,
			wxID_ANY,
			validityStr,
			wxDefaultPosition,
			wxDefaultSize,
			wxTE_NOHIDESEL | wxTE_PROCESS_ENTER);
		validity.SetEditable(m_isGenerateMode);
		validity.SetMinSize(fieldSize);
		contentBox->Add(&validity, fieldFlags);

	}

	if (!m_isGenerateMode) {
		contentBox->Add(
			new wxStaticText(this, wxID_ANY, wxT("Issued by:")),
			labelFlags);
		m_issuerCommonName = new wxTextCtrl(
			this,
			wxID_ANY,
			wxString::FromAscii(m_certificate.issuerCommonName.c_str()),
			wxDefaultPosition,
			wxDefaultSize,
			wxTE_NOHIDESEL | wxTE_PROCESS_ENTER);
		m_issuerCommonName->SetEditable(false);
		m_issuerCommonName->Enable(!m_issuerCommonName->GetValue().IsEmpty());
		m_issuerCommonName->SetMinSize(fieldSize);
		contentBox->Add(m_issuerCommonName, fieldFlags);
	}

	contentBox->Add(
		new wxStaticText(
			this,
			wxID_ANY,
			m_isGenerateMode
				?	wxT("Common name:")
				:	wxT("Issued to:")),
		labelFlags);
	m_subjectCommonName = new wxTextCtrl(
		this,
		wxID_ANY,
		wxString::FromAscii(m_certificate.subjectCommonName.c_str()),
		wxDefaultPosition,
		wxDefaultSize,
		wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
		wxTextValidator(wxFILTER_ASCII));
	m_subjectCommonName->SetEditable(m_isGenerateMode);
	m_subjectCommonName->Enable(m_isGenerateMode || !m_subjectCommonName->GetValue().IsEmpty());
	m_subjectCommonName->SetMinSize(fieldSize);
	contentBox->Add(m_subjectCommonName, fieldFlags);

	contentBox->Add(
		new wxStaticText(this, wxID_ANY, wxT("Organization:")),
		labelFlags);
	m_organization = new wxTextCtrl(
		this,
		wxID_ANY,
		wxString::FromAscii(m_certificate.subjectOrganization.c_str()),
		wxDefaultPosition,
		wxDefaultSize,
		wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
		wxTextValidator(wxFILTER_ASCII));
	m_organization->SetEditable(m_isGenerateMode);
	m_organization->SetMinSize(fieldSize);
	m_organization->Enable(m_isGenerateMode || !m_organization->GetValue().IsEmpty());
	contentBox->Add(m_organization, fieldFlags);

	contentBox->Add(
		new wxStaticText(this, wxID_ANY, wxT("Organization unit:")),
		labelFlags);
	m_organizationUnit = new wxTextCtrl(
		this,
		wxID_ANY,
		wxString::FromAscii(m_certificate.subjectOrganizationUnit.c_str()),
		wxDefaultPosition,
		wxDefaultSize,
		wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
		wxTextValidator(wxFILTER_ASCII));
	m_organizationUnit->SetEditable(m_isGenerateMode);
	m_organizationUnit->SetMinSize(fieldSize);
	m_organizationUnit->Enable(m_isGenerateMode || !m_organizationUnit->GetValue().IsEmpty());
	contentBox->Add(m_organizationUnit, fieldFlags);

	contentBox->Add(
		new wxStaticText(this, wxID_ANY, wxT("City:")),
		labelFlags);
	m_city = new wxTextCtrl(
		this,
		wxID_ANY,
		wxString::FromAscii(m_certificate.subjectCity.c_str()),
		wxDefaultPosition,
		wxDefaultSize,
		wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
		wxTextValidator(wxFILTER_ASCII));
	m_city->SetEditable(m_isGenerateMode);
	m_city->SetMinSize(fieldSize);
	m_city->Enable(m_isGenerateMode || !m_city->GetValue().IsEmpty());
	contentBox->Add(m_city, fieldFlags);

	contentBox->Add(
		new wxStaticText(this, wxID_ANY, wxT("State or province:")),
		labelFlags);

	{
		std::auto_ptr<wxBoxSizer> stateBox(new wxBoxSizer(wxHORIZONTAL));
		m_stateOrProvince = new wxTextCtrl(
			this,
			wxID_ANY,
			wxString::FromAscii(m_certificate.subjectStateOrProvince.c_str()),
			wxDefaultPosition,
			wxDefaultSize,
			wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
			wxTextValidator(wxFILTER_ASCII));
		m_stateOrProvince->SetEditable(m_isGenerateMode);
		m_stateOrProvince->Enable(m_isGenerateMode || !m_stateOrProvince->GetValue().IsEmpty());
		stateBox->Add(m_stateOrProvince, fieldFlags);
		stateBox->AddSpacer(theme.GetDlgBorder() / 2);
		stateBox->Add(
			new wxStaticText(this, wxID_ANY, wxT("Country code:")),
			labelFlags);
		m_country = new wxTextCtrl(
			this,
			CONTROL_COUNTRY,
			wxString::FromAscii(m_certificate.subjectCountry.c_str()),
			wxDefaultPosition,
			wxDefaultSize,
			wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
			wxTextValidator(wxFILTER_ASCII));
		m_country->SetMaxLength(2);
		m_country->SetEditable(m_isGenerateMode);
		m_country->Enable(m_isGenerateMode || !m_country->GetValue().IsEmpty());
		stateBox->Add(m_country, wxSizerFlags(0).Expand());
		contentBox->Add(stateBox.get(), wxSizerFlags(1).Expand());
		stateBox.release();
	}

	if (m_isGenerateMode) {
		contentBox->Add(
			new wxStaticText(this, wxID_ANY, wxT("Key for signing:")),
			labelFlags);
		std::auto_ptr<wxBoxSizer> box(new wxBoxSizer(wxHORIZONTAL));
		m_signKeyPath = new wxTextCtrl(
			this,
			wxID_ANY,
			wxEmptyString,
			wxDefaultPosition,
			wxDefaultSize,
			wxTE_NOHIDESEL | wxTE_PROCESS_ENTER);
		box->Add(m_signKeyPath, wxSizerFlags(1).Center());
		box->Add(theme.GetDlgBorder(), 0);
		box->Add(
			new wxButton(this, CONTROL_BROWSE_SIGN_KEY, wxT("Browse...")),
			wxSizerFlags(0).Center());
		contentBox->Add(box.get(), wxSizerFlags(1).Expand());
		box.release();
	}

	std::auto_ptr<wxBoxSizer> topBox(new wxBoxSizer(wxVERTICAL));

	topBox->Add(contentBox.get(), theme.GetTopSizerFlags());
	contentBox.release();

	if (!m_isGenerateMode) {
		wxTextCtrl &certificateText = *new wxTextCtrl(
			this,
			wxID_ANY,
			wxString::FromAscii(m_certificate.fullInfo.c_str()),
			wxDefaultPosition,
			wxDefaultSize,
			wxTE_NOHIDESEL | wxTE_MULTILINE | wxTE_DONTWRAP);
		certificateText.SetEditable(false);
		certificateText.SetMinSize(wxSize(-1, 130));
		certificateText.Enable(!certificateText.GetValue().IsEmpty());
		topBox->Add(&certificateText, theme.GetTopSizerFlags());
	}

	topBox->Add(new wxStaticLine(this), theme.GetTopSizerFlags());
	
	if (m_isGenerateMode) {
		topBox->Add(
			CreateButtonSizer(wxOK | wxHELP | wxCANCEL),
			theme.GetTopSizerFlags());
		boost::polymorphic_downcast<wxButton *>(FindWindow(wxID_OK))
			->SetLabel(wxT("Generate"));
	} else {
		std::auto_ptr<wxBoxSizer> box(new wxBoxSizer(wxHORIZONTAL));
		box->Add(
			new wxButton(this, CONTROL_SAVE_TO_FILE, wxT("Save to file")),
			wxSizerFlags(0).Expand().Left());
		box->AddStretchSpacer(1);
		box->Add(
			CreateButtonSizer(wxOK | wxHELP),
			wxSizerFlags(0).Expand().Right());
		topBox->Add(box.get(), theme.GetTopSizerFlags());
		box.release();
	}
	topBox->AddSpacer(theme.GetDlgBottomBorder());

	topBox->SetMinSize(GetMinSize());
	SetSizer(topBox.get());
	topBox.release()->SetSizeHints(this);
	Center();

}

void SslCertificateDlg::OnHelp(wxCommandEvent &) {
	m_isGenerateMode
		?	wxGetApp().DisplayHelp(wxT("dialogs/ssl-certificates/view"))
		:	wxGetApp().DisplayHelp(wxT("dialogs/ssl-certificates/generate"));
}

void SslCertificateDlg::OnOk(wxCommandEvent &) {
	
	if (!m_isGenerateMode) {
		EndModal(wxID_CANCEL);
		return;
	}
	
	if (!Validate()) {
		return;
	}

	std::string signKey;
	if (!m_signKeyPath->GetValue().IsEmpty()) {
		std::ifstream f(m_signKeyPath->GetValue().c_str(), std::ios::binary);
		if (!f) {
			wxLogError(wxT("Could not open %s."), m_signKeyPath->GetValue());
			return;
		}
		f.seekg(0, std::ios::end);
		std::vector<char> buffer;
		buffer.resize(f.tellg());
		f.seekg(0, std::ios::beg);
		f.read(&buffer[0], buffer.size());
		std::string(buffer.begin(), buffer.end()).swap(signKey);
	} else {
		signKey.clear();
	}

	texs__SslCertificateInfo certificate;
	certificate.keySize
		= boost::lexical_cast<int>(m_keySize->GetStringSelection().c_str());
	certificate.subjectCommonName
		= certificate.issuerCommonName
		= m_subjectCommonName->GetValue().ToAscii();
	certificate.subjectOrganization
		= certificate.subjectOrganization
		= m_organization->GetValue().ToAscii();
	certificate.subjectOrganizationUnit
		= certificate.subjectOrganizationUnit
		= m_organizationUnit->GetValue().ToAscii();
	certificate.subjectCity
		= certificate.subjectCity
		= m_city->GetValue().ToAscii();
	certificate.subjectStateOrProvince
		= certificate.subjectStateOrProvince
		= m_stateOrProvince->GetValue().ToAscii();
	certificate.subjectCountry
		= certificate.subjectCountry
		= m_country->GetValue().ToAscii();

	signKey.swap(m_signKey);
	std::swap(certificate, m_certificate);
	EndModal(wxID_OK);

}

void SslCertificateDlg::OnCountryText(wxCommandEvent &) {
	if (!m_country) {
		return;
	}
	if (!m_country->GetValue().IsWord()) {
		const long pos = std::max(long(0), m_country->GetInsertionPoint() - 1);
		m_country->ChangeValue(m_validCountry);
		m_country->SetInsertionPoint(std::min(m_country->GetLastPosition(), pos));
	} else {
		wxString val = m_country->GetValue();
		val.UpperCase();
		if (val != m_country->GetValue()) {
			const long pos = m_country->GetInsertionPoint();
			m_country->ChangeValue(val);
			m_country->SetInsertionPoint(pos);
		}
		m_validCountry = m_country->GetValue();
	}
}

void SslCertificateDlg::OnBrowseSignKey(wxCommandEvent &) {
	wxFileDialog fileRequestDlg(
		this,
		wxT("Choose a private key"),
		wxEmptyString,
		wxEmptyString,
		wxT("Privacy Enhanced Mail (*.pem)|*.pem|All files (*.*)|*.*"),
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fileRequestDlg.ShowModal() == wxID_OK) {
		m_signKeyPath->SetValue(fileRequestDlg.GetPath());
	}
}

void SslCertificateDlg::OnSaveToFile(wxCommandEvent &) {
	wxFileDialog fileRequestDlg(
		this,
		wxT("Choose a file to save"),
		wxEmptyString,
		wxEmptyString,
		wxT("Certificates X.509 files (*.cer; *.crt)|*.cer;*.crt")
			wxT("|All files (*.*)|*.*"),
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fileRequestDlg.ShowModal() != wxID_OK) {
		return;
	}
	std::ofstream f(fileRequestDlg.GetPath().c_str(), std::ios::trunc | std::ios::binary);
	if (!f) {
		wxLogError(wxT("Could not open \"%s\" to save."), fileRequestDlg.GetPath());
		return;
	}
	f << m_certificate.certificate;
}
