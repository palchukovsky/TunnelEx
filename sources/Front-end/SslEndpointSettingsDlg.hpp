/**************************************************************************
 *   Created: 2010/11/27 11:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: SslEndpointSettingsDlg.hpp 1109 2010-12-26 06:33:37Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__SslEndpointSettingsDlg_hpp__1011271101
#define INCLUDED_FILE__TUNNELEX__SslEndpointSettingsDlg_hpp__1011271101

#include "Modules/Inet/InetEndpointAddress.hpp"

class ServiceWindow;

class SslEndpointSettingsDlg : public wxDialog {

private:

	enum Control;

public:

	SslEndpointSettingsDlg(
			bool isServer,
			const TunnelEx::SslCertificateId &,
			const TunnelEx::SslCertificateIdCollection &,
			wxWindow *parent,
			ServiceWindow &service,
			const bool readOnly);
	~SslEndpointSettingsDlg() throw();

	DECLARE_EVENT_TABLE();
	DECLARE_NO_COPY_CLASS(SslEndpointSettingsDlg);

public:

	void OnOk(wxCommandEvent &);
	void OnHelp(wxCommandEvent &);
	void OnSelectCertificate(wxCommandEvent &);
	void OnSelectRemoteCertificates(wxCommandEvent &);
	void OnUseAnonymousCertificateToggle(wxCommandEvent &);
	void OnVerifyRemoteCertificatesToggle(wxCommandEvent &);

	const TunnelEx::SslCertificateId & GetCertificate() const {
		return m_certificate;
	}

	const TunnelEx::SslCertificateIdCollection & GetRemoteCertificates() const {
		return m_remoteCertificates;
	}

protected:

	void UpdateState();

	void SelectCertificate();
	void SelectRemoteCertificates();

	static wxString GetCertificateStringInfo(const texs__SslCertificateInfo &);

private:

	void CreateControls(bool isServer);
	
private:

	ServiceWindow &m_service;

	wxCheckBox *m_useAnonymousCertificateToggle;
	wxCheckBox *m_verifyRemoteCertificatesToggle;

	wxStaticText *m_certificateLabel;
	wxTextCtrl *m_certificateInfo;
	TunnelEx::SslCertificateId m_certificate;
	wxButton *m_certificateSelect;
	
	wxStaticText *m_remoteCertificatesLabel;
	wxTextCtrl *m_remoteCertificatesInfo;
	TunnelEx::SslCertificateIdCollection m_remoteCertificates;
	wxButton *m_remoteCertificatesSelect;

	const bool m_readOnly;

};

#endif // INCLUDED_FILE__TUNNELEX__SslEndpointSettingsDlg_hpp__1011271101
