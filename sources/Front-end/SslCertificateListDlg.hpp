/**************************************************************************
 *   Created: 2010/10/16 16:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: SslCertificateListDlg.hpp 1080 2010-12-01 08:33:26Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__CertificateListDlg_hpp__1010161641
#define INCLUDED_FILE__TUNNELEX__CertificateListDlg_hpp__1010161641

#include "Core/SslCertificatesStorage.hpp"

class ServiceWindow;

class SslCertificateListDlg : public wxDialog {

public:

	enum Mode {
		MODE_MANAGE,
		MODE_SELECT,
		MODE_SELECT_PRIVATE
	};

private:

	enum Control;
	
	enum Column {
		COLUMN_SUBJECT_COMMON_NAME,
		COLUMN_ISSUER_COMMON_NAME,
		COLUMN_EXPIRATION_DATE,
		COLUMN_PRIVATE,
		COLUMNS_NUMB
	};

	friend int wxCALLBACK SortCertificateListByPrivate(long, long, long);
	friend int wxCALLBACK SortCertificateListBySubjectCommonName(long, long, long);
	friend int wxCALLBACK SortCertificateListByIssuerCommonName(long, long, long);
	friend int wxCALLBACK SortCertificateListByExpirationDate(long, long, long);

public:

	explicit SslCertificateListDlg(
			ServiceWindow &service,
			wxWindow *parent,
			Mode = MODE_MANAGE,
			wxWindowID = wxID_ANY);
	~SslCertificateListDlg();

	DECLARE_NO_COPY_CLASS(SslCertificateListDlg);
	DECLARE_EVENT_TABLE();

public:

	void OnListColumnResized(wxListEvent &);
	void OnListSort(wxListEvent &);
	void OnGenerate(wxCommandEvent &);
	void OnRequest(wxCommandEvent &);
	void OnImport(wxCommandEvent &);
	void OnView(wxCommandEvent &);
	void OnListItemActivated(wxListEvent &);
	void OnDel(wxCommandEvent &);
	// see TEX-642
	/* void OnExport(wxCommandEvent &);
	void OnBackup(wxCommandEvent &);
	void OnRestore(wxCommandEvent &); */
	void OnListContextMenu(wxContextMenuEvent &);
	void OnListItemSelectionChange(wxListEvent &);
	void OnHelp(wxCommandEvent &);


	void RefreshList();
	
	void SortList(Column, bool changeDirection);
	
	void Select(const TunnelEx::SslCertificateId &id);
	void Select(const TunnelEx::SslCertificateIdCollection &ids);

	void GetSelected(TunnelEx::SslCertificateIdCollection &) const;

protected:

	void ClearSelection();
	void SelectImpl(const std::wstring &id);
	void CheckSelection();

private:

	bool CheckLicense() const;
	void ViewSelected();
	const texs__SslCertificateShortInfo & GetCertificate(const long data) const;

private:

	const Mode m_mode;

	ServiceWindow &m_service;
	const bool m_isLicenseValid;

	wxListCtrl *m_list;
	std::bitset<COLUMNS_NUMB> m_columnSortDirection;
	Column m_lastSortedBy;
	std::bitset<COLUMNS_NUMB> m_columnHasBeenResized;

	wxButton *m_delButton;
	// see TEX-642
	/* wxButton *m_exportButton;
	wxButton *m_backupButton; */

	bool m_columsSizesSet;

	std::list<texs__SslCertificateShortInfo> m_certifiactes;

};

#endif // INCLUDED_FILE__TUNNELEX__CertificateListDlg_hpp__1010161641
