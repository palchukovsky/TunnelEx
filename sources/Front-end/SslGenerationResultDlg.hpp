/**************************************************************************
 *   Created: 2010/11/15 21:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: SslGenerationResultDlg.hpp 1116 2010-12-30 23:13:48Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__SslCertificateRequestDlg_hpp__1011152108
#define INCLUDED_FILE__TUNNELEX__SslCertificateRequestDlg_hpp__1011152108

class SslGenerationResultDlg : public wxDialog {

public:

	enum Mode {
		MODE_CERTIFICATE,
		MODE_REQUEST
	};

private:

	enum Control;

public:
	
	explicit SslGenerationResultDlg(
			const Mode mode,
			const wxString &privateKey,
			const wxString &content,
			wxWindow *parent,
			wxWindowID = wxID_ANY);
	~SslGenerationResultDlg();

	DECLARE_NO_COPY_CLASS(SslGenerationResultDlg);
	DECLARE_EVENT_TABLE();

public:

	void OnOk(wxCommandEvent &);
	void OnCancel(wxCommandEvent &);
	void OnClose(wxCloseEvent &);
	void OnHelp(wxCommandEvent &);
	void OnSavePrivateKey(wxCommandEvent &);
	void OnCopyPrivateKey(wxCommandEvent &);
	void OnSaveContent(wxCommandEvent &);
	void OnCopyContent(wxCommandEvent &);

private:

	void CreateControls(const wxString &privateKey, const wxString &request);
	void Copy(const wxTextCtrl &) const;
	bool CheckClose(bool isOk) const;

private:

	const Mode m_mode;

	wxTextCtrl *m_key;
	wxTextCtrl *m_content;

	bool m_isKeySaved;
	mutable bool m_isClosing;

};

#endif // INCLUDED_FILE__TUNNELEX__SslCertificateRequestDlg_hpp__1011152108
