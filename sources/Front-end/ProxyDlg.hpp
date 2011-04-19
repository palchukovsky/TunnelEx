/**************************************************************************
 *   Created: 2009/12/23 19:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef PROXY_DLG_H_INCLUDED_20091223
#define PROXY_DLG_H_INCLUDED_20091223

#include "LicensePolicies.hpp"

class ServiceWindow;

class ProxyDlg : public wxDialog {

private:

	enum Control;

public:

	struct Info {
		Info()
				: isAuthInUse(false) {
			//...//
		}
		wxString host;
		wxString port;
		bool isAuthInUse;
		wxString user;
		wxString password;
	};

public:

	explicit ProxyDlg(wxWindow *parent, bool inCascade, bool readOnly);
	explicit ProxyDlg(wxWindow *parent, const Info &, bool inCascade, bool readOnly);
	~ProxyDlg();

public:

	wxString GetHost() const;
	wxString GetPort() const;
	bool IsUsingAuth() const;
	wxString GetUser() const;
	wxString GetPassword() const;

	Info GetProxy() const;

public:

	void OnOk(wxCommandEvent &);
	void OnHelp(wxCommandEvent &);
	void OnAuthUsingToogle(wxCommandEvent &);

	void OnCreateCascade(wxCommandEvent &);

	bool IsInCascade() const {
		return m_isInCascade;
	}

private:

	void CreateControls(const Info &);

private:

	static const wxChar * GetInitTitle();

private:

	bool m_readOnly;
	bool m_isInCascade;

	wxTextCtrl *m_host;
	wxTextCtrl *m_port;
	
	wxCheckBox *m_isAuth;
	wxStaticText *m_authUserLabel;
	wxTextCtrl *m_authUser;
	wxStaticText *m_authPassLabel;
	wxTextCtrl *m_authPass;

	DECLARE_EVENT_TABLE();


};

//////////////////////////////////////////////////////////////////////////

class ProxyCascadeDlg : public wxDialog {

private:

	enum Control;

public:

	typedef std::vector<ProxyDlg::Info> Cascade;

public:

	explicit ProxyCascadeDlg(
		ServiceWindow &,
		wxWindow *parent,
		const Cascade &,
		const TunnelEx::Licensing::ProxyCascadeLicense &);
	~ProxyCascadeDlg();

public:

	const Cascade & GetCascade() const {
		return m_cascade;
	}

public:

	void OnOk(wxCommandEvent &);
	void OnHelp(wxCommandEvent &);

	void OnSelectionChange(wxCommandEvent &);

	void OnMoveUp(wxCommandEvent &);
	void OnMoveDown(wxCommandEvent &);
	void OnEdit(wxCommandEvent &);
	void OnAdd(wxCommandEvent &);
	void OnRemove(wxCommandEvent &);

protected:

	void UpdateList();
	void Move(bool up);

	static wxString ConvertProxyToString(const ProxyDlg::Info &);

	void EnableButtons();

private:

	Cascade m_cascade;
	wxListBox *m_listCtrl;

	wxBitmapButton *m_addButton;
	wxBitmapButton *m_upButton;
	wxBitmapButton *m_downButton;
	wxBitmapButton *m_editButton;
	wxBitmapButton *m_removeButton;

	bool m_isLicensed;

	DECLARE_EVENT_TABLE();

};

#endif // #ifndef PROXY_DLG_H_INCLUDED_20091223
