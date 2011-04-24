/**************************************************************************
 *   Created: 2009/12/23 19:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "ProxyDlg.hpp"
#include "Theme.hpp"
#include "RuleUtils.hpp"
#include "Validators.hpp"
#include "Application.hpp"
#include "LicenseRestrictionDlg.hpp"

using namespace TunnelEx;

enum ProxyDlg::Control {
	CONTROL_IS_USING_AUTH,
	CONTROL_CREATE_CASCADE
};

BEGIN_EVENT_TABLE(ProxyDlg, wxDialog)
	EVT_BUTTON(wxID_OK,	ProxyDlg::OnOk)
	EVT_BUTTON(wxID_HELP, ProxyDlg::OnHelp)
	EVT_CHECKBOX(ProxyDlg::CONTROL_IS_USING_AUTH, ProxyDlg::OnAuthUsingToogle)
	EVT_BUTTON(CONTROL_CREATE_CASCADE, ProxyDlg::OnCreateCascade)
END_EVENT_TABLE()

ProxyDlg::ProxyDlg(wxWindow *parent, bool inCascade, bool readOnly)
		: wxDialog(
			parent,
			wxID_ANY,
			GetInitTitle()),
		m_readOnly(readOnly),
		m_isInCascade(inCascade) {
	const Info defaultProxy;
	CreateControls(defaultProxy);
}

ProxyDlg::ProxyDlg(wxWindow *parent, const Info &proxy, bool inCascade, bool readOnly)
		: wxDialog(
			parent,
			wxID_ANY,
			GetInitTitle()),
		m_readOnly(readOnly),
		m_isInCascade(inCascade) {
	CreateControls(proxy);
}

const wxChar * ProxyDlg::GetInitTitle() {
	return wxT("Proxy server settings");
}

ProxyDlg::~ProxyDlg() {
	//...//
}

ProxyDlg::Info ProxyDlg::GetProxy() const {
	Info result;
	result.host = GetHost();
	result.port = GetPort();
	result.isAuthInUse = IsUsingAuth();
	result.user = GetUser();
	result.password = GetPassword();
	return result;
}

wxString ProxyDlg::GetHost() const {
	return m_host->GetValue();
}

wxString ProxyDlg::GetPort() const {
	return m_port->GetValue();
}

bool ProxyDlg::IsUsingAuth() const {
	return m_isAuth->GetValue();
}

wxString ProxyDlg::GetUser() const {
	return m_authUser->GetValue();
}

wxString ProxyDlg::GetPassword() const {
	return m_authPass->GetValue();
}

void ProxyDlg::CreateControls(const Info &proxy) {

	const Theme &theme = wxGetApp().GetTheme();

	std::auto_ptr<wxBoxSizer> topSizer(new wxBoxSizer(wxVERTICAL));

	// host and port: //////////////////////////////////////////////////////
	{
		std::auto_ptr<wxBoxSizer> box(new wxBoxSizer(wxHORIZONTAL));
		wxStaticText *label = new wxStaticText(this, wxID_ANY, wxT("Host:"));
		label->Enable(!m_readOnly);
		box->Add(label, wxSizerFlags(0).Center());
		m_host = new wxTextCtrl(
			this,
			wxID_ANY,
			proxy.host,
			wxDefaultPosition,
			wxDefaultSize,
			wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
			HostValidator(false));
		m_host->Enable(!m_readOnly);
		m_host->SetToolTip(wxT("Proxy server hostname or IP address."));
		box->Add(m_host, wxSizerFlags(1).Center());
		box->AddSpacer(theme.GetDlgBorder());
		label = new wxStaticText(this, wxID_ANY, wxT("Port:"));
		label->Enable(!m_readOnly);
		box->Add(label, wxSizerFlags(0).Center());
		m_port = new wxTextCtrl(
			this,
			wxID_ANY,
			proxy.port.IsEmpty() ? wxT("8080") : proxy.port,
			wxDefaultPosition,
			RuleUtils::GetPortFieldSize(),
			wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
			NetworPortValidator(false));
		m_port->Enable(!m_readOnly);
		m_port->SetToolTip(wxT("Proxy server network port."));
		box->Add(m_port, wxSizerFlags(0).Center());
		wxStaticBoxSizer &group
			= *new wxStaticBoxSizer(wxHORIZONTAL, this, wxT("Connection"));
		group.Add(box.get(), theme.GetStaticBoxFlags());
		box.release();
		topSizer->Add(&group, theme.GetTopSizerFlags());
	}

	// auth: //////////////////////////////////////////////////////////////
	{
		std::auto_ptr<wxBoxSizer> topBox(new wxBoxSizer(wxVERTICAL));
		m_isAuth = new wxCheckBox(
			this,
			CONTROL_IS_USING_AUTH,
			wxT("User authorization"),
			wxDefaultPosition,
			wxDefaultSize,
			wxCHK_2STATE);
		m_isAuth->Enable(!m_readOnly);
		m_isAuth->SetToolTip(wxT("Check it if proxy server requires authorization."));
		m_isAuth->SetValue(proxy.isAuthInUse);
		topBox->Add(m_isAuth);
		std::auto_ptr<wxBoxSizer> paramsBox(new wxBoxSizer(wxHORIZONTAL));
		paramsBox->AddSpacer(theme.GetDlgBorder());
		m_authUserLabel = new wxStaticText(this, wxID_ANY, wxT("User name:"));
		m_authUserLabel->Enable(!m_readOnly && m_isAuth->GetValue());
		paramsBox->Add(m_authUserLabel, wxSizerFlags(0).Center());
		m_authUser = new wxTextCtrl(
			this,
			wxID_ANY,
			proxy.user,
			wxDefaultPosition,
			wxDefaultSize,
			wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
			HttpProxyAuthUserNameValidator(false));
		m_authUser->Enable(!m_readOnly && m_isAuth->GetValue());
		paramsBox->Add(m_authUser,  wxSizerFlags(1).Center());
		m_authPassLabel = new wxStaticText(this, wxID_ANY, wxT("Password:"));
		m_authPassLabel->Enable(!m_readOnly && m_isAuth->GetValue());
		paramsBox->AddSpacer(theme.GetDlgBorder());
		paramsBox->Add(m_authPassLabel,  wxSizerFlags(0).Center());
		m_authPass = new wxTextCtrl(
			this, 
			wxID_ANY,
			proxy.password,
			wxDefaultPosition,
			wxDefaultSize,
			wxTE_NOHIDESEL | wxTE_PROCESS_ENTER | wxTE_PASSWORD,
			HttpProxyAuthPasswordValidator(false));
		m_authPass->Enable(!m_readOnly && m_isAuth->GetValue());
		paramsBox->Add(m_authPass, wxSizerFlags(1).Center());
		topBox->AddSpacer(theme.GetDlgBorder());
		topBox->Add(paramsBox.get(), wxSizerFlags(0).Expand());
		paramsBox.release();
		wxStaticBoxSizer &group
			= *new wxStaticBoxSizer(wxVERTICAL, this, wxT("Authorization"));
		group.Add(topBox.get(), theme.GetStaticBoxFlags());
		topBox.release();
		topSizer->Add(&group, theme.GetTopSizerFlags());
	}

	//////////////////////////////////////////////////////////////////////////

	topSizer->Add(new wxStaticLine(this), theme.GetTopSizerFlags());

	// buttons: ////////////////////////////////////////////////////////////

	if (!m_isInCascade) {
		std::auto_ptr<wxBoxSizer> buttons(new wxBoxSizer(wxHORIZONTAL));
		wxButton *button
			= new wxButton(this, CONTROL_CREATE_CASCADE, wxT("Create proxy cascade"));
		button->Enable(!m_readOnly);
		buttons->Add(button, wxSizerFlags(0).Center().Expand());
		buttons->AddSpacer(theme.GetDlgBorder() * 2);
		buttons->Add(
			CreateButtonSizer(wxOK | wxCANCEL | wxHELP),
			wxSizerFlags(1).Center().Right());
		topSizer->Add(buttons.get(), theme.GetTopSizerFlags());
		buttons.release();
	} else {
		topSizer->Add(
			CreateButtonSizer(wxOK | wxCANCEL | wxHELP),
			theme.GetTopSizerFlags().Right());
	}
	FindWindow(wxID_OK)->Enable(!m_readOnly);

	//////////////////////////////////////////////////////////////////////////

	topSizer->AddSpacer(theme.GetDlgBottomBorder());

	SetSizer(topSizer.get());
	topSizer.release()->SetSizeHints(this);
	Center();

}

void ProxyDlg::OnHelp(wxCommandEvent &) {
	wxGetApp().DisplayHelp(wxT("dialogs/proxy-settings"));
}

void ProxyDlg::OnAuthUsingToogle(wxCommandEvent &) {
	const bool enable = IsUsingAuth();
	m_authUserLabel->Enable(enable);
	m_authUser->Enable(enable);
	m_authPassLabel->Enable(enable);
	m_authPass->Enable(enable);
}

void ProxyDlg::OnOk(wxCommandEvent &) {

	if (!Validate()) {
		return;
	} else if (IsUsingAuth()) {
		if (GetUser().IsEmpty()) {
			HttpProxyAuthUserNameValidator::ShowWarning(*m_authUser);
			return;
		} else if (GetPassword().IsEmpty()) {
			HttpProxyAuthPasswordValidator::ShowWarning(*m_authPass);
			return;
		}
	}

	EndModal(wxID_OK);

}

void ProxyDlg::OnCreateCascade(wxCommandEvent &evnt) {
	assert(m_isInCascade == false);
	const int answer = wxMessageBox(
			wxT("Save current changes?"),
			wxT("Proxy server settings edit"),
			wxYES_NO | wxCANCEL | wxICON_QUESTION,
			this);
	switch (answer) {
		default:
			assert(false);
		case wxCANCEL:
			return;
		case wxNO:
			SetReturnCode(0);
			EndModal(wxID_CANCEL);
			break;
		case wxYES:
			SetReturnCode(0);
			OnOk(evnt);
			break;
	}
	if (GetReturnCode() != 0) {
		m_isInCascade = true;
	}
}

//////////////////////////////////////////////////////////////////////////

enum ProxyCascadeDlg::Control {
	CONTROL_LIST,
	CONTROL_MOVE_UP,
	CONTROL_MOVE_DOWN,
	CONTROL_EDIT,
	CONTROL_ADD,
	CONTROL_REMOVE
};

BEGIN_EVENT_TABLE(ProxyCascadeDlg, wxDialog)

	EVT_BUTTON(wxID_OK,	ProxyCascadeDlg::OnOk)
	EVT_BUTTON(wxID_HELP, ProxyCascadeDlg::OnHelp)

	EVT_LISTBOX_DCLICK(ProxyCascadeDlg::CONTROL_LIST, ProxyCascadeDlg::OnEdit)
	EVT_LISTBOX(ProxyCascadeDlg::CONTROL_LIST, ProxyCascadeDlg::OnSelectionChange) 

	EVT_BUTTON(CONTROL_MOVE_UP, ProxyCascadeDlg::OnMoveUp)
	EVT_BUTTON(CONTROL_MOVE_DOWN, ProxyCascadeDlg::OnMoveDown)
	EVT_BUTTON(CONTROL_EDIT, ProxyCascadeDlg::OnEdit)
	EVT_BUTTON(CONTROL_ADD, ProxyCascadeDlg::OnAdd)
	EVT_BUTTON(CONTROL_REMOVE, ProxyCascadeDlg::OnRemove)

END_EVENT_TABLE()

ProxyCascadeDlg::ProxyCascadeDlg(
			ServiceWindow &service,
			wxWindow *parent,
			const Cascade &cascade,
			const Licensing::ProxyCascadeLicense &licenses)
		: wxDialog(
			parent,
			wxID_ANY,
			wxT("Cascade HTTP tunnel settings")),
		m_cascade(cascade) {

	class Validator : public wxValidator {
	public:
		Validator(const wxString &errorText)
				: m_errorText(errorText) {
			//...//
		}
		virtual bool Validate(wxWindow *) {
			wxListBox &listBox = *boost::polymorphic_downcast<wxListBox *>(GetWindow());
			const bool result = listBox.GetCount() > 0;
			if (!result) {
				wxLogWarning(m_errorText);
			}
			return result;
		}
		virtual wxObject * Clone() const {
			return new Validator(m_errorText);
		}
		virtual bool TransferToWindow() {
			return true;
		}
	private:
		const wxString m_errorText;
	} validator(wxT("Please provide one or more proxy server addresses."));

	m_isLicensed = licenses.IsFeatureAvailable(true);
	if (!m_isLicensed) {
		LicenseRestrictionDlg(service, this, licenses, false).ShowModal();
		if (wxGetApp().IsUnlimitedModeActive()) {
			m_isLicensed = true;
		}
	}

	const Theme &theme = wxGetApp().GetTheme();

	//! @todo: Insert tooltips for all controls [2010/09/10 2:09]

	std::auto_ptr<wxBoxSizer> topStaticBox(new wxBoxSizer(wxHORIZONTAL));

	m_listCtrl = new wxListBox(
		this,
		CONTROL_LIST,
		wxDefaultPosition,
		wxDefaultSize,
		wxArrayString(),
		wxLB_EXTENDED,
		validator);
	topStaticBox->Add(m_listCtrl, wxSizerFlags(1).Expand());

	std::auto_ptr<wxBoxSizer> buttonBox(new wxBoxSizer(wxVERTICAL));
	const wxSizerFlags buttonBoxFlags = wxSizerFlags(0).Bottom().Center();

	const wxSize buttonSize(theme.GetIconButtonSize());
	wxBitmap buttonIcon;
	theme.GetArrowUpButton(buttonIcon);
	wxBitmapButton *button;
	m_upButton = button = new wxBitmapButton(
		this,
		CONTROL_MOVE_UP,
		buttonIcon,
		wxDefaultPosition,
		buttonSize);
	theme.GetArrowUpButtonDisabled(buttonIcon);
	button->SetBitmapDisabled(buttonIcon);
	button->Enable(false);
	button->SetToolTip(wxT("Move selected up."));
	buttonBox->Add(button, buttonBoxFlags);

	theme.GetArrowDownButton(buttonIcon);
	m_downButton = button = new wxBitmapButton(
		this,
		CONTROL_MOVE_DOWN,
		buttonIcon,
		wxDefaultPosition,
		buttonSize);
	theme.GetArrowDownButtonDisabled(buttonIcon);
	button->SetBitmapDisabled(buttonIcon);
	button->Enable(false);
	button->SetToolTip(wxT("Move selected down."));
	buttonBox->Add(button, buttonBoxFlags);

	buttonBox->AddSpacer(theme.GetDlgBorder() / 2);

	theme.GetEditItemButton(buttonIcon);
	m_editButton = button = new wxBitmapButton(
		this,
		CONTROL_EDIT,
		buttonIcon,
		wxDefaultPosition,
		buttonSize);
	theme.GetEditItemButtonDisabled(buttonIcon);
	button->SetBitmapDisabled(buttonIcon);
	button->Enable(false);
	button->SetToolTip(wxT("Edit selected."));
	buttonBox->Add(button, buttonBoxFlags);

	buttonBox->AddSpacer(theme.GetDlgBorder() / 2);

	theme.GetAddItemButton(buttonIcon);
	m_addButton = button = new wxBitmapButton(
		this,
		CONTROL_ADD,
		buttonIcon,
		wxDefaultPosition,
		buttonSize);
	theme.GetAddItemButtonDisabled(buttonIcon);
	button->SetBitmapDisabled(buttonIcon);
	button->SetToolTip(wxT("Add new."));
	buttonBox->Add(button, buttonBoxFlags);

	theme.GetRemoveItemButton(buttonIcon);
	m_removeButton = button = new wxBitmapButton(
		this,
		CONTROL_REMOVE,
		buttonIcon,
		wxDefaultPosition,
		buttonSize);
	theme.GetRemoveItemButtonDisabled(buttonIcon);
	button->SetBitmapDisabled(buttonIcon);
	button->Enable(false);
	button->SetToolTip(wxT("Remove selected."));
	buttonBox->Add(button, buttonBoxFlags);

	topStaticBox->AddSpacer(theme.GetDlgBorder());
	topStaticBox->Add(buttonBox.get());
	buttonBox.release();

	wxStaticBoxSizer &staticBox = *new wxStaticBoxSizer(
		wxHORIZONTAL,
		this,
		wxT("HTTP proxy server list"));
	staticBox.Add(topStaticBox.get(), theme.GetStaticBoxFlags());
	topStaticBox.release();

	std::auto_ptr<wxBoxSizer> topSizer(new wxBoxSizer(wxVERTICAL));
	topSizer->Add(&staticBox, theme.GetTopSizerFlags());
	topSizer->Add(new wxStaticLine(this), theme.GetTopSizerFlags());
	topSizer->Add(CreateButtonSizer(wxOK | wxCANCEL | wxHELP), theme.GetTopSizerFlags());
	topSizer->AddSpacer(theme.GetDlgBottomBorder());

	topSizer->SetMinSize(wxSize(300, -1));
	SetSizer(topSizer.get());
	topSizer.release()->SetSizeHints(this);
	Center();

	UpdateList();

}

ProxyCascadeDlg::~ProxyCascadeDlg() {
	//...//
}

void ProxyCascadeDlg::OnOk(wxCommandEvent &) {
	if (!Validate()) {
		return;
	}
	EndModal(wxID_OK);
}

void ProxyCascadeDlg::OnHelp(wxCommandEvent &) {
	wxGetApp().DisplayHelp(wxT("dialogs/proxy-cascade"));
}

void ProxyCascadeDlg::UpdateList() {
	wxArrayString list;
	foreach (const ProxyDlg::Info &proxy, m_cascade) {
		list.push_back(ConvertProxyToString(proxy));
	}
	m_listCtrl->Set(list);
	EnableButtons();
}

void ProxyCascadeDlg::OnSelectionChange(wxCommandEvent &) {
	EnableButtons();
}

void ProxyCascadeDlg::EnableButtons() {
	wxArrayInt selections;
	m_listCtrl->GetSelections(selections);
	const bool isSelected = selections.size() > 0;
	m_addButton->Enable(m_isLicensed);
	m_upButton->Enable(
		m_isLicensed && isSelected && m_listCtrl->GetCount() > selections.size());
	m_downButton->Enable(
		m_isLicensed && isSelected && m_listCtrl->GetCount() > selections.size());
	m_editButton->Enable(
		m_isLicensed && isSelected && selections.size() == 1);
	m_removeButton->Enable(isSelected);
}

void ProxyCascadeDlg::OnMoveUp(wxCommandEvent &) {
	Move(true);
}

void ProxyCascadeDlg::OnMoveDown(wxCommandEvent &) {
	Move(false);
}

void ProxyCascadeDlg::OnEdit(wxCommandEvent &) {
	if (!m_isLicensed) {
		return;
	}
	wxArrayInt selections;
	m_listCtrl->GetSelections(selections);
	if (selections.size() != 1) {
		return;
	}
	assert(size_t(selections[0]) < m_cascade.size());
	ProxyDlg dlg(this, m_cascade[selections[0]], true, false);
	if (dlg.ShowModal() == wxID_OK) {
		m_cascade[selections[0]] = dlg.GetProxy();
		UpdateList();
	}
}

void ProxyCascadeDlg::OnAdd(wxCommandEvent &) {
	ProxyDlg dlg(this, true, false);
	if (dlg.ShowModal() == wxID_OK) {
		m_cascade.push_back(dlg.GetProxy());
		UpdateList();
	}
}

void ProxyCascadeDlg::OnRemove(wxCommandEvent &) {
	wxArrayInt selections;
	m_listCtrl->GetSelections(selections);
	assert(selections.size() > 0);
	if (selections.size() < 1) {
		return;
	}
	Cascade cascade;
	for (Cascade::const_iterator i = m_cascade.begin(); i != m_cascade.end(); ++i) {
		const wxArrayInt::const_iterator pos = std::find(
			selections.begin(),
			selections.end(),
			std::distance<Cascade::const_iterator>(m_cascade.begin(), i));
		if (pos == selections.end()) {
			cascade.push_back(*i);
		}
	}
	cascade.swap(m_cascade);
	UpdateList();
}

wxString ProxyCascadeDlg::ConvertProxyToString(const ProxyDlg::Info &info) {
	return info.host + wxT(":") + info.port;
}

void ProxyCascadeDlg::Move(bool up) {

	wxArrayInt selections;
	m_listCtrl->GetSelections(selections);
	assert(selections.size() > 0);

	typedef std::map<unsigned int, unsigned int> ChangesMap;
	ChangesMap selectedIndexes;
	for (size_t i = 0; i < selections.GetCount(); ++i) {
		if (	(up && selections.Item(i) <= 0)
				|| (!up && unsigned int(selections.Item(i)) >= m_listCtrl->GetCount() - 1)) {
			return;
		}
		selectedIndexes.insert(
			std::make_pair(
				up ? selections.Item(i) - 1 : selections.Item(i) + 1,
				selections.Item(i)));
	}

	Cascade newCascade(m_cascade);
	const long long end = up ?  m_listCtrl->GetCount() : -1;
	for (unsigned int i = up ? 0 :  m_listCtrl->GetCount() - 1; i != end; ) {
		ChangesMap::const_iterator pos = selectedIndexes.find(i);
		if (pos != selectedIndexes.end()) {
			unsigned int newIndex = up ? i + 1 : i - 1;
			for ( ; selectedIndexes.find(newIndex) != selectedIndexes.end(); ) {
				up ? ++newIndex : --newIndex;
			}
			newCascade[newIndex] = m_cascade[i];
			m_listCtrl->SetString(
				newIndex,
				ConvertProxyToString(newCascade[newIndex]));
			m_listCtrl->SetSelection(newIndex, false);
			do {
				newCascade[i] = m_cascade[pos->second];
				m_listCtrl->SetString(
					i,
					ConvertProxyToString(newCascade[i]));
				m_listCtrl->SetSelection(i, true);
				pos = selectedIndexes.find(up ? ++i : --i);
			} while (pos != selectedIndexes.end());
		} else {
			up ? ++i : --i;
		}
	}

	newCascade.swap(m_cascade);

}
