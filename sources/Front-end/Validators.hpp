/**************************************************************************
 *   Created: 2008/12/15 15:36
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Validators.hpp 1059 2010-11-12 16:17:09Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Validators_hpp__0812151536
#define INCLUDED_FILE__TUNNELEX__Validators_hpp__0812151536

class wxCheckBox;
class wxTextCtrl;

//////////////////////////////////////////////////////////////////////////

class HostValidator : public wxValidator {

public:

	explicit HostValidator(bool validateIfVisibleOnly);

	virtual bool Validate(wxWindow *);
	virtual wxObject * Clone() const;
	virtual bool TransferToWindow();

private:

	const bool m_validateIfVisibleOnly;

};

//////////////////////////////////////////////////////////////////////////

class NetworPortValidator : public wxValidator {

public:

	explicit NetworPortValidator(bool validateIfVisibleOnly);

	virtual bool Validate(wxWindow *);
	virtual wxObject * Clone() const;
	virtual bool TransferToWindow();

private:

	const bool m_validateIfVisibleOnly;

};

//////////////////////////////////////////////////////////////////////////

class PipeValidator : public wxValidator {

public:

	explicit PipeValidator(bool validateIfVisibleOnly);

	virtual bool Validate(wxWindow *);
	virtual wxObject * Clone() const;
	virtual bool TransferToWindow();

private:

	const bool m_validateIfVisibleOnly;

};

//////////////////////////////////////////////////////////////////////////

class LicenseUuidValidator : public wxValidator {

public:

	explicit LicenseUuidValidator(bool validateIfVisibleOnly);

	virtual bool Validate(wxWindow *);
	virtual wxObject * Clone() const;
	virtual bool TransferToWindow();

private:

	const bool m_validateIfVisibleOnly;

};

//////////////////////////////////////////////////////////////////////////

class HttpProxyAuthUserNameValidator : public wxValidator {

public:

	explicit HttpProxyAuthUserNameValidator(bool validateIfVisibleOnly);

	virtual bool Validate(wxWindow *);
	virtual wxObject * Clone() const;
	virtual bool TransferToWindow();

public:

	static void ShowWarning(wxTextCtrl &ctrl);

private:

	const bool m_validateIfVisibleOnly;

};

//////////////////////////////////////////////////////////////////////////

class NumericValidator : public wxValidator {

public:

	explicit NumericValidator(
			const wxString &fieldName,
			bool validateIfVisibleOnly);

	virtual bool Validate(wxWindow *);
	virtual wxObject * Clone() const;
	virtual bool TransferToWindow();

private:

	const wxString m_fieldName;
	const bool m_validateIfVisibleOnly;

};

//////////////////////////////////////////////////////////////////////////

class NotEmptyValidator : public wxValidator {

public:

	explicit NotEmptyValidator(
			const wxString &fieldName,
			bool validateIfVisibleOnly);

	virtual bool Validate(wxWindow *);
	virtual wxObject * Clone() const;
	virtual bool TransferToWindow();

private:

	const wxString m_fieldName;
	const bool m_validateIfVisibleOnly;

};

//////////////////////////////////////////////////////////////////////////

class HttpProxyAuthPasswordValidator : public wxValidator {

public:

	explicit HttpProxyAuthPasswordValidator(bool validateIfVisibleOnly);

	virtual bool Validate(wxWindow *);
	virtual wxObject * Clone() const;
	virtual bool TransferToWindow();

public:

	static void ShowWarning(wxTextCtrl &ctrl);

private:

	const bool m_validateIfVisibleOnly;

};

//////////////////////////////////////////////////////////////////////////

class LicenseKeyValidator : public wxValidator {

public:

	explicit LicenseKeyValidator(bool validateIfVisibleOnly);

	virtual bool Validate(wxWindow *);
	virtual wxObject * Clone() const;
	virtual bool TransferToWindow();

private:

	const bool m_validateIfVisibleOnly;

};

//////////////////////////////////////////////////////////////////////////

class PasswordConfirmationValidator : public wxValidator {

public:

	explicit PasswordConfirmationValidator(
			const wxTextCtrl &passwordCtrl,
			bool validateIfVisibleOnly);

	virtual bool Validate(wxWindow *);
	virtual wxObject * Clone() const;
	virtual bool TransferToWindow();

private:

	const wxTextCtrl &m_passwordCtrl;
	const bool m_validateIfVisibleOnly;

};

//////////////////////////////////////////////////////////////////////////

#endif // INCLUDED_FILE__TUNNELEX__Validators_hpp__0812151536
