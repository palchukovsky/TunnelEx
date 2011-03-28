/**************************************************************************
 *   Created: 2010/04/07 2:00
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: LicenseRestrictionDlg.cpp 1043 2010-10-25 10:22:29Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "LicenseRestrictionDlg.hpp"
#include "ServiceWindow.hpp"
#include "Application.hpp"

using namespace TunnelEx;

BEGIN_EVENT_TABLE(LicenseRestrictionDlg, wxDialog)
	EVT_BUTTON(LicenseRestrictionDlg::CTRL_REQUEST_TRIAL, LicenseRestrictionDlg::OnRequestTrial)
	EVT_BUTTON(LicenseRestrictionDlg::CTRL_ORDER, LicenseRestrictionDlg::OnOrderPage)
END_EVENT_TABLE()

void LicenseRestrictionDlg::CreateControls(const bool isTrial, const bool isLastDlg) {

	const int border = 10;

	wxString message(
		wxT("The functionality you have requested requires a License Upgrade.")
			wxT(" Please click \"Upgrade\" to purchase a License that will enable")
			wxT(" this feature."));
	if (!isTrial) {
		message
			+= wxT(" Or click \"Get a Free Trial\" to try full free program version.");
	}

	const wxStaticText &noteCtrl = *new wxStaticText(
		this,
		-1,
		message,
		wxPoint(border, border),
		wxSize(GetClientSize().GetX() - (border * 2), 60),
		wxALIGN_LEFT);

	int y = noteCtrl.GetPosition().y + noteCtrl.GetSize().GetY() + border * 2;

	const wxStaticLine &line = *new wxStaticLine(
		this,
		wxID_ANY,
		wxPoint(border, y),
		wxSize(GetClientSize().GetWidth() - (border * 2), -1));
	y += border + line.GetSize().GetHeight();

	const int buttonWidth = 90;

	const wxButton &orderButton = *new wxButton(
		this,
		CTRL_ORDER,
		wxT("Upgrade"),
		wxPoint(border, y),
		wxSize(buttonWidth, -1));

	if (!isTrial) {
		new wxButton(
			this,
			CTRL_REQUEST_TRIAL,
			wxT("Get a Free Trial"),
			wxPoint(
				orderButton.GetPosition().x + orderButton.GetSize().GetWidth() + (border / 2),
				orderButton.GetPosition().y),
			wxSize(buttonWidth, -1));
	}

	const wxButton &okButton = *new wxButton(
		this,
		wxID_OK,
		isLastDlg ? wxT("Close") : wxT("Continue"),
		wxPoint(
			GetClientSize().x - border - buttonWidth,
			orderButton.GetPosition().y),
		wxSize(buttonWidth, -1));

	SetClientSize(
		GetClientSize().GetWidth(),
		okButton.GetPosition().y + okButton.GetSize().GetHeight() +  border);

}

LicenseRestrictionDlg::~LicenseRestrictionDlg() {
	//...//
}

void LicenseRestrictionDlg::OnRequestTrial(wxCommandEvent &) {
	if (m_service.ActivateTrial()) {
		wxMessageBox(
			wxT("The trial has been activated successfully.")
				wxT(" Please save the current changes and reopen the dialog")
				wxT(" to start using this feature."),
			wxT("Product trial activation"),
			wxOK | wxICON_INFORMATION,
			this);
		EndModal(wxID_OK);
	}
}

void LicenseRestrictionDlg::OnOrderPage(wxCommandEvent &) {
	wxGetApp().OpenOrderPage();
}
