/**************************************************************************
 *   Created: 2010/11/15 21:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "SslGenerationResultDlg.hpp"
#include "Application.hpp"

using namespace TunnelEx;

enum SslGenerationResultDlg::Control {
	CONTROL_SAVE_PRIVATE_KEY,
	CONTROL_COPY_PRIVATE_KEY,
	CONTROL_SAVE_CONTENT,
	CONTROL_COPY_CONTENT
};

BEGIN_EVENT_TABLE(SslGenerationResultDlg, wxDialog)
	EVT_BUTTON(wxID_OK, SslGenerationResultDlg::OnOk)
	EVT_BUTTON(wxID_CANCEL, SslGenerationResultDlg::OnCancel)
	EVT_BUTTON(wxID_HELP, SslGenerationResultDlg::OnHelp)
	EVT_BUTTON(
		SslGenerationResultDlg::CONTROL_SAVE_PRIVATE_KEY,
		SslGenerationResultDlg::OnSavePrivateKey)
	EVT_BUTTON(
		SslGenerationResultDlg::CONTROL_COPY_PRIVATE_KEY,
		SslGenerationResultDlg::OnCopyPrivateKey)
	EVT_BUTTON(
		SslGenerationResultDlg::CONTROL_SAVE_CONTENT,
		SslGenerationResultDlg::OnSaveContent)
	EVT_BUTTON(
		SslGenerationResultDlg::CONTROL_COPY_CONTENT,
		SslGenerationResultDlg::OnCopyContent)
	EVT_CLOSE(SslGenerationResultDlg::OnClose)
END_EVENT_TABLE()

SslGenerationResultDlg::SslGenerationResultDlg(
			const Mode mode,
			const wxString &privateKey,
			const wxString &content,
			wxWindow *parent,
			wxWindowID id /*= wxID_ANY*/)
		: wxDialog(parent, id, wxT("SSL Certificate Signing Request")),
		m_mode(mode),
		m_isKeySaved(false),
		m_isClosing(false) {
	assert(!privateKey.IsEmpty());
	assert(!content.IsEmpty());
	CreateControls(privateKey, content);
}

SslGenerationResultDlg::~SslGenerationResultDlg() {
	//...//
}

void SslGenerationResultDlg::OnHelp(wxCommandEvent &) {
	wxGetApp().DisplayHelp(wxT("dialogs/ssl-certificates/request"));
}

void SslGenerationResultDlg::OnSavePrivateKey(wxCommandEvent &) {
	wxFileDialog fileRequestDlg(
		const_cast<SslGenerationResultDlg *>(this),
		wxT("Choose a file to save"),
		wxEmptyString,
		wxEmptyString,
		wxT("Privacy Enhanced Mail (*.pem)|*.pem"),
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fileRequestDlg.ShowModal() != wxID_OK) {
		return;
	}
	std::ofstream f(fileRequestDlg.GetPath().c_str(), std::ios::trunc | std::ios::binary);
	if (!f) {
		wxLogError(wxT("Could not open \"%s\" to save."), fileRequestDlg.GetPath());
		return;
	}
	f << m_key->GetValue().ToAscii();
	m_isKeySaved = true;
}

void SslGenerationResultDlg::OnCopyPrivateKey(wxCommandEvent &) {
	Copy(*m_key);
}

void SslGenerationResultDlg::OnSaveContent(wxCommandEvent &) {

	wxString filter;
	switch (m_mode) {
		case MODE_CERTIFICATE:
			filter
				= wxT("Certificates X.509 files (*.cer; *.crt)|*.cer;*.crt")
					wxT("|Personal Information Exchange (*.pfx; *.p12)|*.pfx;*.p12");
			break;
		case MODE_REQUEST:
			filter = wxT("Certificate Signing Request (*.csr)|*.csr");
			break;
		default:
			assert(false);
	}

	wxFileDialog fileRequestDlg(
		const_cast<SslGenerationResultDlg *>(this),
		wxT("Choose a file to save"),
		wxEmptyString,
		wxEmptyString,
		filter,
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fileRequestDlg.ShowModal() != wxID_OK) {
		return;
	}
	
	wxString lowerName = fileRequestDlg.GetPath();
	lowerName.MakeLower();
	const bool isPkcs12
		= m_mode == MODE_CERTIFICATE
			&& (lowerName.EndsWith(wxT(".pfx")) || lowerName.EndsWith(wxT(".p12")));
	
	if (!isPkcs12) {
		std::ofstream f(fileRequestDlg.GetPath().c_str(), std::ios::trunc | std::ios::binary);
		if (!f) {
			wxLogError(wxT("Could not open \"%s\" to save."), fileRequestDlg.GetPath());
			return;
		}
		f << m_content->GetValue().ToAscii();
	} else {
		using namespace TunnelEx::Helpers::Crypto;
		try {
			PrivateKey privateKey(std::string(m_key->GetValue().ToAscii()));
			const std::string certificateStr = m_content->GetValue().ToAscii();
			X509Private x509(
				reinterpret_cast<const unsigned char *>(certificateStr.c_str()),
				certificateStr.size(),
				privateKey);
			std::ofstream f(fileRequestDlg.GetPath().c_str(), std::ios::trunc | std::ios::binary);
			if (!f) {
				wxLogError(wxT("Could not open \"%s\" to save."), fileRequestDlg.GetPath());
				return;
			}
			const wxString password = wxGetPasswordFromUser(
				wxT("Please provide certificate password (optional)."),
				wxT("Certificate password"),
				wxEmptyString,
				this);
			if (!password.IsEmpty()) {
				const wxString passwordConfirm = wxGetPasswordFromUser(
					wxT("Please confirm certificate password."),
					wxT("Certificate password"),
					wxEmptyString,
					this);
				if (passwordConfirm != password) {
					wxMessageBox(
						wxT("Password does not match the confirm password."),
						wxT("Certificate password"),
						wxOK | wxCENTER | wxICON_ERROR,
						this);
					return;
				}
			}
			Pkcs12(x509, std::string(), std::string(password.ToAscii())).Export(f);
			f.close();
			m_isKeySaved = true;
		} catch (const TunnelEx::Helpers::Crypto::OpenSslException &ex) {
			wxLogError(wxString::FromAscii(ex.what()));
			return;
		}
	}

}

void SslGenerationResultDlg::OnCopyContent(wxCommandEvent &) {
	Copy(*m_content);
}

void SslGenerationResultDlg::Copy(const wxTextCtrl &ctrl) const {
	if (!wxTheClipboard->Open()) {
		assert(false);
		return;
	};
	struct AutoCloseClipboard {
		~AutoCloseClipboard() {
			wxTheClipboard->Close();
		}
	} autoCloseClipboard;
	wxTheClipboard->SetData(new wxTextDataObject(ctrl.GetValue()));
}

bool SslGenerationResultDlg::CheckClose(bool isOk) const {

	if (m_isClosing) {
		return true;
	}
	
	if (m_mode == MODE_CERTIFICATE && !isOk) {
		m_isClosing = wxMessageBox(
				wxT("The certificate was not installed. Are you sure you want to close?"),
				wxT("Close"),
				wxYES_NO | wxCENTER | wxICON_WARNING,
				const_cast<SslGenerationResultDlg *>(this))
			== wxYES;
	} else {
		m_isClosing
			= m_isKeySaved
			|| wxMessageBox(
					wxT("The private key is not saved. Are you sure you want to close?"),
					m_mode == MODE_CERTIFICATE ? wxT("Install") : wxT("Close"),
					wxYES_NO | wxCENTER | wxICON_WARNING,
					const_cast<SslGenerationResultDlg *>(this))
				== wxYES;
	}
	
	return m_isClosing;

}

void SslGenerationResultDlg::OnOk(wxCommandEvent &) {
	if (!CheckClose(true)) {
		return;
	}
	EndModal(wxID_OK);
}

void SslGenerationResultDlg::OnCancel(wxCommandEvent &) {
	if (!CheckClose(false)) {
		return;
	}
	EndModal(wxID_CANCEL);
}

void SslGenerationResultDlg::OnClose(wxCloseEvent &closeEvent) {
	if (!closeEvent.CanVeto()) {
		closeEvent.Skip();
		return;
	}
	if (!CheckClose(false)) {
		return;
	}
	closeEvent.Skip();
}

void SslGenerationResultDlg::CreateControls(
			const wxString &privateKey,
			const wxString &request) {

	wxString privateKeyHint;
	wxString contentGroupHint;
	wxString contentGroupLabel;
	switch (m_mode) {
		case MODE_CERTIFICATE:
			SetTitle(wxT("SSL Certificate"));
			privateKeyHint
				= wxT("Do not disclose this key to anyone. Save it into a file")
					wxT(" and carefully protect.");
			contentGroupLabel = wxT("Certificate");
			break;
		case MODE_REQUEST:
			SetTitle(wxT("SSL Certificate Signing Request"));
			privateKeyHint
				= wxT("Do not disclose this key to anyone. Save it into a file")
					wxT(" and carefully protect. To install requested")
					wxT(" certificate use this private key.");
			contentGroupHint
				= wxT("Send this Certificate Signing Request (CSR) to")
					wxT(" a Certifying Authority (CA).");
			contentGroupLabel = wxT("Request");
			break;
		default:
			assert(false);
	}

	const Theme &theme = wxGetApp().GetTheme();

	const int width = 460;
	const int keyHeight = 150;

	std::auto_ptr<wxBoxSizer> topBox(new wxBoxSizer(wxVERTICAL));
	{
		std::auto_ptr<wxBoxSizer> box(new wxBoxSizer(wxVERTICAL));
		wxStaticText &text = *new wxStaticText(this, wxID_ANY, privateKeyHint);
		text.SetMinSize(wxSize(width, -1));
		text.Wrap(width);
		box->Add(&text, wxSizerFlags(0).Expand());
		box->AddSpacer(theme.GetDlgBorder());
		m_key = new wxTextCtrl(
			this,
			wxID_ANY,
			privateKey,
			wxDefaultPosition,
			wxDefaultSize,
			wxTE_MULTILINE | wxTE_DONTWRAP);
		m_key->SetEditable(false);
		m_key->SetMinSize(wxSize(width, keyHeight));
		box->Add(m_key, wxSizerFlags(1).Expand());
		box->AddSpacer(theme.GetDlgBorder());
		{
			std::auto_ptr<wxBoxSizer> buttonsSizer(new wxBoxSizer(wxHORIZONTAL));
			buttonsSizer->AddStretchSpacer(1);
			buttonsSizer->Add(
				new wxButton(
					this,
					CONTROL_COPY_PRIVATE_KEY,
					wxT("Copy")));
			buttonsSizer->AddSpacer(theme.GetDlgBorder() / 2);
			buttonsSizer->Add(
				new wxButton(
					this,
					CONTROL_SAVE_PRIVATE_KEY,
					wxT("Save")));
			box->Add(buttonsSizer.get(), wxSizerFlags(0).Expand());
			buttonsSizer.release();
		}
		wxStaticBoxSizer &group = *new wxStaticBoxSizer(
			wxVERTICAL,
			this,
			wxT("Private key"));
		group.Add(box.get(), theme.GetStaticBoxFlags());
		box.release();
		topBox->Add(&group, theme.GetTopSizerFlags());
	}

	{
		std::auto_ptr<wxBoxSizer> box(new wxBoxSizer(wxVERTICAL));
		if (!contentGroupHint.IsEmpty()) {
			wxStaticText &text = *new wxStaticText(this, wxID_ANY, contentGroupHint);
			text.SetMinSize(wxSize(width, -1));
			text.Wrap(width);
			box->Add(&text, wxSizerFlags(0).Expand());
			box->AddSpacer(theme.GetDlgBorder());
		}
		m_content = new wxTextCtrl(
			this,
			wxID_ANY,
			request,
			wxDefaultPosition,
			wxDefaultSize,
			wxTE_MULTILINE | wxTE_DONTWRAP);
		m_content->SetEditable(false);
		m_content->SetMinSize(wxSize(width, keyHeight));
		box->Add(m_content, wxSizerFlags(1).Expand());
		box->AddSpacer(theme.GetDlgBorder());
		{
			std::auto_ptr<wxBoxSizer> buttonsSizer(new wxBoxSizer(wxHORIZONTAL));
			buttonsSizer->AddStretchSpacer(1);
			buttonsSizer->Add(
				new wxButton(
					this,
					CONTROL_COPY_CONTENT,
					wxT("Copy")));
			buttonsSizer->AddSpacer(theme.GetDlgBorder() / 2);
			buttonsSizer->Add(
				new wxButton(
					this,
					CONTROL_SAVE_CONTENT,
					wxT("Save")));
			box->Add(buttonsSizer.get(), wxSizerFlags(0).Expand());
			buttonsSizer.release();
		}
		wxStaticBoxSizer &group = *new wxStaticBoxSizer(
			wxVERTICAL,
			this,
			contentGroupLabel);
		group.Add(box.get(), theme.GetStaticBoxFlags());
		box.release();
		topBox->Add(&group, theme.GetTopSizerFlags());
	}
	
	topBox->Add(new wxStaticLine(this), theme.GetTopSizerFlags());
	int buttons = wxOK | wxHELP;
	if (m_mode == MODE_CERTIFICATE) {
		buttons |= wxCANCEL;
	}
	topBox->Add(CreateButtonSizer(buttons), theme.GetTopSizerFlags());
	if (m_mode == MODE_CERTIFICATE) {
		FindWindow(wxID_OK)->SetLabel(wxT("Install"));
	}
	topBox->AddSpacer(theme.GetDlgBottomBorder());

	topBox->SetMinSize(GetMinSize());
	SetSizer(topBox.get());
	topBox.release()->SetSizeHints(this);
	Center();

}
