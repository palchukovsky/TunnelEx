/**************************************************************************
 *   Created: 2010/10/25 16:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__LicenseStartDlg_hpp__1010251635
#define INCLUDED_FILE__TUNNELEX__LicenseStartDlg_hpp__1010251635

class ServiceWindow;

class LicenseStartDlg : public wxDialog {

private:

	enum Control {
		CTRL_REQUEST_TRIAL,
		CTRL_ACTIVATE,
		CTRL_ORDER
	};

public:
	
	explicit LicenseStartDlg(
				ServiceWindow &service,
				wxWindow *parent,
				wxWindowID id = wxID_ANY);
	~LicenseStartDlg();

	DECLARE_NO_COPY_CLASS(LicenseStartDlg);
	DECLARE_EVENT_TABLE();

public:

	void OnRequestTrial(wxCommandEvent &);
	void OnActivate(wxCommandEvent &);
	void OnOrder(wxCommandEvent &);

private:

	void CreateControls();

private:

	ServiceWindow &m_service;

};

#endif // INCLUDED_FILE__TUNNELEX__LicenseStartDlg_hpp__1010251635
