/**************************************************************************
 *   Created: 2010/10/16 16:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: SslCertificateDlg.hpp 1092 2010-12-13 17:59:17Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__CertificateDlg_hpp__1010161642
#define INCLUDED_FILE__TUNNELEX__CertificateDlg_hpp__1010161642

class SslCertificateDlg : public wxDialog {

private:

	enum Control;

public:
	
	explicit SslCertificateDlg(
			wxWindow *parent,
			wxWindowID = wxID_ANY);
	explicit SslCertificateDlg(
			const texs__SslCertificateInfo &certificate,
			wxWindow *parent,
			wxWindowID = wxID_ANY);
	~SslCertificateDlg();

	DECLARE_NO_COPY_CLASS(SslCertificateDlg);
	DECLARE_EVENT_TABLE();

public:

	const texs__SslCertificateInfo & GetCertificate() const {
		return m_certificate;
	}
	
	const std::string & GetSignKey() const {
		return m_signKey;
	}

public:

	void OnOk(wxCommandEvent &);
	void OnHelp(wxCommandEvent &);
	void OnCountryText(wxCommandEvent &);
	void OnBrowseSignKey(wxCommandEvent &);
	void OnSaveToFile(wxCommandEvent &);

private:

	void CreateControls();

private:

	texs__SslCertificateInfo m_certificate;
	std::string m_signKey;

	bool m_isGenerateMode;

	wxChoice *m_keySize;
	wxTextCtrl *m_country;
	wxString m_validCountry;
	wxTextCtrl *m_stateOrProvince;
	wxTextCtrl *m_city;
	wxTextCtrl *m_organization;
	wxTextCtrl *m_organizationUnit;
	wxTextCtrl *m_subjectCommonName;
	wxTextCtrl *m_issuerCommonName;
	wxTextCtrl *m_signKeyPath;

};

#endif // INCLUDED_FILE__TUNNELEX__CertificateDlg_hpp__1010161642
