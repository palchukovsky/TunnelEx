/**************************************************************************
 *   Created: 2009/12/06 23:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__LicenseEnterDlg_hpp__0912062335
#define INCLUDED_FILE__TUNNELEX__LicenseEnterDlg_hpp__0912062335

class wxTextCtrl;
class ServiceAdapter;

//////////////////////////////////////////////////////////////////////////

class LicenseKeyDlg : public wxDialog {

private:

	enum Control {
		CTRL_PASTE
	};

public:

	explicit LicenseKeyDlg(
			ServiceAdapter &service,
			wxWindow *parent,
			wxWindowID id = wxID_ANY);

	DECLARE_NO_COPY_CLASS(LicenseKeyDlg);

public:

	void OnOk(wxCommandEvent &);
	void OnPaste(wxCommandEvent &);

	DECLARE_EVENT_TABLE();

private:

	ServiceAdapter &m_service;
	wxTextCtrl *m_licenseKeyCtrl;

};

//////////////////////////////////////////////////////////////////////////

class LicenseEnterDlg : public wxDialog {

private:

	enum Control {
		CTRL_LICENSE,
		CTRL_ACTIVATE_ONLINE,
		CTRL_ACTIVATE_OFFLINE
	};

public:

	explicit LicenseEnterDlg(
			ServiceAdapter &service,
			wxWindow *parent,
			wxWindowID = wxID_ANY);
	~LicenseEnterDlg();

	DECLARE_NO_COPY_CLASS(LicenseEnterDlg);

public:

	void OnOnlineActivation(wxCommandEvent &);
	void OnOfflineActivation(wxCommandEvent &);

private:

	void TrimLicense();

private:

	ServiceAdapter &m_service;

	DECLARE_EVENT_TABLE();

};

//////////////////////////////////////////////////////////////////////////

#endif // INCLUDED_FILE__TUNNELEX__LicenseEnterDlg_hpp__0912062335
