/**************************************************************************
 *   Created: 2009/11/29 21:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2009 Eugene V. Palchukovsky
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__LicenseDlg_hpp__0911292141
#define INCLUDED_FILE__TUNNELEX__LicenseDlg_hpp__0911292141

class ServiceWindow;

class LicenseDlg : public wxDialog {

private:

	enum Control {
		CTRL_ACTIVATE,
		CTRL_REQUEST_TRIAL,
		CTRL_ORDER
	};

public:
	
	explicit LicenseDlg(
			ServiceWindow &service,
			wxWindow *parent,
			wxWindowID = wxID_ANY);
	~LicenseDlg();

	DECLARE_NO_COPY_CLASS(LicenseDlg);

public:

	void OnActivate(wxCommandEvent &);
	void OnRequestTrial(wxCommandEvent &);
	void OnOrderPage(wxCommandEvent &);

private:

	void Reinit();

private:

	ServiceWindow &m_service;
	wxStaticText *m_licensedTo;
	wxButton *m_trialButton;

	DECLARE_EVENT_TABLE();

};

#endif // INCLUDED_FILE__TUNNELEX__LicenseDlg_hpp__0911292141
