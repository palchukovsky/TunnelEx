/**************************************************************************
 *   Created: 2008/03/31 8:59
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "Theme.hpp"

namespace fs = boost::filesystem;
using namespace TunnelEx::Helpers;

class Theme::Implementation : private boost::noncopyable {
public:
	Implementation()
			: m_repositoryPath(GetModuleFilePath().branch_path()),
			m_evenLineColor(255, 255, 255),
			m_notEvenLineColor(255, 255, 235) {
		m_repositoryPath /= L"templates";
	}
	~Implementation() {
		//...//
	}
public:
	void LoadPngFile(const wchar_t *const file, wxBitmap &buffer) const {
		fs::wpath path(m_repositoryPath);
		path /= file;
		buffer.LoadFile(path.string(), wxBITMAP_TYPE_PNG);
	}
private:
	fs::wpath m_repositoryPath;
public:
	const wxColour m_evenLineColor;
	const wxColour m_notEvenLineColor;
};

Theme::Theme()
		: m_pimpl(new Implementation) {
	//...//
}

Theme::~Theme() {
	//...//
}

wxSize Theme::GetIconButtonSize() const {
	return wxSize(24, 24);
}

void Theme::GetAddItemButton(wxBitmap &buffer) const {
	m_pimpl->LoadPngFile(L"AddItemButton.png", buffer);
}

void Theme::GetAddItemButtonDisabled(wxBitmap &buffer) const {
	m_pimpl->LoadPngFile(L"AddItemButtonDisabled.png", buffer);
}

void Theme::GetRemoveItemButton(wxBitmap &buffer) const {
	m_pimpl->LoadPngFile(L"RemoveItemButton.png", buffer);
}

void Theme::GetEditItemButton(wxBitmap &buffer) const {
	m_pimpl->LoadPngFile(L"EditItemButton.png", buffer);
}

void Theme::GetEditItemButtonDisabled(wxBitmap &buffer) const {
	m_pimpl->LoadPngFile(L"EditItemButtonDisabled.png", buffer);
}

void Theme::GetRemoveItemButtonDisabled(wxBitmap &buffer) const {
	m_pimpl->LoadPngFile(L"RemoveItemButtonDisabled.png", buffer);
}

void Theme::GetArrowUpButton(wxBitmap &buffer) const {
	m_pimpl->LoadPngFile(L"ArrowUpButton.png", buffer);
}

void Theme::GetArrowUpButtonDisabled(wxBitmap &buffer) const {
	m_pimpl->LoadPngFile(L"ArrowUpButtonDisabled.png", buffer);
}

void Theme::GetArrowDownButton(wxBitmap &buffer) const {
	m_pimpl->LoadPngFile(L"ArrowDownButton.png", buffer);
}

void Theme::GetArrowDownButtonDisabled(wxBitmap &buffer) const {
	m_pimpl->LoadPngFile(L"ArrowDownButtonDisabled.png", buffer);
}

void Theme::GetRuleIcon(wxBitmap &buffer, bool big) const {
	m_pimpl->LoadPngFile(big ? L"RuleIconBig.png" : L"RuleIcon.png", buffer);
}

void Theme::GetAddRuleIcon(wxBitmap &buffer, bool big) const {
	m_pimpl->LoadPngFile(big ? L"AddRuleIconBig.png" : L"AddRuleIcon.png", buffer);
}

void Theme::GetEditRuleIcon(wxBitmap &buffer, bool big) const {
	m_pimpl->LoadPngFile(big ? L"EditRuleIconBig.png" : L"EditRuleIcon.png", buffer);
}

void Theme::GetRemoveRuleIcon(wxBitmap &buffer, bool big) const {
	m_pimpl->LoadPngFile(big ? L"RemoveRuleIconBig.png" : L"RemoveRuleIcon.png", buffer);
}

void Theme::GetWarningRuleIcon(wxBitmap &buffer) const {
	m_pimpl->LoadPngFile(L"WarningRuleIcon.png", buffer);
}

void Theme::GetGoRuleIcon(wxBitmap &buffer, bool big) const {
	m_pimpl->LoadPngFile(big ? L"GoRuleIconBig.png" : L"GoRuleIcon.png", buffer);
}

void Theme::GetKeyIcon(wxBitmap &buffer) const {
	m_pimpl->LoadPngFile(L"KeyIcon.png", buffer);
}

void Theme::GetCutIcon(wxBitmap &buffer, bool big) const {
	m_pimpl->LoadPngFile(big ? L"CutIconBig.png" : L"CutIcon.png", buffer);
}

void Theme::GetCopyIcon(wxBitmap &buffer, bool big) const {
	m_pimpl->LoadPngFile(big ? L"CopyIconBig.png" : L"CopyIcon.png", buffer);
}

void Theme::GetPasteIcon(wxBitmap &buffer, bool big) const {
	m_pimpl->LoadPngFile(big ? L"PasteIconBig.png" : L"PasteIcon.png", buffer);
}

void Theme::GetBlogIcon(wxBitmap &buffer) const {
	m_pimpl->LoadPngFile(L"BloggerIcon.png", buffer);
}

void Theme::GetHelpIcon(wxBitmap &buffer, bool big) const {
	m_pimpl->LoadPngFile(big ? L"HelpIconBig.png" : L"HelpIcon.png", buffer);
}

void Theme::GetBugIcon(wxBitmap &buffer, bool big) const {
	m_pimpl->LoadPngFile(big ? L"BugIconBig.png" : L"BugIcon.png", buffer);
}

void Theme::GetPlayIcon(wxBitmap &buffer, bool big) const {
	m_pimpl->LoadPngFile(big ? L"PlayIconBig.png" : L"PlayIcon.png", buffer);
}

void Theme::GetStopIcon(wxBitmap &buffer, bool big) const {
	m_pimpl->LoadPngFile(big ? L"StopIconBig.png" : L"StopIcon.png", buffer);
}

void Theme::GetLogIcon(wxBitmap &buffer, bool big) const {
	m_pimpl->LoadPngFile(big ? L"LogIconBig.png" : L"LogIcon.png", buffer);
}

void Theme::GetGoProIcon(wxBitmap &buffer, bool big) const {
	m_pimpl->LoadPngFile(big ? L"GoProIconBig.png" : L"GoProIcon.png", buffer);
}

int Theme::GetDlgBorder() const {
	return 8;
}

int Theme::GetDlgBottomBorder() const {
	return GetDlgBorder() * 2;
}

int Theme::GetTopSizerBorder() const {
	return GetDlgBorder() * 1.5;
}

wxSizerFlags Theme::GetTopSizerFlags() const {
	return wxSizerFlags(0)
		.Expand()
		.Border(wxLEFT | wxRIGHT | wxTOP, GetTopSizerBorder());
}

wxSizerFlags Theme::GetStaticBoxFlags() const {
	return wxSizerFlags(1).Expand().Border(wxALL, GetDlgBorder() * 0.7);
}

const wxColour & Theme::GetEvenLineColor() const {
	return m_pimpl->m_evenLineColor;
}

const wxColour & Theme::GetNotEvenLineColor() const {
	return m_pimpl->m_notEvenLineColor;
}

void Theme::GetSortAscIcon(wxBitmap &buffer) const {
	const char *const xpm[]
		= {
			"16 16 2 1",
			". c #ACA899",
			"  c None",
			"                ",
			"                ",
			"                ",
			"                ",
			"                ",
			"                ",
			"        .       ",
			"       ...      ",
			"      .....     ",
			"     .......    ",
			"    .........   ",
			"                ",
			"                ",
			"                ",
			"                ",
			"                "}; 
	buffer = wxBitmap(xpm);
}

void Theme::GetSortDescIcon(wxBitmap &buffer) const {
	const char *const xpm[]
		= {
			"16 16 2 1",
			". c #ACA899",
			"  c None",
			"                ",
			"                ",
			"                ",
			"                ",
			"                ",
			"                ",
			"    .........   ",
			"     .......    ",
			"      .....     ",
			"       ...      ",
			"        .       ",
			"                ",
			"                ",
			"                ",
			"                ",
			"                "};
	buffer = wxBitmap(xpm);	
}
