/**************************************************************************
 *   Created: 2010/06/27 13:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: NewVersionDlg.cpp 1127 2011-02-22 17:23:32Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "NewVersionDlg.hpp"
#include "Application.hpp"


using namespace std;

NewVersionDlg::NewVersionDlg(
			const UpdateChecker::Version &version,
			wxWindow *parent)
		: wxDialog(
			parent,
			wxID_ANY,
			wxT("New version is available!")) {
	auto_ptr<wxBoxSizer> sizer(new wxBoxSizer(wxVERTICAL));
	sizer->Add(
		new wxStaticText(
			this,
			wxID_ANY,
			wxT("A new version of ") TUNNELEX_NAME_W wxT(" is available!"),
			wxDefaultPosition,
			wxDefaultSize,
			wxALIGN_CENTRE),
		wxGetApp().GetTheme().GetTopSizerFlags().Center().Expand());
	sizer->Add(
		new wxStaticText(
			this,
			wxID_ANY,
			wxT("Your version is: ") TUNNELEX_VERSION_W,
			wxDefaultPosition,
			wxDefaultSize,
			wxALIGN_CENTRE),
		wxGetApp().GetTheme().GetTopSizerFlags().Center());
	sizer->Add(
		new wxStaticText(
			this,
			wxID_ANY,
			(TunnelEx::WFormat(L"Current version is: %1%.%2%.%3%")
					% version.majorHigh
					% version.majorLow
					% version.minorHigh)
				.str(),
			wxDefaultPosition,
			wxDefaultSize,
			wxALIGN_CENTRE),
		wxSizerFlags().Expand().Center());
	sizer->Add(
		new wxHyperlinkCtrl(
			this,
			wxID_ANY,
			wxT("Please go to http://") TUNNELEX_DOMAIN_W wxT(" to get it."),
			wxT("http://") TUNNELEX_DOMAIN_W wxT("/?about")),
		wxGetApp().GetTheme().GetTopSizerFlags().Center());
	
	sizer->Add(new wxStaticLine(this), wxGetApp().GetTheme().GetTopSizerFlags());
	
	auto_ptr<wxBoxSizer> buttonSizer(new wxBoxSizer(wxHORIZONTAL));
	m_doNotCheckCtrl = new wxCheckBox(
		this,
		wxID_ANY,
		wxT("Do not check new versions"),
		wxDefaultPosition,
		wxDefaultSize,
		wxCHK_2STATE);
	buttonSizer->Add(m_doNotCheckCtrl);
	buttonSizer->AddSpacer(wxGetApp().GetTheme().GetDlgBorder() * 2);
	buttonSizer->Add(new wxButton(this, wxID_CANCEL, wxT("Close")));

	sizer->Add(buttonSizer.get(), wxGetApp().GetTheme().GetTopSizerFlags());
	buttonSizer.release();

	sizer->AddSpacer(wxGetApp().GetTheme().GetDlgBottomBorder());

	sizer->SetMinSize(GetMinSize());
	SetSizer(sizer.get());
	sizer.release()->SetSizeHints(this);
	Center();

}

NewVersionDlg::~NewVersionDlg() {
	//...//
}

bool NewVersionDlg::IsNewVersionCheckingOn() const {
	return !m_doNotCheckCtrl->GetValue();
}
