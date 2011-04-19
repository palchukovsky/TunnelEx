/**************************************************************************
 *   Created: 2007/11/15 15:02
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__RulesListBox_h__0711151502
#define INCLUDED_FILE__TUNNELEX__RulesListBox_h__0711151502

#include "Rule.hpp"

class RuleListCtrl : public wxListCtrl {

	friend int wxCALLBACK SortByName(long, long, long);
	friend int wxCALLBACK SortByInputs(long, long, long);
	friend int wxCALLBACK SortByDestintations(long, long, long);
	friend int wxCALLBACK SortByState(long, long, long);
	
public:

	RuleListCtrl(
			const RulesMap &rulesHolder,
			const NotAppliedRulesUuids &notAppliedRulesIdsHolder,
			wxWindow *parent,
			wxWindowID id = wxID_ANY,
			const wxPoint &pos = wxDefaultPosition,
			const wxSize &size = wxDefaultSize,
			long style = wxLC_ICON,
			const wxValidator &validator = wxDefaultValidator,
			const wxString &name = wxT("RuleList"));
	~RuleListCtrl();

	void SortByName();
	void SortByInputs();
	void SortByDestinations();
	void SortByState();

	void OnSort(wxListEvent &);
	void OnColumnResized(wxListEvent &);
	void OnItemContextMenu(wxContextMenuEvent &);

	void RefreshList(bool isServerStarted);

	void OnItemActivated(wxListEvent &);

	enum Column;

	const TunnelEx::Rule * GetFirstSelectedRule() const;
	void GetSelected(RulesUuids &ids) const;

	bool SetColumnWidth(int col, int width);

	void SelectAll();
	void Select(const RulesUuids &);

private:

	class Implementation;
	std::auto_ptr<Implementation> m_pimpl;

	DECLARE_NO_COPY_CLASS(RuleListCtrl)
	DECLARE_EVENT_TABLE()

};

#endif // INCLUDED_FILE__TUNNELEX__RulesListBox_h__0711151502
