/**************************************************************************
 *   Created: 2010/10/16 16:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: SslCertificateListDlg.cpp 1097 2010-12-14 18:04:02Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "SslCertificateListDlg.hpp"
#include "SslCertificateDlg.hpp"
#include "SslGenerationResultDlg.hpp"
#include "ServiceWindow.hpp"
#include "LicenseRestrictionDlg.hpp"
#include "LicensePolicies.hpp"
#include "Application.hpp"
#include "Auto.hpp"

#include "Core/String.hpp"

using namespace TunnelEx;

//////////////////////////////////////////////////////////////////////////

int wxCALLBACK SortCertificateListBySubjectCommonName(long item1, long item2, long sortData) {
	if (item1 == item2) {
		return 0;
	}
	const SslCertificateListDlg &dlg
		= *reinterpret_cast<SslCertificateListDlg *>(sortData);
	std::string a = dlg.GetCertificate(item1).subjectCommonName;
	std::string b = dlg.GetCertificate(item2).subjectCommonName;
	if (!dlg.m_columnSortDirection[SslCertificateListDlg::COLUMN_SUBJECT_COMMON_NAME]) {
		a.swap(b);
	}
	return a.compare(b);
}

int wxCALLBACK SortCertificateListByIssuerCommonName(long item1, long item2, long sortData) {
	if (item1 == item2) {
		return 0;
	}
	const SslCertificateListDlg &dlg
		= *reinterpret_cast<SslCertificateListDlg *>(sortData);
	std::string a = dlg.GetCertificate(item1).issuerCommonName;
	std::string b = dlg.GetCertificate(item2).issuerCommonName;
	if (!dlg.m_columnSortDirection[SslCertificateListDlg::COLUMN_ISSUER_COMMON_NAME]) {
		a.swap(b);
	}
	return a.compare(b);
}

int wxCALLBACK SortCertificateListByExpirationDate(long item1, long item2, long sortData) {
	if (item1 == item2) {
		return 0;
	}
	const SslCertificateListDlg &dlg
		= *reinterpret_cast<SslCertificateListDlg *>(sortData);
	time_t a = dlg.GetCertificate(item1).validBeforeTimeUtc;
	time_t b = dlg.GetCertificate(item2).validBeforeTimeUtc;
	if (!dlg.m_columnSortDirection[SslCertificateListDlg::COLUMN_EXPIRATION_DATE]) {
		std::swap(a, b);
	}
	return a < b ? -1 : a > b ? 1 : 0;
}

int wxCALLBACK SortCertificateListByPrivate(long item1, long item2, long sortData) {
	if (item1 == item2) {
		return 0;
	}
	const SslCertificateListDlg &dlg
		= *reinterpret_cast<SslCertificateListDlg *>(sortData);
	bool a = dlg.GetCertificate(item1).isPrivate;
	bool b = dlg.GetCertificate(item2).isPrivate;
	if (!dlg.m_columnSortDirection[SslCertificateListDlg::COLUMN_PRIVATE]) {
		std::swap(a, b);
	}
	return a < b ? -1 : a > b ? 1 : 0;
}

//////////////////////////////////////////////////////////////////////////

enum SslCertificateListDlg::Control {
	CONTROL_LIST,
	CONTROL_GENERATE,
	CONTROL_REQUEST,
	CONTROL_IMPORT,
	CONTROL_VIEW,
	CONTROL_DEL,
	// see TEX-642
	/* CONTROL_EXPORT,
	CONTROL_BACKUP,
	CONTROL_RESTORE */
};

BEGIN_EVENT_TABLE(SslCertificateListDlg, wxDialog)
	EVT_LIST_COL_CLICK(SslCertificateListDlg::CONTROL_LIST, SslCertificateListDlg::OnListSort)
	EVT_LIST_COL_END_DRAG(SslCertificateListDlg::CONTROL_LIST, SslCertificateListDlg::OnListColumnResized)
	EVT_LIST_ITEM_ACTIVATED(SslCertificateListDlg::CONTROL_LIST, SslCertificateListDlg::OnListItemActivated)
	EVT_COMMAND_CONTEXT_MENU(SslCertificateListDlg::CONTROL_LIST, SslCertificateListDlg::OnListContextMenu)
	EVT_LIST_ITEM_SELECTED(SslCertificateListDlg::CONTROL_LIST, SslCertificateListDlg::OnListItemSelectionChange)
	EVT_LIST_ITEM_DESELECTED(SslCertificateListDlg::CONTROL_LIST, SslCertificateListDlg::OnListItemSelectionChange)
	EVT_BUTTON(SslCertificateListDlg::CONTROL_GENERATE, SslCertificateListDlg::OnGenerate)
	EVT_BUTTON(SslCertificateListDlg::CONTROL_REQUEST, SslCertificateListDlg::OnRequest)
	EVT_BUTTON(SslCertificateListDlg::CONTROL_IMPORT, SslCertificateListDlg::OnImport)
	EVT_BUTTON(SslCertificateListDlg::CONTROL_VIEW, SslCertificateListDlg::OnView)
	EVT_MENU(SslCertificateListDlg::CONTROL_VIEW, SslCertificateListDlg::OnView)
	EVT_BUTTON(SslCertificateListDlg::CONTROL_DEL, SslCertificateListDlg::OnDel)
	EVT_MENU(SslCertificateListDlg::CONTROL_DEL, SslCertificateListDlg::OnDel)
	// see TEX-642
	/* EVT_BUTTON(SslCertificateListDlg::CONTROL_EXPORT, SslCertificateListDlg::OnExport)
	EVT_MENU(SslCertificateListDlg::CONTROL_EXPORT, SslCertificateListDlg::OnExport)
	EVT_BUTTON(SslCertificateListDlg::CONTROL_BACKUP, SslCertificateListDlg::OnBackup)
	EVT_BUTTON(SslCertificateListDlg::CONTROL_RESTORE, SslCertificateListDlg::OnRestore) */
	EVT_BUTTON(wxID_HELP, SslCertificateListDlg::OnHelp)
END_EVENT_TABLE()

SslCertificateListDlg::SslCertificateListDlg(
			ServiceWindow &service,
			wxWindow *parent,
			Mode mode /*= MODE_MANAGE*/,
			wxWindowID id /*= wxID_ANY*/)
		: wxDialog(
			parent,
			id,
			mode == MODE_MANAGE
				?	wxT("SSL Certificate Manager")
				:	mode == MODE_SELECT_PRIVATE
					?	wxT("Select SSL Certificate")
					:	wxT("Select SSL Certificates"),
			wxDefaultPosition,
			wxDefaultSize,
			wxDEFAULT_DIALOG_STYLE),
		m_mode(mode),
		m_service(service),
		m_isLicenseValid(CheckLicense()),
		m_lastSortedBy(COLUMNS_NUMB),
		m_columsSizesSet(false) {

	const Theme &theme = wxGetApp().GetTheme();

	std::auto_ptr<wxBoxSizer> topBox(new wxBoxSizer(wxVERTICAL));

	long listStyle = wxLC_REPORT | wxLC_HRULES | wxLC_VRULES;
	if (m_mode == MODE_SELECT_PRIVATE) {
		listStyle |= wxLC_SINGLE_SEL;
	}

	m_list = new wxListCtrl(
		this,
		CONTROL_LIST,
		wxDefaultPosition,
		wxDefaultSize,
		listStyle);
	m_list->SetMinSize(wxSize(550, 300));
	topBox->Add(m_list, theme.GetTopSizerFlags());
	{

		std::auto_ptr<wxImageList> imageList(new wxImageList(16, 16, true));
		wxBitmap icon;
		theme.GetSortAscIcon(icon);
		imageList->Add(icon);
		theme.GetSortDescIcon(icon);
		imageList->Add(icon);
		theme.GetKeyIcon(icon);
		imageList->Add(icon);
		m_list->AssignImageList(imageList.get(), wxIMAGE_LIST_SMALL);
		imageList.release();

		Config &config = wxGetApp().GetConfig();
		bool flag = false;
		config.Read(wxT("/CertificateList/Columns/Sort/Direction/SubjectCommonName"), &flag, false);
		m_columnSortDirection.set(COLUMN_SUBJECT_COMMON_NAME, flag);
		config.Read(wxT("/CertificateList/Columns/Sort/Direction/IssuerCommonName"), &flag, false);
		m_columnSortDirection.set(COLUMN_ISSUER_COMMON_NAME, flag);
		config.Read(wxT("/CertificateList/Columns/Sort/Direction/ExpirationData"), &flag, false);
		m_columnSortDirection.set(COLUMN_EXPIRATION_DATE, flag);
		config.Read(wxT("/CertificateList/Columns/Sort/Direction/Private"), &flag, false);
		m_columnSortDirection.set(COLUMN_PRIVATE, flag);

		const long tmpSortBy = config.Read(
			wxT("/CertificateList/Columns/Sort/SortedBy"), m_lastSortedBy);
		if (tmpSortBy <= COLUMNS_NUMB) {
			m_lastSortedBy = static_cast<Column>(tmpSortBy);
		}

		wxListItem itemCol;
		itemCol.SetAlign(wxLIST_FORMAT_LEFT);

		itemCol.SetText(wxT("Issued to"));
		m_list->InsertColumn(COLUMN_SUBJECT_COMMON_NAME, itemCol);

		itemCol.SetText(wxT("Issued by"));
		m_list->InsertColumn(COLUMN_ISSUER_COMMON_NAME, itemCol);

		itemCol.SetText(wxT("Expiration date"));
		m_list->InsertColumn(COLUMN_EXPIRATION_DATE, itemCol);

		itemCol.SetText(wxT("Private key"));
		m_list->InsertColumn(COLUMN_PRIVATE, itemCol);

	}

	std::auto_ptr<wxBoxSizer> contentButtonsBox(new wxBoxSizer(wxHORIZONTAL));
	const wxSizerFlags buttonFlags = wxSizerFlags(1);

	wxButton &generateButton = *new wxButton(this, CONTROL_GENERATE, wxT("Generate"));
	generateButton.Enable(m_isLicenseValid);
	generateButton.SetToolTip(wxT("Generate new certificate."));
	contentButtonsBox->Add(&generateButton, buttonFlags);

	contentButtonsBox->AddSpacer(theme.GetDlgBorder() / 2);

	wxButton &requestButton = *new wxButton(this, CONTROL_REQUEST, wxT("Request"));
	requestButton.Enable(m_isLicenseValid);
	requestButton.SetToolTip(wxT("Generate certificate request."));
	contentButtonsBox->Add(&requestButton, buttonFlags);

	contentButtonsBox->AddSpacer(theme.GetDlgBorder() / 2);

	wxButton &loadButton = *new wxButton(this, CONTROL_IMPORT, wxT("Import"));
	loadButton.Enable(m_isLicenseValid);
	loadButton.SetToolTip(wxT("Load certificate from file."));
	contentButtonsBox->Add(&loadButton, buttonFlags);

	contentButtonsBox->AddSpacer(theme.GetDlgBorder() / 2);

	m_delButton = new wxButton(this, CONTROL_DEL, wxT("Delete"));
	m_delButton->Enable(false);
	m_delButton->SetToolTip(wxT("Remove selected certificates."));
	contentButtonsBox->Add(m_delButton, buttonFlags);

	// see TEX-642
	/* m_exportButton = new wxButton(this, CONTROL_EXPORT, wxT("Export"));
	m_exportButton->Enable(false);
	m_exportButton->SetToolTip(wxT("Export selected certificates."));
	contentButtonsBox->Add(m_exportButton, buttonFlags);

	m_backupButton = new wxButton(this, CONTROL_BACKUP, wxT("Backup"));
	m_backupButton->Enable(false);
	m_backupButton->SetToolTip(wxT("Backup all certificates into ZIP archive."));
	contentButtonsBox->Add(m_backupButton, buttonFlags);

	wxButton &restoreButton = *new wxButton(this, CONTROL_RESTORE, wxT("Restore"));
	restoreButton.Enable(m_isLicenseValid);
	restoreButton.SetToolTip(wxT("Restore certificates from ZIP archive."));
	contentButtonsBox->Add(&restoreButton, buttonFlags);   */

	topBox->Add(contentButtonsBox.get(), theme.GetTopSizerFlags());
	contentButtonsBox.release();

	topBox->Add(new wxStaticLine(this), theme.GetTopSizerFlags());
	long mainButtons = wxOK | wxHELP;
	if (m_mode != MODE_MANAGE) {
		mainButtons |= wxCANCEL;
	}
	topBox->Add(CreateButtonSizer(mainButtons), theme.GetTopSizerFlags());
	if (m_mode != MODE_MANAGE) {
		boost::polymorphic_downcast<wxButton *>(FindWindow(wxID_OK))->SetLabel(wxT("Select"));
	}
	topBox->AddSpacer(theme.GetDlgBottomBorder());

	topBox->SetMinSize(GetMinSize());
	SetSizer(topBox.get());
	topBox.release()->SetSizeHints(this);
	Center();

	RefreshList();

	m_list->SetFocus();

}

SslCertificateListDlg::~SslCertificateListDlg() {
	
	try {
		
		Config &config = wxGetApp().GetConfig();
		
		if (m_columnHasBeenResized[COLUMN_SUBJECT_COMMON_NAME]) {
			config.Write(
				wxT("/CertificateList/Columns/Width/SubjectCommonName"),
				m_list->GetColumnWidth(COLUMN_SUBJECT_COMMON_NAME));
		}
		if (m_columnHasBeenResized[COLUMN_ISSUER_COMMON_NAME]) {
			config.Write(
				wxT("/CertificateList/Columns/Width/IssuerCommonName"),
				m_list->GetColumnWidth(COLUMN_ISSUER_COMMON_NAME));
		}
		if (m_columnHasBeenResized[COLUMN_EXPIRATION_DATE]) {
			config.Write(
				wxT("/CertificateList/Columns/Width/ExpirationData"),
				m_list->GetColumnWidth(COLUMN_EXPIRATION_DATE));
		}

		config.Write(
			wxT("/CertificateList/Columns/Sort/Direction/SubjectCommonName"),
			m_columnSortDirection[COLUMN_SUBJECT_COMMON_NAME]);
		config.Write(
			wxT("/CertificateList/Columns/Sort/Direction/IssuerCommonName"),
			m_columnSortDirection[COLUMN_SUBJECT_COMMON_NAME]);
		config.Write(
			wxT("/CertificateList/Columns/Sort/Direction/ExpirationData"),
			m_columnSortDirection[COLUMN_EXPIRATION_DATE]);
		config.Write(
			wxT("/CertificateList/Columns/Sort/Direction/Private"),
			m_columnSortDirection[COLUMN_PRIVATE]);

		if (m_lastSortedBy != COLUMNS_NUMB) {
			config.Write(wxT("/CertificateList/Columns/Sort/SortedBy"), m_lastSortedBy);
		}

	} catch (...) {
		BOOST_ASSERT(false);
	}

}

bool SslCertificateListDlg::CheckLicense() const {

	const struct Licenses {
		explicit Licenses(ServiceAdapter &service)
				: ssl(LicenseState(service)) {
			//...//
		}
		Licensing::SslLicense ssl;
	} licenses(m_service.GetService());
	
	if (!licenses.ssl.IsFeatureAvailable(true)) {
		LicenseRestrictionDlg(
				const_cast<SslCertificateListDlg *>(this)->m_service,
				const_cast<SslCertificateListDlg *>(this),
				licenses.ssl,
				false)
			.ShowModal();
		return wxGetApp().IsUnlimitedModeActive();
	}

	return true;

}

void SslCertificateListDlg::OnListColumnResized(wxListEvent &evt) {
	m_columnHasBeenResized.set(evt.GetColumn());
}

void SslCertificateListDlg::Select(const SslCertificateId &id) {
	ClearSelection();
	SelectImpl(id.GetCStr());
}

void SslCertificateListDlg::Select(const SslCertificateIdCollection &ids) {
	BOOST_ASSERT(m_mode != MODE_SELECT_PRIVATE || ids.GetSize() <= 1);
	ClearSelection();
	for (size_t i = 0; i < ids.GetSize(); ++i) {
		SelectImpl(ids[i].GetCStr());
	}
}

void SslCertificateListDlg::ClearSelection() {
	for (long item = -1; ; ) {
		item = m_list->GetNextItem(
			item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1) {
			break;
		}
		m_list->SetItemState(
			item,
			0,
			wxLIST_STATE_SELECTED);
		return;
	}
}

void SslCertificateListDlg::SelectImpl(const std::wstring &id) {
	for (long item = -1; ; ) {
		item = m_list->GetNextItem(item, wxLIST_NEXT_ALL);
		if (item == -1) {
			break;
		}
		if (id == GetCertificate(m_list->GetItemData(item)).id) {
			m_list->SetItemState(
				item,
				wxLIST_STATE_SELECTED,
				wxLIST_STATE_SELECTED);
			return;
		}
	}
}

void SslCertificateListDlg::CheckSelection() {
	const int selected = m_list->GetSelectedItemCount();
	m_delButton->Enable(selected > 0);
	// see TEX-642
	// m_exportButton->Enable(selected > 0);
	if (m_mode != MODE_MANAGE) {
		FindWindow(wxID_OK)->Enable(selected > 0);
	}
}

void SslCertificateListDlg::RefreshList() {

	SslCertificateIdCollection selected;
	GetSelected(selected);

	m_list->DeleteAllItems();

	std::list<texs__SslCertificateShortInfo> certificates;
	m_service.GetService().GetSslCertificates(certificates);

	foreach (const texs__SslCertificateShortInfo &certificate, certificates) {

		if (m_mode == MODE_SELECT_PRIVATE && !certificate.isPrivate) {
			continue;
		}

		wxListItem itemInfo;
		itemInfo.SetImage(-1);
		itemInfo.SetAlign(wxLIST_FORMAT_LEFT);
		itemInfo.SetId(m_list->GetItemCount());
		itemInfo.SetText(wxString::FromAscii(certificate.subjectCommonName.c_str()));
		if (certificate.isPrivate) {
			itemInfo.SetImage(2);
		}
		const long index = m_list->InsertItem(itemInfo);
		BOOST_ASSERT(index >= 0);
		m_list->SetItemData(index, reinterpret_cast<long>(&certificate));
		m_list->SetItem(
			index,
			COLUMN_ISSUER_COMMON_NAME,
			wxString::FromAscii(certificate.issuerCommonName.c_str()));
		m_list->SetItem(
			index,
			COLUMN_EXPIRATION_DATE,
			wxDateTime(certificate.validBeforeTimeUtc).Format(wxT("%d.%m.%Y")));
		m_list->SetItem(
			index,
			COLUMN_PRIVATE,
			certificate.isPrivate ? wxT("yes") : wxT("no"));
	
	}
	certificates.swap(m_certifiactes);

	Select(selected);

	// see TEX-642
	// m_backupButton->Enable(m_isLicenseValid && m_list->GetItemCount() > 0);

	if (!m_columsSizesSet) {
		if (m_list->GetItemCount() > 0) {
			Config &config = wxGetApp().GetConfig();
			m_list->SetColumnWidth(
				COLUMN_SUBJECT_COMMON_NAME,
				config.Read(wxT("/CertificateList/Columns/Width/SubjectCommonName"), 200));
			m_list->SetColumnWidth(
				COLUMN_ISSUER_COMMON_NAME,
				config.Read(wxT("/CertificateList/Columns/Width/IssuerCommonName"), 200));
			m_list->SetColumnWidth(
				COLUMN_EXPIRATION_DATE,
				config.Read(wxT("/CertificateList/Columns/Width/ExpirationData"), 210));
			m_columsSizesSet = true;
		} else {
			m_list->SetColumnWidth(COLUMN_SUBJECT_COMMON_NAME, 200);
			m_list->SetColumnWidth(COLUMN_ISSUER_COMMON_NAME, 200);
			m_list->SetColumnWidth(COLUMN_EXPIRATION_DATE, 120);
		}
		m_list->SetColumnWidth(COLUMN_PRIVATE, 100);
	}

	if (m_lastSortedBy != COLUMNS_NUMB) {
		SortList(m_lastSortedBy, false);
	}

	CheckSelection();

}

void SslCertificateListDlg::OnListSort(wxListEvent &evt) {
	SortList(static_cast<Column>(evt.GetColumn()), true);
}

void SslCertificateListDlg::SortList(Column column, bool changeDirection) {

	wxListCtrlCompare fnSortCallBack;
	switch (column) {
		case COLUMN_SUBJECT_COMMON_NAME:
			fnSortCallBack = &SortCertificateListBySubjectCommonName;
			break;
		case COLUMN_ISSUER_COMMON_NAME:
			fnSortCallBack = &SortCertificateListByIssuerCommonName;
			break;
		case COLUMN_EXPIRATION_DATE:
			fnSortCallBack = &SortCertificateListByExpirationDate;
			break;
		case COLUMN_PRIVATE:
			fnSortCallBack = &SortCertificateListByPrivate;
			break;
		default:
			BOOST_ASSERT(false);
			return;
	}

	if (changeDirection && column == m_lastSortedBy) {
		m_columnSortDirection[column].flip();
	}

	m_list->SortItems(fnSortCallBack, reinterpret_cast<long>(this));
	m_lastSortedBy = column;

	wxListItem item;
	item.SetMask(wxLIST_MASK_IMAGE);
	for (int i = 0; i < COLUMNS_NUMB; ++i) {
		const int image = i == m_lastSortedBy
			?	m_columnSortDirection[column] ? 0 : 1
			:	-1;
		item.SetImage(image);
		m_list->SetColumn(i, item);
	}

	item.SetMask(wxLIST_MASK_DATA);
	const int itemCount = m_list->GetItemCount();
	const Theme &theme = wxGetApp().GetTheme();
	for (int i = 0; i < itemCount; ++i) {
		item.SetId(i);
		if (!m_list->GetItem(item)) {
			BOOST_ASSERT(false);
			continue;
		}
		item.SetBackgroundColour(i % 2
			?	theme.GetNotEvenLineColor()
			:	theme.GetEvenLineColor());
		m_list->SetItem(item);
	}

}

namespace {
	TunnelEx::Helpers::Crypto::Key::Size GetSslCertificateKeySize(const int size) {
		using namespace TunnelEx::Helpers::Crypto;
		switch (size) {
			case Key::SIZE_512:
			case Key::SIZE_1024:
			case Key::SIZE_2048:
			case Key::SIZE_4096:
			case Key::SIZE_8192:
				return Key::Size(size);
			default:
				{
					const Key::Size defaultKeySize = Key::SIZE_2048;
					wxLogError(
						wxT("Unknown SSL certificate key size: %1%, %2% will be used."),
						size,
						int(defaultKeySize));
					BOOST_ASSERT(false);
					return defaultKeySize;
				}
		}
	}
}

void SslCertificateListDlg::OnGenerate(wxCommandEvent &) {
	
	using namespace TunnelEx::Helpers::Crypto;

	SslCertificateDlg infoDlg(this);
	if (infoDlg.ShowModal() != wxID_OK) {
		return;
	}

	class CertificateGenerator : public wxThread {
	public:
		explicit CertificateGenerator(
					Helpers::Crypto::Key::Size keySize,
					const texs__SslCertificateInfo &info,
					const std::string &signKey)
				: wxThread(wxTHREAD_JOINABLE),
				m_keySize(keySize),
				m_info(info),
				m_signKey(signKey) {
			//...//
		}
		virtual ~CertificateGenerator() {
			//...//
		}
	public:
		bool Generate() {
			Create();
			Run();
			wxProgressDialog progress(
				wxT("Generating SSL Certificate..."),
				wxT("Processing SSL Certificate generation, please wait..."),
				100,
				NULL,
				wxPD_APP_MODAL | wxPD_SMOOTH);
			for ( ; IsRunning(); progress.Pulse(), wxMilliSleep(25));
			if (!m_error.IsEmpty()) {
				wxLogError(
					wxT("Could not generate SSL Certificate: %s."),
					m_error);
				return false;
			}
			BOOST_ASSERT(!m_privateKeyStr.IsEmpty());
			BOOST_ASSERT(!m_certificateStr.IsEmpty());
			return true;
		}
	public:
		const wxString & GetPrivateKey() const {
			return m_privateKeyStr;
		}
		const wxString & GetCertificate() const {
			return m_certificateStr;
		}
	public:
		
		virtual ExitCode Entry() {
		
			const PrivateKey *signKey = 0;

			std::auto_ptr<const PrivateKey> signKeyImpl;
			if (!m_signKey.empty()) {
				try {
					signKeyImpl.reset(new PrivateKey(m_signKey));
					signKey = signKeyImpl.get();
				} catch (const TunnelEx::Helpers::Crypto::Exception &ex) {
					Format message("Could not load sign key: %1%. Please check the file format.");
					message % ex.what();
					m_error = wxString::FromAscii(message.str().c_str());
					return 0;
				}
			}
				
			try {

				const std::auto_ptr<const Rsa> rsa(Rsa::Generate(m_keySize));
				if (!signKey) {
					BOOST_ASSERT(m_signKey.empty());
					BOOST_ASSERT(!signKeyImpl.get());
					signKey = &rsa->GetPrivateKey();
				}

				const std::auto_ptr<const X509Private> cert(
					X509Private::GenerateVersion3(
						rsa->GetPrivateKey(),
						rsa->GetPublicKey(),
						*signKey,
						m_info.issuerCommonName,
						m_info.subjectOrganization,
						m_info.subjectOrganizationUnit,
						m_info.subjectCity,
						m_info.subjectStateOrProvince,
						m_info.subjectCountry,
						m_info.subjectCommonName,
						m_info.subjectOrganization,
						m_info.subjectOrganizationUnit,
						m_info.subjectCity,
						m_info.subjectStateOrProvince,
						m_info.subjectCountry));
		
				m_privateKeyStr
					= wxString::FromAscii(cert->GetPrivateKey().Export().c_str());
				m_certificateStr = wxString::FromAscii(cert->Export().c_str());
			
			} catch (const TunnelEx::Helpers::Crypto::Exception &ex) {
				Format message("Could not generate new SSL certificate: %1%.");
				message % ex.what();
				m_error = wxString::FromAscii(message.str().c_str());
				return 0;
			}
			
			return 0;

		}

	private:
	
		const Helpers::Crypto::Key::Size m_keySize;
		const texs__SslCertificateInfo &m_info;
		const std::string m_signKey;
		wxString m_privateKeyStr;
		wxString m_certificateStr;
		wxString m_error;

	} generator(
		GetSslCertificateKeySize(infoDlg.GetCertificate().keySize),
		infoDlg.GetCertificate(),
		infoDlg.GetSignKey());

	if (!generator.Generate()) {
		return;
	}

	SslGenerationResultDlg resultDlg(
		SslGenerationResultDlg::MODE_CERTIFICATE,
		generator.GetPrivateKey(),
		generator.GetCertificate(),
		this);
	if (resultDlg.ShowModal() == wxID_OK) {
		const std::string buffer = generator.GetCertificate().ToAscii();
		m_service.GetService().ImportSslCertificateX509(
			std::vector<unsigned char>(buffer.begin(), buffer.end()),
			std::string(generator.GetPrivateKey().ToAscii()));
		RefreshList();
	}

}

void SslCertificateListDlg::OnRequest(wxCommandEvent &) {

	using namespace TunnelEx::Helpers::Crypto;

	SslCertificateDlg dlg(this);
	if (dlg.ShowModal() != wxID_OK) {
		return;
	}

	class RequestGenerator : public wxThread {
	public:
		explicit RequestGenerator(
					Helpers::Crypto::Key::Size keySize,
					const texs__SslCertificateInfo &info,
					const std::string &signKey)
				: wxThread(wxTHREAD_JOINABLE),
				m_keySize(keySize),
				m_info(info),
				m_signKey(signKey) {
			//...//
		}
		virtual ~RequestGenerator() {
			//...//
		}
	public:
		bool Generate() {
			Create();
			Run();
			wxProgressDialog progress(
				wxT("Generating CSR..."),
				wxT("Processing Certificate Signing Request generation, please wait..."),
				100,
				NULL,
				wxPD_APP_MODAL | wxPD_SMOOTH);
			for ( ; IsRunning(); progress.Pulse(), wxMilliSleep(25));
			if (!m_error.IsEmpty()) {
				wxLogError(
					wxT("Could not generate Certificate Signing Request: %s."),
					m_error);
				return false;
			}
			BOOST_ASSERT(!m_keyStr.IsEmpty());
			BOOST_ASSERT(!m_requestStr.IsEmpty());
			return true;
		}
	public:
		const wxString & GetPrivateKey() const {
			return m_keyStr;
		}
		const wxString & GetCsr() const {
			return m_requestStr;
		}
	public:
		virtual ExitCode Entry() {
			try {
				std::auto_ptr<const PrivateKey> signKey;
				if (!m_signKey.empty()) {
					try {
						signKey.reset(new PrivateKey(m_signKey));
					} catch (const TunnelEx::Helpers::Crypto::Exception &) {
						m_error = L"Could not load sign key. Please check the file format.";
						return 0;
					}
				}
				const std::auto_ptr<Rsa> rsa = Rsa::Generate(m_keySize);
				X509Request request(
					rsa->GetPublicKey(),
					signKey.get() ? *signKey : rsa->GetPrivateKey(),
					m_info.subjectCommonName,
					m_info.subjectOrganization,
					m_info.subjectOrganizationUnit,
					m_info.subjectCity,
					m_info.subjectStateOrProvince,
					m_info.subjectCountry);
				m_keyStr
					= wxString::FromAscii(rsa->GetPrivateKey().Export().c_str());
				m_requestStr
					= wxString::FromAscii(request.Export().c_str());
			} catch (const TunnelEx::Helpers::Crypto::Exception &ex) {
				m_error = wxString::FromAscii(ex.what());
			}
			return 0;
		}
	private:
		const Helpers::Crypto::Key::Size m_keySize;
		const texs__SslCertificateInfo &m_info;
		const std::string m_signKey;
		wxString m_keyStr;
		wxString m_requestStr;
		wxString m_error;
	} generator(
		GetSslCertificateKeySize(dlg.GetCertificate().keySize),
		dlg.GetCertificate(),
		dlg.GetSignKey());

	if (!generator.Generate()) {
		return;
	}

	SslGenerationResultDlg(
			SslGenerationResultDlg::MODE_REQUEST,
			generator.GetPrivateKey(),
			generator.GetCsr(),
			this)
		.ShowModal();

}

void SslCertificateListDlg::OnImport(wxCommandEvent &) {

	wxFileDialog fileRequestDlg(
		this,
		wxT("Choose a file to import certificate"),
		wxEmptyString,
		wxEmptyString,
		wxT("All supported files|*.cer;*.crt;*.pfx;*.p12")
			wxT("|Certificates X.509 files (*.cer; *.crt)|*.cer;*.crt")
			wxT("|Personal Information Exchange (*.pfx; *.p12)|*.pfx;*.p12")
			wxT("|All files (*.*)|*.*"),
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fileRequestDlg.ShowModal() != wxID_OK) {
		return;
	}

	std::vector<unsigned char> buffer;
	{
		std::ifstream f(fileRequestDlg.GetPath().c_str(), std::ios::binary);
		if (!f) {
			wxLogError(wxT("Could not open %s."), fileRequestDlg.GetPath());
			return;
		}
		f.seekg(0, std::ios::end);
		buffer.resize(f.tellg());
		f.seekg(0, std::ios::beg);
		f.read(reinterpret_cast<char *>(&buffer[0]), buffer.size());
	}

	std::string password;
	std::string privateKey;
	wxString upperName = fileRequestDlg.GetPath();
	upperName.MakeUpper();
	const bool isX509
		= upperName.EndsWith(wxT(".CRT")) || upperName.EndsWith(wxT(".CER"));
	
	if (!isX509) {
		password = wxGetPasswordFromUser(
				wxT("Please provide certificate password, if required."),
				wxT("Certificate password"),
				wxEmptyString,
				this)
			.ToAscii();
	} else {
		const int answer = wxMessageBox(
			wxT("Do you have the private key for this certificate?")
				wxT(" Click Yes to choose a private key file."),
			wxT("Certificate Private Key"),
			wxYES_NO | wxCENTER | wxICON_QUESTION,
			this);
		if (answer == wxYES) {
			wxFileDialog fileRequestDlg(
				this,
				wxT("Choose a file to import private key"),
				wxEmptyString,
				wxEmptyString,
				wxT("Privacy Enhanced Mail (*.pem)|*.pem|All files (*.*)|*.*"),
				wxFD_OPEN | wxFD_FILE_MUST_EXIST);
			if (fileRequestDlg.ShowModal() != wxID_OK) {
				wxMessageBox(
					wxT("Import the key was canceled."),
					wxT("Import"),
					wxOK | wxCENTER | wxICON_WARNING,
					this);
				return;
			}
			std::ifstream f(fileRequestDlg.GetPath().c_str(), std::ios::binary);
			if (!f) {
				wxLogError(wxT("Could not open %s."), fileRequestDlg.GetPath());
				return;
			}
			f.seekg(0, std::ios::end);
			std::vector<unsigned char> buffer;
			buffer.resize(f.tellg());
			f.seekg(0, std::ios::beg);
			f.read(reinterpret_cast<char *>(&buffer[0]), buffer.size());
			std::string(buffer.begin(), buffer.end()).swap(privateKey);
		}
	}

	if (isX509) {
		m_service.GetService().ImportSslCertificateX509(buffer, privateKey);
	} else {
		m_service.GetService().ImportSslCertificatePkcs12(buffer, password);
	}

	RefreshList();

}

void SslCertificateListDlg::OnView(wxCommandEvent &) {
	ViewSelected();
}

void SslCertificateListDlg::OnListItemActivated(wxListEvent &) {
	ViewSelected();
}

void SslCertificateListDlg::ViewSelected() {
	SslCertificateIdCollection selected;
	GetSelected(selected);
	BOOST_ASSERT(selected.GetSize() > 0);
	if (selected.GetSize() == 0) {
		return;
	}
	texs__SslCertificateInfo certificate;
	m_service.GetService().GetSslCertificate(selected[0].GetCStr(), certificate);
	if (!certificate.id.empty()) {
		if (SslCertificateDlg(certificate, this).ShowModal() == wxID_OK) {
			RefreshList();
		}
	} else {
		wxLogError(L"Could not get SSL certificate.");
		RefreshList();
	}
}

void SslCertificateListDlg::GetSelected(SslCertificateIdCollection &result) const {
	SslCertificateIdCollection selected;
	for (long item = -1; ; ) {
		item = m_list->GetNextItem(
			item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1) {
			break;
		}
		selected.Append(GetCertificate(m_list->GetItemData(item)).id.c_str());
	}
	selected.Swap(result);
}

void SslCertificateListDlg::OnDel(wxCommandEvent &) {
	SslCertificateIdCollection selected;
	GetSelected(selected);
	BOOST_ASSERT(selected.GetSize() > 0);
	if (selected.GetSize() > 0) {
		const int answer = wxMessageBox(
			wxT("Delete selected certificates?"),
			wxT("Confirm delete"),
			wxYES_NO | wxNO_DEFAULT | wxCENTER | wxICON_WARNING,
			this);
		if (answer != wxYES) {
			return;
		}
		std::list<std::wstring> texsIds;
		for (size_t i = 0; i < selected.GetSize(); ++i) {
			texsIds.push_back(selected[i].GetCStr());
		}
		m_service.GetService().DeleteSslCertificates(texsIds);
		RefreshList();
	}
}

// see TEX-642
/* void SslCertificateListDlg::OnExport(wxCommandEvent &) {
	list<std::string> selected;
	GetSelected(selected);
	BOOST_ASSERT(selected.size() > 0);
	if (selected.size() == 0) {
		return;
	}
	list<texs__SslCertificate> certificates;
	m_service.GetService().GetSslCertificates(true, certificates);
	foreach (const texs__SslCertificate &certificate, certificates) {
		bool isSelected = false;
		foreach (const std::wstring &id, selected) {
			if (id == certificate.id) {
				isSelected = true;
				break;
			}
		}
		if (!isSelected) {
			continue;
		}
		const std::string exported
			= m_service.GetService().ExportSslCertificate(certificate.id);
		wxFileDialog fileRequestDlg(
			this,
			wxT("Choose a file to export certificate"),
			wxEmptyString,
			wxEmptyString,
			wxT("All files (*.*)|*.*"),
			wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		if (fileRequestDlg.ShowModal() != wxID_OK) {
			break;
		}
		BOOST_ASSERT(!fileRequestDlg.GetPath().IsEmpty());
		ofstream f(fileRequestDlg.GetPath().c_str(), std::ios::trunc | std::ios::binary);
		if (!f) {
			wxLogError(wxT("Could not open file %d."), fileRequestDlg.GetPath().c_str());
			break;
		} else {
			f << exported;
		}
	}
	RefreshList(&certificates);
} */

// see TEX-642
/* void SslCertificateListDlg::OnBackup(wxCommandEvent &) {
	list<texs__SslCertificate> certificates;
	m_service.GetService().GetSslCertificates(true, certificates);
	wxFileDialog fileRequestDlg(
		this,
		wxT("Choose a file to backup certificates"),
		wxEmptyString,
		wxEmptyString,
		wxT("ZIP files (*.zip)|*.zip|All files (*.*)|*.*"),
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fileRequestDlg.ShowModal() != wxID_OK) {
		return;
	}
	size_t count = 0;
	BOOST_ASSERT(!fileRequestDlg.GetPath().IsEmpty());
	{
		wxFFileOutputStream out(fileRequestDlg.GetPath());
		BOOST_ASSERT(out.IsOk());
		if (!out.IsOk()) {
			return;
		}
		wxZipOutputStream zip(out);
		BOOST_ASSERT(zip.IsOk());
		if (!zip.IsOk()) {
			return;
		}
		zip.SetLevel(9);
		foreach (const texs__SslCertificate &certificate, certificates) {
			std::wstring exported = m_service.GetService().ExportSslCertificate(certificate.id);
			if (!exported.empty()) {
				zip.PutNextEntry(wxString::FromAscii(certificate.id.c_str()));
				wxStringInputStream exportedStream(wxString::FromAscii(exported.c_str()));
				zip.Write(exportedStream);
				++count;
			}
		}
	}
	if (count > 0) {
		WFormat message(wxT("Successfully saved %1% rule(s)."));
		message % count;
		wxMessageBox(
			message.str().c_str(),
			wxT("SSL Certificates backup"),
			wxOK | wxICON_INFORMATION,
			this);
	} else {
		wxMessageBox(
			wxT("No certificates were found to backup."),
			wxT("SSL Certificates backup"),
			wxOK | wxICON_ERROR,
			this);
	}
	RefreshList(&certificates);
} */

// see TEX-642
/* void SslCertificateListDlg::OnRestore(wxCommandEvent &) {

	const int answer = wxMessageBox(
		wxT("This operation can replace the installed certificates.")
			wxT(" Do not forget to make the current backup before applying.")
			wxT("\n\nContinue the restore operation?"),
		wxT("Confirm restore"),
		wxYES_NO | wxNO_DEFAULT | wxCENTER | wxICON_WARNING,
		this);
	if (answer != wxYES) {
		return;
	}

	wxLogNull logNo;

	wxFileDialog fileRequestDlg(
		this,
		wxT("Choose a file to restore certificates"),
		wxEmptyString,
		wxEmptyString,
		wxT("ZIP files (*.zip)|*.zip|All files (*.*)|*.*"),
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fileRequestDlg.ShowModal() != wxID_OK) {
		return;
	}

	wxFFileInputStream in(fileRequestDlg.GetPath());
	BOOST_ASSERT(in.IsOk());
	BOOST_ASSERT(in.IsSeekable());
	if (!in.IsOk() || !in.IsSeekable()) {
		return;
	}

	{
		wxZipInputStream zip(in);
		BOOST_ASSERT(zip.IsOk());
		if (zip.IsOk()) {
			std::auto_ptr<wxZipEntry> entry;
			while (entry.reset(zip.GetNextEntry()), entry.get() != NULL) {
				wxStringOutputStream stream;
				zip.Read(stream);
				m_service.GetService().SetSslCertificate(
					ConvertString<String>(entry->GetName().c_str()).GetCStr(),
					ConvertString<String>(stream.GetString().c_str()).GetCStr());
			}
		}
	}

	RefreshList();

} */

void SslCertificateListDlg::OnListContextMenu(wxContextMenuEvent &) {

	const int selected = m_list->GetSelectedItemCount();
	if (selected == 0) {
		return;
	}

	wxMenu menu;
	wxMenuItem *item;

	item = new wxMenuItem(&menu, CONTROL_VIEW, wxT("&View..."));
	item->Enable(selected == 1);
	menu.Append(item);

	item = new wxMenuItem(&menu, CONTROL_DEL, wxT("&Delete"));
	menu.Append(item);

	// see TEX-642
	/* item = new wxMenuItem(&menu, CONTROL_EXPORT, wxT("&Export"));
	menu.Append(item); */

	PopupMenu(&menu, wxDefaultPosition);

}

void SslCertificateListDlg::OnListItemSelectionChange(wxListEvent &) {
	CheckSelection();
}

const texs__SslCertificateShortInfo & SslCertificateListDlg::GetCertificate(
			const long data)
		const {
	return *reinterpret_cast<texs__SslCertificateShortInfo *>(data);
}

void SslCertificateListDlg::OnHelp(wxCommandEvent &) {
	wxGetApp().DisplayHelp(wxT("dialogs/ssl-certificates"));
}
