/**************************************************************************
 *   Created: 2008/03/31 8:58
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 * -------------------------------------------------------------------
 *       $Id: Theme.hpp 1049 2010-11-07 16:40:27Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Icons_hpp__0803310858
#define INCLUDED_FILE__TUNNELEX__Icons_hpp__0803310858

class wxBitmap;

class Theme : private boost::noncopyable {

public:
	
	Theme();
	~Theme() throw();

public:

	wxSize GetIconButtonSize() const;

	void GetAddItemButton(wxBitmap &buffer) const;
	void GetAddItemButtonDisabled(wxBitmap &buffer) const;

	void GetEditItemButton(wxBitmap &buffer) const;
	void GetEditItemButtonDisabled(wxBitmap &buffer) const;

	void GetRemoveItemButton(wxBitmap &buffer) const;
	void GetRemoveItemButtonDisabled(wxBitmap &buffer) const;

	void GetArrowUpButton(wxBitmap &buffer) const;
	void GetArrowUpButtonDisabled(wxBitmap &buffer) const;

	void GetArrowDownButton(wxBitmap &buffer) const;
	void GetArrowDownButtonDisabled(wxBitmap &buffer) const;

	void GetRuleIcon(wxBitmap &buffer, bool big) const;
	void GetAddRuleIcon(wxBitmap &buffer, bool big) const;
	void GetEditRuleIcon(wxBitmap &buffer, bool big) const;
	void GetRemoveRuleIcon(wxBitmap &buffer, bool big) const;
	void GetWarningRuleIcon(wxBitmap &buffer) const;
	void GetGoRuleIcon(wxBitmap &buffer, bool big) const;

	void GetKeyIcon(wxBitmap &buffer) const;

	void GetCutIcon(wxBitmap &buffer, bool big) const;
	void GetCopyIcon(wxBitmap &buffer, bool big) const;
	void GetPasteIcon(wxBitmap &buffer, bool big) const;

	void GetBlogIcon(wxBitmap &buffer) const;
	void GetBugIcon(wxBitmap &buffer, bool big) const;

	void GetHelpIcon(wxBitmap &buffer, bool big) const;

	void GetPlayIcon(wxBitmap &buffer, bool big) const;
	void GetStopIcon(wxBitmap &buffer, bool big) const;

	void GetLogIcon(wxBitmap &buffer, bool big) const;

	void GetGoProIcon(wxBitmap &buffer, bool big) const;

	void GetSortAscIcon(wxBitmap &buffer) const;
	void GetSortDescIcon(wxBitmap &buffer) const;

	int GetDlgBorder() const;
	int GetDlgBottomBorder() const;
	int GetTopSizerBorder() const;
	wxSizerFlags GetTopSizerFlags() const;
	wxSizerFlags GetStaticBoxFlags() const;

	const wxColour & GetEvenLineColor() const;
	const wxColour & GetNotEvenLineColor() const;

private:

	class Implementation;
	std::auto_ptr<Implementation> m_pimpl;

};

#endif // INCLUDED_FILE__TUNNELEX__Icons_hpp__0803310858