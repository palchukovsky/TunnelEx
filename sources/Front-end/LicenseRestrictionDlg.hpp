/**************************************************************************
 *   Created: 2010/04/07 1:58
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: LicenseRestrictionDlg.hpp 1080 2010-12-01 08:33:26Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__LicenseRestrictionDlg_hpp__1004070158
#define INCLUDED_FILE__TUNNELEX__LicenseRestrictionDlg_hpp__1004070158

class ServiceWindow;

class LicenseRestrictionDlg : public wxDialog {

private:

	enum Control {
		CTRL_REQUEST_TRIAL,
		CTRL_ORDER
	};

public:
	
	template<typename License>
	explicit LicenseRestrictionDlg(
				ServiceWindow &service,
				wxWindow *parent,
				const License &license,
				bool isLastDialog,
				wxWindowID id = wxID_ANY)
			: wxDialog(
				parent,
				id,
				wxT("License upgrade required"),
				wxDefaultPosition,
				wxSize(325, 100),
				wxCAPTION | wxCLOSE_BOX | wxSYSTEM_MENU),
			m_service(service) {
		CreateControls(license.IsTrial(), isLastDialog);
	}
	~LicenseRestrictionDlg();

	DECLARE_NO_COPY_CLASS(LicenseRestrictionDlg);

public:

	void OnRequestTrial(wxCommandEvent &);
	void OnOrderPage(wxCommandEvent &);

private:

	void CreateControls(const bool isTrial, const bool isLastDlg);

private:

	ServiceWindow &m_service;

	DECLARE_EVENT_TABLE();

};

#endif // INCLUDED_FILE__TUNNELEX__LicenseRestrictionDlg_hpp__1004070158
