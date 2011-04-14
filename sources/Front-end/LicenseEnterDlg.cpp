/**************************************************************************
 *   Created: 2009/12/06 23:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: LicenseEnterDlg.cpp 1033 2010-10-15 20:27:41Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "Application.hpp"
#include "LicenseEnterDlg.hpp"
#include "Validators.hpp"
#include "ServiceAdapter.hpp"
#include "LicensePolicies.hpp"

using namespace TunnelEx;
using namespace TunnelEx::Licensing;

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(LicenseKeyDlg, wxDialog)
	EVT_BUTTON(wxID_OK, LicenseKeyDlg::OnOk)
	EVT_BUTTON(LicenseKeyDlg::CTRL_PASTE, LicenseKeyDlg::OnPaste)
END_EVENT_TABLE()

LicenseKeyDlg::LicenseKeyDlg(
			ServiceAdapter &service,
			wxWindow *parent,
			wxWindowID id)
		: wxDialog(
			parent,
			id,
			wxT("Offline activation"),
			wxDefaultPosition,
			wxSize(450, 0),
			wxCAPTION | wxCLOSE_BOX | wxSYSTEM_MENU),
		m_service(service) {
	const int border = 10;
	wxStaticText &messageCtrl = *new wxStaticText(
		this,
		wxID_ANY,
		wxT("Insert activation code in the field below:"),
		wxPoint(border, border),
		wxSize(GetClientSize().GetWidth() - (border * 2), -1));
	m_licenseKeyCtrl = new wxTextCtrl(
		this,
		wxID_ANY,
		wxEmptyString,
		wxPoint(
			border,
			messageCtrl.GetPosition().y + messageCtrl.GetSize().GetHeight() + border),
		wxSize(GetClientSize().GetWidth() - (border * 2), 300),
		wxTE_NOHIDESEL | wxTE_MULTILINE | wxTE_DONTWRAP,
		LicenseKeyValidator(true));
	const int buttonWidth = 80;
	new wxButton(
		this,
		CTRL_PASTE,
		wxT("Paste"),
		wxPoint(
			border,
			m_licenseKeyCtrl->GetPosition().y + m_licenseKeyCtrl->GetSize().GetHeight() + border),
		wxSize(buttonWidth, -1));
	new wxButton(
		this,
		wxID_OK,
		wxT("OK"),
		wxPoint(
			GetClientSize().GetWidth() - border - (border / 2) - (buttonWidth * 2),
			m_licenseKeyCtrl->GetPosition().y + m_licenseKeyCtrl->GetSize().GetHeight() + border),
		wxSize(buttonWidth, -1));
	const wxButton &closeButton = *new wxButton(
		this,
		wxID_CANCEL,
		wxT("Cancel"),
		wxPoint(
			GetClientSize().GetWidth() - border - buttonWidth,
			m_licenseKeyCtrl->GetPosition().y + m_licenseKeyCtrl->GetSize().GetHeight() + border),
		wxSize(buttonWidth, -1));
	SetClientSize(
		GetClientSize().GetWidth(),
		closeButton.GetPosition().y + closeButton.GetSize().GetHeight() +  border);
}

void LicenseKeyDlg::OnPaste(wxCommandEvent &) {
	if (!wxTheClipboard->Open()) {
		BOOST_ASSERT(false);
		return;
	};
	struct AutoCloseClipboard {
		~AutoCloseClipboard() {
			wxTheClipboard->Close();
		}
	} autoCloseClipboard;
	if (wxTheClipboard->IsSupported(wxDF_TEXT)) {
		wxTextDataObject data;
		wxTheClipboard->GetData(data);
		m_licenseKeyCtrl->SetValue(data.GetText());
	}  
}

void LicenseKeyDlg::OnOk(wxCommandEvent &) {
	if (!Validate()) {
		return;
	}
	BOOST_ASSERT(
		wxGetApp().GetConfig().Exists(wxT("/License/OfflineActivation/State")));
	const wxString privateKeySerialized
		= wxGetApp().GetConfig().Read(wxT("/License/OfflineActivation/State"));
	BOOST_ASSERT(!privateKeySerialized.IsEmpty());
	if (!privateKeySerialized.IsEmpty()) {
		std::string privateKey;
		{
			std::vector<unsigned char> encryptedPrivateKey;
			TunnelEx::Helpers::StringUtil::AsciiToBin(
					privateKeySerialized.ToAscii(),
					privateKeySerialized.size(),
					encryptedPrivateKey);
			std::vector<unsigned char> key;
			LicenseDataEncryption::GetOfflineActivationPrivateKeyEncryptingKey(key);
			size_t token = 0;
			foreach (unsigned char ch, encryptedPrivateKey) {
				ch ^= key[token++ % key.size()];
				privateKey.push_back(ch);
			}
		}
		const std::string licenseKey = m_licenseKeyCtrl->GetValue().ToAscii();
		OfflineKeyRequest request(
			licenseKey,
			privateKey,
			LicenseState(m_service));
		if (request.TestKey<InfoDlgLicense>()) {
			request.Accept();
			EndModal(wxID_OK);
			return;
		}
	}
	wxLogError(
		wxT("Unknown error at license activation.")
			wxT(" Please make sure that you have entered")
			wxT(" a correct activation code and try again."));
	EndModal(wxID_CANCEL);
}

//////////////////////////////////////////////////////////////////////////

class LicenseKeyRequestDlg : public wxDialog {
private:
	enum Control {
		CTRL_COPY
	};
public:
	explicit LicenseKeyRequestDlg(
				const wxString &request,
				wxWindow *parent,
				wxWindowID id = wxID_ANY)
			: wxDialog(
				parent,
				id,
				wxT("Offline activation"),
				wxDefaultPosition,
				wxSize(450, 0),
				wxCAPTION | wxCLOSE_BOX | wxSYSTEM_MENU) {
		const int border = 10;
		wxStaticText &messageCtrl = *new wxStaticText(
			this,
			wxID_ANY,
			wxT("Please send the code below to support@") TUNNELEX_DOMAIN_W wxT(".")
				wxT(" An activation code will be send in answer message."),
			wxPoint(border, border),
			wxSize(GetClientSize().GetWidth() - (border * 2), -1));
		messageCtrl.Wrap(GetClientSize().GetWidth() - (border * 2));
		m_requestCtrl = new wxTextCtrl(
			this,
			wxID_ANY,
			request,
			wxPoint(
				border,
				messageCtrl.GetPosition().y + messageCtrl.GetSize().GetHeight() + border),
			wxSize(GetClientSize().GetWidth() - (border * 2), 300),
			wxTE_NOHIDESEL | wxTE_MULTILINE | wxTE_DONTWRAP);
		m_requestCtrl->SetEditable(false);
		m_requestCtrl->SelectAll();
		const int buttonWidth = 80;
		new wxButton(
			this,
			CTRL_COPY,
			wxT("Copy"),
			wxPoint(
				GetClientSize().GetWidth() - border - (border / 2) - (buttonWidth * 2),
				m_requestCtrl->GetPosition().y + m_requestCtrl->GetSize().GetHeight() + border),
			wxSize(buttonWidth, -1));
		const wxButton &closeButton = *new wxButton(
			this,
			wxID_CANCEL,
			wxT("Close"),
			wxPoint(
				GetClientSize().GetWidth() - border - buttonWidth,
				m_requestCtrl->GetPosition().y + m_requestCtrl->GetSize().GetHeight() + border),
			wxSize(buttonWidth, -1));
		SetClientSize(
			GetClientSize().GetWidth(),
			closeButton.GetPosition().y + closeButton.GetSize().GetHeight() +  border);
	}
	DECLARE_NO_COPY_CLASS(LicenseKeyRequestDlg);
public:
	void OnCopy(wxCommandEvent &) {
		if (!wxTheClipboard->Open()) {
			BOOST_ASSERT(false);
			return;
		};
		struct AutoCloseClipboard {
			~AutoCloseClipboard() {
				wxTheClipboard->Close();
			}
		} autoCloseClipboard;
		m_requestCtrl->SelectAll();
		wxTheClipboard->SetData(
			new wxTextDataObject(
				m_requestCtrl->GetValue()));
	}
	DECLARE_EVENT_TABLE();
private:
	wxTextCtrl *m_requestCtrl;
};

BEGIN_EVENT_TABLE(LicenseKeyRequestDlg, wxDialog)
	EVT_BUTTON(LicenseKeyRequestDlg::CTRL_COPY, LicenseKeyRequestDlg::OnCopy)
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(LicenseEnterDlg, wxDialog)
	EVT_BUTTON(LicenseEnterDlg::CTRL_ACTIVATE_ONLINE, LicenseEnterDlg::OnOnlineActivation)
	EVT_BUTTON(LicenseEnterDlg::CTRL_ACTIVATE_OFFLINE, LicenseEnterDlg::OnOfflineActivation)
END_EVENT_TABLE()

LicenseEnterDlg::LicenseEnterDlg(
			ServiceAdapter &service,
			wxWindow *parent,
			wxWindowID id)
		: wxDialog(
			parent,
			id,
			wxT("Activation"),
			wxDefaultPosition,
			wxSize(335, 50),
			wxCAPTION | wxCLOSE_BOX | wxSYSTEM_MENU),
		m_service(service) {
	const int border = 10;
	const wxTextCtrl &licenseCtrl = *new wxTextCtrl(
		this,
		CTRL_LICENSE,
		wxEmptyString,
		wxPoint(border, border),
		wxSize(GetClientSize().GetWidth() - (border * 2), -1),
		wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
		LicenseUuidValidator(false));
	const int buttonWidth = 100;
	new wxButton(
		this,
		CTRL_ACTIVATE_ONLINE,
		wxT("Activate online"),
		wxPoint(
			GetClientSize().GetWidth() - border - ((border / 2) * 2) - (buttonWidth * 3),
			licenseCtrl.GetPosition().y + licenseCtrl.GetSize().GetHeight() + border),
		wxSize(buttonWidth, -1));
	new wxButton(
		this,
		CTRL_ACTIVATE_OFFLINE,
		wxT("Activate offline"),
		wxPoint(
			GetClientSize().GetWidth() - border - (border / 2) - (buttonWidth * 2),
			licenseCtrl.GetPosition().y + licenseCtrl.GetSize().GetHeight() + border),
		wxSize(buttonWidth, -1));
	const wxButton &cancelButton = *new wxButton(
		this,
		wxID_CANCEL,
		wxT("Cancel"),
		wxPoint(
			GetClientSize().GetWidth() - border - buttonWidth,
			licenseCtrl.GetPosition().y + licenseCtrl.GetSize().GetHeight() + border),
		wxSize(buttonWidth, -1));
	SetClientSize(
		GetClientSize().GetWidth(),
		cancelButton.GetPosition().y + cancelButton.GetSize().GetHeight() +  border);
}

LicenseEnterDlg::~LicenseEnterDlg() {
	//...//
}

void LicenseEnterDlg::TrimLicense() {
	wxString trimed = boost::polymorphic_downcast<wxTextCtrl *>(FindWindow(CTRL_LICENSE))
		->GetValue().Trim(false);
	boost::polymorphic_downcast<wxTextCtrl *>(FindWindow(CTRL_LICENSE))
		->SetValue(trimed.Trim(true));
}

void LicenseEnterDlg::OnOnlineActivation(wxCommandEvent &) {
	TrimLicense();
	if (!Validate()) {
		return;
	}
	OnlineActivation activation;
	activation.Activate(
		std::string(
			boost::polymorphic_downcast<wxTextCtrl *>(FindWindow(CTRL_LICENSE))
				->GetValue().ToAscii()),
		m_service);
	if (!activation.GetActivationResult()) {
		return;
	}
	EndModal(wxID_OK);
}

void LicenseEnterDlg::OnOfflineActivation(wxCommandEvent &) {
	TrimLicense();
	if (!Validate()) {
		return;
	}
	const std::string license
		= boost::polymorphic_downcast<wxTextCtrl *>(FindWindow(CTRL_LICENSE))
			->GetValue().ToAscii();
	OfflineKeyRequest request(license, LicenseState(m_service));
	wxString privateKey;
	{
		std::vector<unsigned char> key;
		std::vector<unsigned char> encrypted;
		LicenseDataEncryption::GetOfflineActivationPrivateKeyEncryptingKey(key);
		size_t token = 0;
		foreach (char ch, request.GetPrivateKey()) {
			ch ^= key[token++ % key.size()];
			encrypted.push_back(ch);
		}
		privateKey = wxString::FromAscii(
			TunnelEx::Helpers::StringUtil::BinToAscii(encrypted).c_str());
	}
	LicenseKeyRequestDlg(
			wxString::FromAscii(request.GetContent().c_str()),
			this)
		.ShowModal();
	wxGetApp().GetConfig().Write(
		wxT("/License/OfflineActivation/State"),
		privateKey);
	EndModal(wxID_CANCEL);
}
