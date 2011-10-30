/**************************************************************************
 *   Created: 2008/12/15 15:38
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "Validators.hpp"

using namespace TunnelEx;

//////////////////////////////////////////////////////////////////////////

HostValidator::HostValidator(bool validateIfVisibleOnly)
		: m_validateIfVisibleOnly(validateIfVisibleOnly) {
	//...//
}

bool HostValidator::IsValidationRequired() const {
	const auto &ctrl = *boost::polymorphic_downcast<wxTextCtrl *>(GetWindow());
	return !(m_validateIfVisibleOnly && (!ctrl.IsShown() || !ctrl.GetParent()->IsShown()));
}

bool HostValidator::Validate(wxWindow *) {

	if (!IsValidationRequired()) {
		return true;
	}

	wxTextCtrl &ctrl = *boost::polymorphic_downcast<wxTextCtrl *>(GetWindow());
	const wxString val = ctrl.GetValue().Trim();

	const boost::wregex expression(
		L"([a-z0-9]([\\-a-z0-9]*[a-z0-9])?\\.?)*[a-z0-9]+",
		boost::regex_constants::perl | boost::regex_constants::icase);
	if (!boost::regex_match(val.c_str(), expression)) {
		ctrl.SetFocus();
		ctrl.SelectAll();
		wxLogWarning(
			wxT("Please, provide a valid host name or IP address.")
				wxT("\nEx.: ") TUNNELEX_DOMAIN_W wxT(", example.com, 192.168.2.24."));
		return false;
	}
		
	ctrl.SetValue(val);
	return true;

}

wxObject * HostValidator::Clone() const {
	return new HostValidator(m_validateIfVisibleOnly);
}

bool HostValidator::TransferToWindow() {
	return true;
}

////////////////////////////////////////////////////////////////////////////////

DestinationHostValidator::DestinationHostValidator(
			bool validateIfVisibleOnly,
			wxWindowID pathfinderToggleCtrlId,
			wxWindowID proxyToggleCtrlId)
		: Base(validateIfVisibleOnly),
		m_pathfinderToggleCtrlId(pathfinderToggleCtrlId),
		m_proxyToggleCtrlId(proxyToggleCtrlId) {
	//...//
}

bool DestinationHostValidator::Validate(wxWindow *window) {
	
	if (!IsValidationRequired()) {
		return true;
	}
	
	auto &ctrl = *boost::polymorphic_downcast<wxTextCtrl *>(GetWindow());
	if (IsPathfinderInUse() && !IsProxyInUse() && IsLocalHost(ctrl.GetValue().Trim())) {
		ctrl.SetFocus();
		ctrl.SelectAll();
		wxLogWarning(
			wxT("It is impossible to assign a loopback address")
				wxT(" (such as localhost, 127.0.0.1, 0:0:0:0:0:0:0:1 or ::1)")
				wxT(" as the tunnel destination if uses Pathfinder online service,")
				wxT(" because this address is not reachable for a external proxy server.")
				wxT(" Please provide an address of a network interface,")
				wxT(" which are external proxy server will be able to connect."));
		return false;
	}
	
	return Base::Validate(window);

}

bool DestinationHostValidator::IsPathfinderInUse() const {
	return boost::polymorphic_downcast<wxCheckBox *>(
		GetWindow()
		->GetParent()
		->FindWindow(m_pathfinderToggleCtrlId))
		->GetValue();
}

bool DestinationHostValidator::IsProxyInUse() const {
	return boost::polymorphic_downcast<wxCheckBox *>(
		GetWindow()
		->GetParent()
		->FindWindow(m_proxyToggleCtrlId))
		->GetValue();
}

bool DestinationHostValidator::IsLocalHost(const wxString &host) const {
	return
		host.CmpNoCase(wxT("localhost")) == 0
		|| host.CmpNoCase(wxT("127.0.0.1")) == 0
		|| host.CmpNoCase(wxT("0:0:0:0:0:0:0:1")) == 0
		|| host.CmpNoCase(wxT("::1")) == 0;
}

wxObject * DestinationHostValidator::Clone() const {
	return new DestinationHostValidator(
		ValidateIfVisibleOnly(),
		m_pathfinderToggleCtrlId,
		m_proxyToggleCtrlId);
}

//////////////////////////////////////////////////////////////////////////

NetworPortValidator::NetworPortValidator(bool validateIfVisibleOnly)
		: m_validateIfVisibleOnly(validateIfVisibleOnly) {
	//...//
}

bool NetworPortValidator::Validate(wxWindow *) {
	wxTextCtrl &ctrl = *boost::polymorphic_downcast<wxTextCtrl *>(GetWindow());
	if (m_validateIfVisibleOnly && (!ctrl.IsShown() || !ctrl.GetParent()->IsShown())) {
		return true;
	}
	std::wstring val = ctrl.GetValue().c_str();
	boost::trim(val);
	if (!(!val.empty() && boost::regex_match(val, boost::wregex(L"\\d+")))) {
		ctrl.SetFocus();
		ctrl.SelectAll();
		wxLogWarning(wxT("Please, provide a valid network port."));
		return false;
	} else {
		ctrl.SetValue(val.c_str());
		return true;
	}
}

wxObject * NetworPortValidator::Clone() const {
	return new NetworPortValidator(m_validateIfVisibleOnly);
}

bool NetworPortValidator::TransferToWindow() {
	return true;
}

//////////////////////////////////////////////////////////////////////////

PipeValidator::PipeValidator(bool validateIfVisibleOnly)
		: m_validateIfVisibleOnly(validateIfVisibleOnly) {
	//...//
}

bool PipeValidator::Validate(wxWindow *) {
	wxTextCtrl &ctrl = *boost::polymorphic_downcast<wxTextCtrl *>(GetWindow());
	if (m_validateIfVisibleOnly && (!ctrl.IsShown() || !ctrl.GetParent()->IsShown())) {
		return true;
	}
	wxString val = ctrl.GetValue().Trim();
	//! @todo: make one expression
	const bool result
		= boost::regex_match(val.c_str(), boost::wregex(L"([ \\d\\W_A-Za-z]*/+)*[ \\d\\W_A-Za-z]+"))
			&& val.Find(L"\\") == -1
			&& !boost::regex_match(val.c_str(), boost::wregex(L"[/\\\\]+"));
	if (!result) {
		ctrl.SetFocus();
		ctrl.SelectAll();
		wxLogWarning(wxT("Please, provide a valid pipe name.\nEx.: MyPipeName, MyPipeName/Test, MyPipeName_{E8971C26-CA8E-11DD-BD0D-DD6855D89593}."));
	} else {
		ctrl.SetValue(val);
	}
	return result;
}

wxObject * PipeValidator::Clone() const {
	return new PipeValidator(m_validateIfVisibleOnly);
}

bool PipeValidator::TransferToWindow() {
	return true;
}

//////////////////////////////////////////////////////////////////////////

LicenseUuidValidator::LicenseUuidValidator(bool validateIfVisibleOnly)
		: m_validateIfVisibleOnly(validateIfVisibleOnly) {
	//...//
}

bool LicenseUuidValidator::Validate(wxWindow *) {
	wxTextCtrl &ctrl = *boost::polymorphic_downcast<wxTextCtrl *>(GetWindow());
	if (m_validateIfVisibleOnly && (!ctrl.IsShown() || !ctrl.GetParent()->IsShown())) {
		return true;
	}
	wxString val = ctrl.GetValue().Trim();
	const boost::wregex expr(
		L"^[\\dA-F]{8,8}-[\\dA-F]{4,4}-[\\dA-F]{4,4}-[\\dA-F]{4,4}-[\\dA-F]{12,12}$",
		boost::regex_constants::perl | boost::regex_constants::icase);
	const bool result = boost::regex_match(val.c_str(), expr);
	if (!result) {
		ctrl.SetFocus();
		ctrl.SelectAll();
		wxLogWarning(wxT("Please, provide a valid license number in format XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX."));
	} else {
		ctrl.SetValue(val);
	}
	return result;
}

wxObject * LicenseUuidValidator::Clone() const {
	return new LicenseUuidValidator(m_validateIfVisibleOnly);
}

bool LicenseUuidValidator::TransferToWindow() {
	return true;
}

//////////////////////////////////////////////////////////////////////////

HttpProxyAuthUserNameValidator::HttpProxyAuthUserNameValidator(bool validateIfVisibleOnly)
		: m_validateIfVisibleOnly(validateIfVisibleOnly) {
	//...//
}

bool HttpProxyAuthUserNameValidator::Validate(wxWindow *) {
	wxTextCtrl &ctrl = *boost::polymorphic_downcast<wxTextCtrl *>(GetWindow());
	if (m_validateIfVisibleOnly && (!ctrl.IsShown() || !ctrl.GetParent()->IsShown())) {
		return true;
	}
	const bool result = true;
	if (!result) {
		ShowWarning(ctrl);		
	}
	return result;
}

wxObject * HttpProxyAuthUserNameValidator::Clone() const {
	return new HttpProxyAuthUserNameValidator(m_validateIfVisibleOnly);
}

bool HttpProxyAuthUserNameValidator::TransferToWindow() {
	return true;
}

void HttpProxyAuthUserNameValidator::ShowWarning(wxTextCtrl &ctrl) {
	ctrl.SetFocus();
	ctrl.SelectAll();
	wxLogWarning(wxT("Please, provide a valid user name for proxy server authorization."));
}

//////////////////////////////////////////////////////////////////////////

HttpProxyAuthPasswordValidator::HttpProxyAuthPasswordValidator(bool validateIfVisibleOnly)
		: m_validateIfVisibleOnly(validateIfVisibleOnly) {
	//...//
}

bool HttpProxyAuthPasswordValidator::Validate(wxWindow *) {
	wxTextCtrl &ctrl = *boost::polymorphic_downcast<wxTextCtrl *>(GetWindow());
	if (m_validateIfVisibleOnly && (!ctrl.IsShown() || !ctrl.GetParent()->IsShown())) {
		return true;
	}
	const bool result = true;
	if (!result) {
		ShowWarning(ctrl);
	}
	return result;
}

wxObject * HttpProxyAuthPasswordValidator::Clone() const {
	return new HttpProxyAuthPasswordValidator(m_validateIfVisibleOnly);
}

bool HttpProxyAuthPasswordValidator::TransferToWindow() {
	return true;
}

void HttpProxyAuthPasswordValidator::ShowWarning(wxTextCtrl &ctrl) {
	ctrl.SetFocus();
	ctrl.SelectAll();
	wxLogWarning(wxT("Please, provide a valid password for proxy server authorization."));
}

//////////////////////////////////////////////////////////////////////////

NumericValidator::NumericValidator(const wxString &fieldName, bool validateIfVisibleOnly)
		: m_fieldName(fieldName),
		m_validateIfVisibleOnly(validateIfVisibleOnly) {
	//...//
}

bool NumericValidator::Validate(wxWindow *) {
	wxTextCtrl &ctrl = *boost::polymorphic_downcast<wxTextCtrl *>(GetWindow());
	if (m_validateIfVisibleOnly && (!ctrl.IsShown() || !ctrl.GetParent()->IsShown())) {
		return true;
	}
	const bool result = boost::regex_match(ctrl.GetValue().c_str(), boost::wregex(L"^\\d+$"));
	if (!result) {
		ctrl.SetFocus();
		ctrl.SelectAll();
		WFormat message(wxT("Please, provide a valid %1% value (should be numeric)."));
		message % m_fieldName.c_str();
		wxLogWarning(message.str().c_str());
	}
	return result;
}

wxObject * NumericValidator::Clone() const {
	return new NumericValidator(m_fieldName, m_validateIfVisibleOnly);
}

bool NumericValidator::TransferToWindow() {
	return true;
}

//////////////////////////////////////////////////////////////////////////

NotEmptyValidator::NotEmptyValidator(const wxString &fieldName, bool validateIfVisibleOnly)
		: m_fieldName(fieldName),
		m_validateIfVisibleOnly(validateIfVisibleOnly) {
	//...//
}

bool NotEmptyValidator::Validate(wxWindow *) {
	wxTextCtrl &ctrl = *boost::polymorphic_downcast<wxTextCtrl *>(GetWindow());
	if (m_validateIfVisibleOnly && (!ctrl.IsShown() || !ctrl.GetParent()->IsShown())) {
		return true;
	}
	const bool result = !ctrl.GetValue().Trim().IsEmpty();
	if (!result) {
		ctrl.SetFocus();
		ctrl.SelectAll();
		WFormat message(wxT("Please, provide a valid %1% value (cannot be empty)."));
		message % m_fieldName.c_str();
		wxLogWarning(message.str().c_str());
	}
	return result;
}

wxObject * NotEmptyValidator::Clone() const {
	return new NotEmptyValidator(m_fieldName, m_validateIfVisibleOnly);
}

bool NotEmptyValidator::TransferToWindow() {
	return true;
}

//////////////////////////////////////////////////////////////////////////

LicenseKeyValidator::LicenseKeyValidator(bool validateIfVisibleOnly)
		: m_validateIfVisibleOnly(validateIfVisibleOnly) {
	//...//
}

bool LicenseKeyValidator::Validate(wxWindow *) {
	wxTextCtrl &ctrl = *boost::polymorphic_downcast<wxTextCtrl *>(GetWindow());
	if (m_validateIfVisibleOnly && (!ctrl.IsShown() || !ctrl.GetParent()->IsShown())) {
		return true;
	}
	std::string val = ctrl.GetValue().Trim().ToAscii();
	boost::trim_if(val, boost::is_any_of("\n\r "));
	bool result = !val.empty();
	if (result) {
		std::vector<std::string> lines;
		split(lines, val, boost::is_any_of("\n"));
		foreach (std::string &line, lines) {
			boost::trim_if(line, boost::is_any_of("\r "));
			if (line.empty()) {
				result = false;
				break;
			}
		}
		result = result 
			&& lines.size() >= 3
			&& lines[0] == "-----BEGIN TUNNELEX LICENSE KEY-----"
			&& *lines.rbegin() == "-----END TUNNELEX LICENSE KEY-----";
	}
	if (!result) {
		ctrl.SetFocus();
		wxLogWarning(
			wxT("Please, provide a correct activation code.")
				wxT(" It should contain several lines,")
				wxT(" begins with \"-----BEGIN TUNNELEX LICENSE KEY-----\"")
				wxT(" and ends with \"-----END TUNNELEX LICENSE KEY-----\"."));
	}
	return result;
}

wxObject * LicenseKeyValidator::Clone() const {
	return new LicenseKeyValidator(m_validateIfVisibleOnly);
}

bool LicenseKeyValidator::TransferToWindow() {
	return true;
}

//////////////////////////////////////////////////////////////////////////

PasswordConfirmationValidator::PasswordConfirmationValidator(
			const wxTextCtrl &passwordCtrl,
			bool validateIfVisibleOnly)
		: m_passwordCtrl(passwordCtrl),
		m_validateIfVisibleOnly(validateIfVisibleOnly) {
	//...//
}

bool PasswordConfirmationValidator::Validate(wxWindow *) {
	wxTextCtrl &ctrl = *boost::polymorphic_downcast<wxTextCtrl *>(GetWindow());
	if (m_validateIfVisibleOnly && (!ctrl.IsShown() || !ctrl.GetParent()->IsShown())) {
		return true;
	} else if (m_passwordCtrl.GetValue() != ctrl.GetValue()) {
		ctrl.Clear();
		ctrl.SetFocus();
		wxLogWarning(wxT("Password does not match the confirm password."));
		return false;
	} else {
		return true;
	}
}

wxObject * PasswordConfirmationValidator::Clone() const {
	return new PasswordConfirmationValidator(m_passwordCtrl, m_validateIfVisibleOnly);
}

bool PasswordConfirmationValidator::TransferToWindow() {
	return true;
}

//////////////////////////////////////////////////////////////////////////
