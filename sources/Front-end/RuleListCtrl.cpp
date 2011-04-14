/**************************************************************************
 *   Created: 2007/11/15 15:09
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: RuleListCtrl.cpp 1082 2010-12-02 07:57:11Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "Application.hpp"
#include "RuleListCtrl.hpp"
#include "ServiceWindow.hpp"
#include "Config.h"
#include "MainFrame.hpp"
#include "ServiceAdapter.hpp"

#include "Modules/Inet/InetEndpointAddress.hpp"
#include "Modules/Serial/SerialEndpointAddress.hpp"
#include "Modules/Upnp/UpnpEndpointAddress.hpp"
#include "Modules/Upnp/UpnpcService.hpp"

//////////////////////////////////////////////////////////////////////////

using namespace TunnelEx;

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(RuleListCtrl, wxListCtrl)
	EVT_LIST_COL_CLICK(wxID_ANY, RuleListCtrl::OnSort)
	EVT_LIST_COL_END_DRAG(wxID_ANY, RuleListCtrl::OnColumnResized)
	EVT_LIST_ITEM_ACTIVATED(wxID_ANY, RuleListCtrl::OnItemActivated)
	EVT_CONTEXT_MENU(RuleListCtrl::OnItemContextMenu)
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////

// Do not forget change config-file version when
// columns will be added or removed.
enum RuleListCtrl::Column {
	COLUMN_NAME,
	COLUMN_INPUTS,
	COLUMN_DESTINATIONS,
	COLUMN_STATE,
	COLUMNS_NUMB
};

//////////////////////////////////////////////////////////////////////////

class RuleListCtrl::Implementation : private boost::noncopyable {

	friend class RuleListCtrl;

	friend int wxCALLBACK SortByName(long, long, long);
	friend int wxCALLBACK SortByInputs(long, long, long);
	friend int wxCALLBACK SortByDestintations(long, long, long);
	friend int wxCALLBACK SortByState(long, long, long);
	
public:

	//! Used for sorting.
	/** Can't get the object pointer in sort-function as
		pointer size on 64-bit OS less then size of long). */
	typedef long ControlIndex;
	//! Used for sorting.
	/** Can't get the object pointer in sort-function as
		pointer size on 64-bit OS less then size of long). */
	typedef std::map<ControlIndex, Implementation*> ControlsMap;

public:
	
	Implementation(
				const RulesMap &rules,
				const NotAppliedRulesUuids &notAppliedRules,
				ControlIndex controlIndex,
				RuleListCtrl &myInterface)
			: m_controlIndex(controlIndex),
			m_myInterface(myInterface),
			m_lastSortedBy(COLUMNS_NUMB),
			m_rules(rules),
			m_notAppliedRules(notAppliedRules),
			m_fistTimeRulesSet(true) {

		std::auto_ptr<wxImageList> imageList(new wxImageList(16, 16, true));
		wxBitmap icon;
		wxGetApp().GetTheme().GetSortAscIcon(icon);
		imageList->Add(icon);
		wxGetApp().GetTheme().GetSortDescIcon(icon);
		imageList->Add(icon);
		wxGetApp().GetTheme().GetGoRuleIcon(icon, false);
		imageList->Add(icon);
		wxGetApp().GetTheme().GetEditRuleIcon(icon, false);
		imageList->Add(icon);
		wxGetApp().GetTheme().GetRuleIcon(icon, false);
		imageList->Add(icon);
		wxGetApp().GetTheme().GetAddRuleIcon(icon, false);
		imageList->Add(icon);
		wxGetApp().GetTheme().GetRemoveRuleIcon(icon, false);
		imageList->Add(icon);
		myInterface.AssignImageList(imageList.get(), wxIMAGE_LIST_SMALL);
		imageList.release();

		Config &config = wxGetApp().GetConfig();
		bool flag = false;
		config.Read(wxT("/RuleList/Columns/Sort/Direction/Name"), &flag);
		m_columnSortDirection.set(COLUMN_NAME, flag);
		flag = false;
		config.Read(wxT("/RuleList/Columns/Sort/Direction/Inputs"), &flag);
		m_columnSortDirection.set(COLUMN_INPUTS, flag);
		flag = false;
		config.Read(wxT("/RuleList/Columns/Sort/Direction/Destinations"), &flag);
		m_columnSortDirection.set(COLUMN_DESTINATIONS, flag);
		flag = false;
		config.Read(wxT("/RuleList/Columns/Sort/Direction/State"), &flag);
		m_columnSortDirection.set(COLUMN_STATE, flag);
		const long tmpSortBy = config.Read(
			wxT("/RuleList/Columns/Sort/SortedBy"), m_lastSortedBy);
		if (tmpSortBy <= COLUMNS_NUMB) {
			m_lastSortedBy = static_cast<Column>(tmpSortBy);
		}

		wxListItem itemCol;
		itemCol.SetAlign(wxLIST_FORMAT_LEFT);

		itemCol.SetText(wxT("Name"));
		m_myInterface.InsertColumn(COLUMN_NAME, itemCol);

		itemCol.SetText(wxT("Inputs"));
		m_myInterface.InsertColumn(COLUMN_INPUTS, itemCol);

		itemCol.SetText(wxT("Destinations"));
		m_myInterface.InsertColumn(COLUMN_DESTINATIONS, itemCol);

		itemCol.SetText(wxT("State"));
		m_myInterface.InsertColumn(COLUMN_STATE, itemCol);

		BOOST_ASSERT(m_controlsMap.find(m_controlIndex) == m_controlsMap.end());
		m_controlsMap.insert(std::make_pair(m_controlIndex, this));

	}

	~Implementation() {
		
		try {
			Config &config = wxGetApp().GetConfig();
			if (m_columnHasBeenResized[COLUMN_NAME]) {
				config.Write(wxT("/RuleList/Columns/Width/Name"), m_myInterface.GetColumnWidth(COLUMN_NAME));
			}
			if (m_columnHasBeenResized[COLUMN_INPUTS]) {
				config.Write(wxT("/RuleList/Columns/Width/Inputs"), m_myInterface.GetColumnWidth(COLUMN_INPUTS));
			}
			if (m_columnHasBeenResized[COLUMN_DESTINATIONS]) {
				config.Write(wxT("/RuleList/Columns/Width/Destinations"), m_myInterface.GetColumnWidth(COLUMN_DESTINATIONS));
			}
			if (m_columnHasBeenResized[COLUMN_DESTINATIONS]) {
				config.Write(wxT("/RuleList/Columns/Width/State"), m_myInterface.GetColumnWidth(COLUMN_STATE));
			}
			config.Write(wxT("/RuleList/Columns/Sort/Direction/Name"), m_columnSortDirection[COLUMN_NAME]);
			config.Write(wxT("/RuleList/Columns/Sort/Direction/Inputs"), m_columnSortDirection[COLUMN_INPUTS]);
			config.Write(wxT("/RuleList/Columns/Sort/Direction/Destinations"), m_columnSortDirection[COLUMN_DESTINATIONS]);
			config.Write(wxT("/RuleList/Columns/Sort/Direction/State"), m_columnSortDirection[COLUMN_STATE]);
			if (m_lastSortedBy != COLUMNS_NUMB) {
				config.Write(wxT("/RuleList/Columns/Sort/SortedBy"), m_lastSortedBy);
			}
		} catch (...) {
			//...//
		}

		BOOST_ASSERT(m_controlsMap.find(m_controlIndex) != m_controlsMap.end());
		const ControlsMap::iterator pos = m_controlsMap.find(m_controlIndex);
		if (pos != m_controlsMap.end()) {
			m_controlsMap.erase(pos);
		}
	}

private:

	//! Used for sorting.
	/** Can't get the object pointer in sort-function as
		pointer size on 64-bit OS less then size of long). */
	static ControlIndex m_nextControlIndex;

	//! Used for sorting.
	/** Can't get the object pointer in sort-function as
		pointer size on 64-bit OS less then size of long). */
	static ControlsMap m_controlsMap;

	//! Used for sorting.
	/** Can't get the object pointer in sort-function as
		pointer size on 64-bit OS less then size of long). */
	const ControlIndex m_controlIndex;

	RuleListCtrl &m_myInterface;

	std::bitset<COLUMNS_NUMB> m_columnSortDirection;
	Column m_lastSortedBy;
	std::bitset<COLUMNS_NUMB> m_columnHasBeenResized;

	const RulesMap& m_rules;
	const NotAppliedRulesUuids &m_notAppliedRules;
	typedef std::vector<RulesMap::const_iterator> RulesBind;
	RulesBind m_rulesBind;

	bool m_fistTimeRulesSet;

private:

	std::wstring SearchNetworkAdapter(const std::wstring &wId) const {
		if (wId == L"all") {
			return L"*";
		} else if (wId == L"loopback") {
			return L"127.0.0.1";
		}
		std::wstring result;
		const std::string id = wxString(wId).ToAscii();
		std::list<texs__NetworkAdapterInfo> serviceNetworkAdapters;
		boost::polymorphic_downcast<ServiceWindow *>(m_myInterface.GetParent())
			->GetService()
			.GetNetworkAdapters(false, serviceNetworkAdapters);
		foreach (const texs__NetworkAdapterInfo &info, serviceNetworkAdapters) {
			if (info.id == id) {
				BOOST_ASSERT(!info.name.empty());
				result = wxString::FromAscii(info.ipAddress.c_str()).c_str();
			}
		}
		if (result.empty()) {
			result = L"<unknown>";
		}
		return result;
	}

public:

	void RefreshList(bool isServerStarted) {
	
		RulesUuids selectedUuids;
		GetSelected(selectedUuids);

		m_myInterface.DeleteAllItems();
		m_rulesBind.resize(0);
		m_rulesBind.reserve(m_rules.size());

		const RulesMap::const_iterator end = m_rules.end();
		for (RulesMap::const_iterator i = m_rules.begin(); i != end; ++i) {

			const Rule &rule = *i->second;
			m_rulesBind.push_back(i);
			const wxChar *stateName;
			
			wxListItem itemInfo;
			itemInfo.SetImage(-1);
			itemInfo.SetAlign(wxLIST_FORMAT_LEFT);
			itemInfo.SetId(m_myInterface.GetItemCount());
			itemInfo.SetText(rule.GetName().GetCStr());
			const Theme &theme = wxGetApp().GetTheme();
			itemInfo.SetBackgroundColour(
				m_myInterface.GetItemCount() % 2
					?	theme.GetNotEvenLineColor()
					:	theme.GetEvenLineColor());
			const NotAppliedRulesUuids::const_iterator notAppliedPos
				= m_notAppliedRules.find(rule.GetUuid().GetCStr());
			if (notAppliedPos != m_notAppliedRules.end()) {
				switch (notAppliedPos->second) {
					case NARS_ADDED:
						itemInfo.SetImage(5);
						stateName = wxT("added");
						break;
					case NARS_DELETED:
						itemInfo.SetImage(6);
						itemInfo.SetTextColour(*wxLIGHT_GREY);
						stateName = wxT("deleted");
						break;
					default:
						BOOST_ASSERT(false);
					case NARS_MODIFIED:
						itemInfo.SetImage(3);
						stateName = wxT("modified");
						break;		
				}
			} else  if (isServerStarted) {
				if (rule.IsEnabled()) {
					itemInfo.SetImage(2);
					stateName = wxT("online");
				} else {
					itemInfo.SetImage(4);
					stateName = wxT("disabled");
				}
			} else {
				itemInfo.SetImage(4);
				stateName = rule.IsEnabled() ? wxT("stopped") : wxT("disabled");
			}

			const long index = m_myInterface.InsertItem(itemInfo);
			BOOST_ASSERT(index >= 0);
			m_myInterface.SetItemData(index, long(m_rulesBind.size() - 1));

			struct EndpointsJoiner {
				static wxString Join(
							const RuleEndpointCollection &endpoints,
							bool isInput,
							boost::function<std::wstring(const std::wstring &)> adapterSearcher,
							const ServiceAdapter &service) {
					wxString result;
					const size_t size = endpoints.GetSize();
					for (size_t i = 0; i < size; ++i) {
						using Mods::Inet::InetEndpointAddress;
						using Mods::Serial::SerialEndpointAddress;
						using Mods::Upnp::UpnpEndpointAddress;
						const RuleEndpoint &ep = endpoints[i];
						if (ep.IsCombined()) {
							if (ep.CheckCombinedAddressType<InetEndpointAddress>()) {
								result
									+= ep.GetCombinedTypedAddress<InetEndpointAddress>()
										.GetHumanReadable(adapterSearcher);
							} else if (ep.CheckCombinedAddressType<SerialEndpointAddress>()) {
								result
									+= ep.GetCombinedTypedAddress<SerialEndpointAddress>()
										.GetHumanReadable();
							} else if (ep.CheckCombinedAddressType<UpnpEndpointAddress>()) {
								result
									+= ep.GetCombinedTypedAddress<UpnpEndpointAddress>()
										.GetHumanReadable(GetExternalIp(service));
							} else {
								result += ep.GetCombinedResourceIdentifier().GetCStr();
							}
						} else if (!isInput || ep.GetReadWriteAcceptor() == Endpoint::ACCEPTOR_WRITER) {
							if (ep.CheckWriteAddressType<InetEndpointAddress>()) {
								result
									+= ep.GetWriteTypedAddress<InetEndpointAddress>()
										.GetHumanReadable(adapterSearcher);
							} else if (ep.CheckWriteAddressType<SerialEndpointAddress>()) {
								result
									+= ep.GetWriteTypedAddress<SerialEndpointAddress>()
										.GetHumanReadable();
							} else if (ep.CheckWriteAddressType<UpnpEndpointAddress>()) {
								result
									+= ep.GetWriteTypedAddress<UpnpEndpointAddress>()
										.GetHumanReadable(GetExternalIp(service));
							} else {
								result += ep.GetWriteResourceIdentifier().GetCStr();
							}
						} else {
							if (ep.CheckReadAddressType<InetEndpointAddress>()) {
								result
									+= ep.GetReadTypedAddress<InetEndpointAddress>()
										.GetHumanReadable(adapterSearcher);
							} else if (ep.CheckReadAddressType<SerialEndpointAddress>()) {
								result
									+= ep.GetReadTypedAddress<SerialEndpointAddress>()
										.GetHumanReadable();
							} else if (ep.CheckReadAddressType<UpnpEndpointAddress>()) {
								result
									+= ep.GetReadTypedAddress<UpnpEndpointAddress>()
										.GetHumanReadable(GetExternalIp(service));
							} else {
								result += ep.GetReadResourceIdentifier().GetCStr();
							}
						}
						if (i + 1 >= size) {
							break;
						} else if (result.size() > 75) {
							result += wxT("...");
							break;
						} else {
							result += wxT(", ");
						}
					}
					return result;
				}
				static wxString Join(
							const ServiceRule::ServiceSet &services,
							const ServiceAdapter &service,
							const bool isInput) {
					wxString result;
					const size_t size = services.GetSize();
					for (size_t i = 0; i < size; ++i) {
						const ServiceRule::Service &ruleService = services[i];
						BOOST_ASSERT(ruleService.name == L"Upnpc");
						if (ruleService.name != L"Upnpc") {
							continue;
						}
						try {
							using Mods::Upnp::UpnpcService;
							result += isInput
								?	UpnpcService::GetHumanReadableExternalPort(
										ruleService.param,
										GetExternalIp(service))
								:	UpnpcService::GetHumanReadableDestination(
										ruleService.param);
						} catch (const ::TunnelEx::InvalidLinkException &) {
							result += ruleService.name.GetCStr();
						}
						if (i + 1 >= size) {
							break;
						} else if (result.size() > 75) {
							result += wxT("...");
							break;
						} else {
							result += wxT(", ");
						}
					}
					return result;
				}
				static std::wstring GetExternalIp(const ServiceAdapter &service)  {
					if (!service.GetCachedUpnpDeviceExternalIp()) {
						wxString local;
						wxString external;
						service.GetUpnpStatus(external, local);
					}
					BOOST_ASSERT(service.GetCachedUpnpDeviceExternalIp());
					return !service.GetCachedUpnpDeviceExternalIp()->IsEmpty()
						?	service.GetCachedUpnpDeviceExternalIp()->c_str()
						:	L"<unknown>";

				}
			};

			const ServiceAdapter &service =
				boost::polymorphic_downcast<ServiceWindow *>(m_myInterface.GetParent())
					->GetService();

			if (IsTunnel(rule)) {
				boost::function<std::wstring(const std::wstring &)> adapterSearcher
					= boost::bind(&Implementation::SearchNetworkAdapter, this, _1);
				m_myInterface.SetItem(
					index,
					1,
					EndpointsJoiner::Join(
						boost::polymorphic_downcast<const TunnelRule *>(&rule)->GetInputs(),
						true,
						adapterSearcher,
						service));
				m_myInterface.SetItem(
					index,
					2,
					EndpointsJoiner::Join(
						boost::polymorphic_downcast<const TunnelRule *>(&rule)->GetDestinations(),
						false,
						adapterSearcher,
						service));
			} else {
				BOOST_ASSERT(IsService(rule));
				m_myInterface.SetItem(
					index,
					1,
					EndpointsJoiner::Join(
						boost::polymorphic_downcast<const ServiceRule *>(&rule)->GetServices(),
						service,
						true));
				m_myInterface.SetItem(
					index,
					2,
					EndpointsJoiner::Join(
						boost::polymorphic_downcast<const ServiceRule *>(&rule)->GetServices(),
						service,
						false));
			}

			m_myInterface.SetItem(index, 3, stateName);

		}

		if (m_fistTimeRulesSet && m_myInterface.GetItemCount() > 0) {
			Config& config = wxGetApp().GetConfig();
			m_myInterface.SetColumnWidth(
				COLUMN_NAME,
				config.Read(wxT("/RuleList/Columns/Width/Name"), wxLIST_AUTOSIZE));
			m_myInterface.SetColumnWidth(
				COLUMN_INPUTS,
				config.Read(wxT("/RuleList/Columns/Width/Inputs"), wxLIST_AUTOSIZE));
			m_myInterface.SetColumnWidth(
				COLUMN_DESTINATIONS,
				config.Read(wxT("/RuleList/Columns/Width/Destinations"), wxLIST_AUTOSIZE));
			m_fistTimeRulesSet = false;
		}

		if (m_lastSortedBy != COLUMNS_NUMB) {
			Sort(m_lastSortedBy, false);
		}

		Select(selectedUuids);
		
	}

	void SortByName() {
		Sort(COLUMN_NAME, true);
	}

	void SortByInputs() {
		Sort(COLUMN_INPUTS, true);
	}

	void SortByDestinations() {
		Sort(COLUMN_DESTINATIONS, true);
	}

	void SortByState() {
		Sort(COLUMN_STATE, true);
	}

	void OnSort(wxListEvent &event) {
		Sort(static_cast<RuleListCtrl::Column>(event.GetColumn()), true);
	}

	void Sort(RuleListCtrl::Column columnNumb, bool changeDirection) {
		
		wxListCtrlCompare fnSortCallBack;
		
		switch (columnNumb) {
			case COLUMN_NAME:
				fnSortCallBack = &::SortByName;
				break;
			case COLUMN_INPUTS:
				fnSortCallBack = &::SortByInputs;
				break;
			case COLUMN_DESTINATIONS:
				fnSortCallBack = &::SortByDestintations;
				break;
			case COLUMN_STATE:
				fnSortCallBack = &::SortByState;
				break;
			default:
				BOOST_ASSERT(false);
				return;
		}

		if (changeDirection && columnNumb == m_lastSortedBy) {
			m_columnSortDirection[columnNumb].flip();
		}
		
		m_myInterface.SortItems(fnSortCallBack, m_controlIndex);
		m_lastSortedBy = columnNumb;
		
		wxListItem item;
		item.SetMask(wxLIST_MASK_IMAGE);
		for (int i = 0; i < COLUMNS_NUMB; ++i) {
			const int image = i == m_lastSortedBy
				?	m_columnSortDirection[columnNumb] ? 0 : 1
				:	-1;
			item.SetImage(image);
			m_myInterface.SetColumn(i, item);
		}
	
		item.SetMask(wxLIST_MASK_DATA);
		const Theme &theme = wxGetApp().GetTheme();
		const int itemCount = m_myInterface.GetItemCount();
		for (int i = 0; i < itemCount; ++i) {
			item.SetId(i);
			if (!m_myInterface.GetItem(item)) {
				BOOST_ASSERT(false);
				continue;
			}
			item.SetBackgroundColour(i % 2
				?	theme.GetNotEvenLineColor()
				:	theme.GetEvenLineColor());
			m_myInterface.SetItem(item);
		}

	}

	void OnColumnResized(wxListEvent &event) {
		m_columnHasBeenResized.set(event.GetColumn());
	}

	const Rule * GetFirstSelectedRule() const {
		long item = m_myInterface.GetNextItem(
			-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1) {
			return NULL;
		}
		const long data = m_myInterface.GetItemData(item);
		BOOST_ASSERT(data < long(m_rulesBind.size()));
		return m_rulesBind[data]->second;
	}

	void GetSelected(RulesUuids &ids) const {
		for (long item = -1; ; ) {
			item = m_myInterface.GetNextItem(
				item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
			if (item == -1) {
				break;
			}
			const long data = m_myInterface.GetItemData(item);
#			pragma warning(push)
#			pragma warning(disable: 4018)
			BOOST_ASSERT(data < m_rulesBind.size());
#			pragma warning(pop)
			ids.insert(m_rulesBind[data]->first);
		}
	}

	void Select(const RulesUuids &ids) {
		const RulesUuids::const_iterator idsEnd = ids.end();
		for (long item = -1; ; ) {
			item = m_myInterface.GetNextItem(item, wxLIST_NEXT_ALL);
			if (item == -1) {
				break;
			}
			const long data = m_myInterface.GetItemData(item);
#			pragma warning(push)
#			pragma warning(disable: 4018)
			BOOST_ASSERT(data < m_rulesBind.size());
#			pragma warning(pop)
			if (ids.find(m_rulesBind[data]->first) != idsEnd) {
				m_myInterface.SetItemState(
					item,
					wxLIST_STATE_SELECTED,
					wxLIST_STATE_SELECTED);
			}
		}
	}

	void OnItemActivated(wxListEvent &) {
		const NotAppliedRulesUuids::const_iterator pos
			= m_notAppliedRules.find(GetFirstSelectedRule()->GetUuid().GetCStr());
		if (pos == m_notAppliedRules.end() || pos->second != NARS_DELETED) {
			wxCommandEvent commandEvent(
				wxEVT_COMMAND_MENU_SELECTED,
				MainFrame::CMD_RULE_EDIT);
			wxPostEvent(wxGetApp().GetTopWindow(), commandEvent);
		}
	}

	void OnItemContextMenu(wxContextMenuEvent &) {

		wxMenu menu;
		wxBitmap icon;
		
		const ServiceWindow &serviceWindow
			= *boost::polymorphic_downcast<ServiceWindow *>(m_myInterface.GetParent());
		const size_t selected = serviceWindow.GetSelectedRulesCount();
		const bool isConnected = serviceWindow.GetService().IsConnected();
		const size_t editableSelected 
			= selected > 0 && serviceWindow.GetEditableSelectedRulesCount();
		const size_t modifedSelected
			= isConnected && selected > 0 && serviceWindow.GetModifiedSelectedRulesCount();

		wxMenuItem *item(
			new wxMenuItem(&menu, MainFrame::CMD_RULE_ADD_CUSTOM, wxT("&New...\tCtrl-N")));
		wxGetApp().GetTheme().GetAddRuleIcon(icon, false);
		item->SetBitmap(icon);
		menu.Append(item);
		item = new wxMenuItem(&menu, MainFrame::CMD_RULE_EDIT, wxT("&Edit...\tF4"));
		wxGetApp().GetTheme().GetEditRuleIcon(icon, false);
		item->SetBitmap(icon);
		menu.Append(item);
		menu.Enable(MainFrame::CMD_RULE_EDIT, selected == 1 && editableSelected > 0);

		menu.AppendSeparator();

		item = new wxMenuItem(&menu, MainFrame::CMD_RULE_CUT, wxT("Cu&t Rule(s)\tCtrl-X"));
		wxGetApp().GetTheme().GetCutIcon(icon, false);
		item->SetBitmap(icon);
		menu.Append(item);
		menu.Enable(MainFrame::CMD_RULE_CUT, editableSelected > 0);
		item = new wxMenuItem(&menu, MainFrame::CMD_RULE_COPY, wxT("&Copy Rule(s)\tCtrl-C"));
		wxGetApp().GetTheme().GetCopyIcon(icon, false);
		item->SetBitmap(icon);
		menu.Append(item);
		menu.Enable(MainFrame::CMD_RULE_COPY, selected > 0);
		item = new wxMenuItem(&menu, MainFrame::CMD_RULE_PASTE, wxT("&Pase Rule(s)\tCtrl-V"));
		wxGetApp().GetTheme().GetPasteIcon(icon, false);
		item->SetBitmap(icon);
		menu.Append(item);
		menu.Enable(
			MainFrame::CMD_RULE_PASTE,
			!boost::polymorphic_downcast<MainFrame *>(wxGetApp().GetTopWindow())
				->GetInternalClipboardContent()
				.IsEmpty());
		
		menu.AppendSeparator();

		item = new wxMenuItem(&menu, MainFrame::CMD_RULE_ENABLE, wxT("&Enable Rule(s)"));
		wxGetApp().GetTheme().GetGoRuleIcon(icon, false);
		item->SetBitmap(icon);
		menu.Append(item);
		menu.Enable(
			MainFrame::CMD_RULE_ENABLE,
			isConnected && serviceWindow.GetDisabledSelectedRulesCount() > 0);

		item = new wxMenuItem(&menu, MainFrame::CMD_RULE_DISABLE, wxT("&Disable Rule(s)"));
		wxGetApp().GetTheme().GetRuleIcon(icon, false);
		item->SetBitmap(icon);
		menu.Append(item);
		menu.Enable(
			MainFrame::CMD_RULE_DISABLE,
			isConnected && serviceWindow.GetEnabledSelectedRulesCount() > 0);

		menu.AppendSeparator();

		menu.Append(MainFrame::CMD_RULE_CHANGES_APPLY, wxT("&Apply changes"));
		menu.Append(MainFrame::CMD_RULE_CHANGES_CANCEL, wxT("&Cancel changes"));
		menu.Enable(MainFrame::CMD_RULE_CHANGES_APPLY, modifedSelected > 0);
		menu.Enable(MainFrame::CMD_RULE_CHANGES_CANCEL, modifedSelected > 0);

		menu.AppendSeparator();

		item = new wxMenuItem(&menu, MainFrame::CMD_RULE_DELETE, wxT("&Delete"));
		wxGetApp().GetTheme().GetRemoveRuleIcon(icon, false);
		item->SetBitmap(icon);
		menu.Append(item);
		menu.Enable(MainFrame::CMD_RULE_DELETE, editableSelected > 0);

		m_myInterface.PopupMenu(&menu, wxDefaultPosition);

	}

};

//////////////////////////////////////////////////////////////////////////

RuleListCtrl::Implementation::ControlsMap RuleListCtrl::Implementation::m_controlsMap;
RuleListCtrl::Implementation::ControlIndex RuleListCtrl::Implementation::m_nextControlIndex
	= 0;

//////////////////////////////////////////////////////////////////////////


RuleListCtrl::RuleListCtrl(
			const RulesMap &rules,
			const NotAppliedRulesUuids &notAppliedRulesIds,
			wxWindow *parent,
			wxWindowID id,
			const wxPoint &pos,
			const wxSize &size,
			long style,
			const wxValidator &validator,
			const wxString &name)
		: wxListCtrl(parent, id, pos, size, style, validator, name) {
	m_pimpl.reset(new Implementation(rules, notAppliedRulesIds, Implementation::m_nextControlIndex, *this));
	++Implementation::m_nextControlIndex;
}

RuleListCtrl::~RuleListCtrl() {
	m_pimpl.reset();
}

void RuleListCtrl::RefreshList(bool isServerStarted) {
	m_pimpl->RefreshList(isServerStarted);
}

void RuleListCtrl::SortByName() {
	m_pimpl->SortByName();
}

void RuleListCtrl::SortByInputs() {
	m_pimpl->SortByInputs();
}

void RuleListCtrl::SortByDestinations() {
	m_pimpl->SortByDestinations();
}

void RuleListCtrl::SortByState() {
	m_pimpl->SortByState();
}

void RuleListCtrl::OnSort(wxListEvent &event) {
	m_pimpl->OnSort(event);
}

const Rule * RuleListCtrl::GetFirstSelectedRule() const {
	return m_pimpl->GetFirstSelectedRule();
}

void RuleListCtrl::GetSelected(RulesUuids &ids) const {
	m_pimpl->GetSelected(ids);
}

void RuleListCtrl::OnColumnResized(wxListEvent &event) {
	m_pimpl->OnColumnResized(event);
}

void RuleListCtrl::OnItemActivated(wxListEvent &event) {
	m_pimpl->OnItemActivated(event);
}

bool RuleListCtrl::SetColumnWidth(int col, int width) {
	if (!wxListCtrl::SetColumnWidth(col, width)) {
		return false;
	} else if (width == wxLIST_AUTOSIZE) {
		wxListItem item;
		if (GetColumn(col, item) && item.GetWidth() >= (GetClientRect().GetWidth() * 0.8)) {
			item.SetMask(wxLIST_MASK_WIDTH);
			item.SetWidth(GetClientRect().GetWidth() * 0.35);
			SetColumn(col, item);
		}
	}
	return true;
}

void RuleListCtrl::OnItemContextMenu(wxContextMenuEvent &event) {
	m_pimpl->OnItemContextMenu(event);
}

void RuleListCtrl::SelectAll() {
	for (long item = -1; ; ) {
		item = GetNextItem(item, wxLIST_NEXT_ALL);
		if (item == -1) {
			break;
		}
		SetItemState(item, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
	}
}

void RuleListCtrl::Select(const RulesUuids &ids) {
	m_pimpl->Select(ids);
}

//////////////////////////////////////////////////////////////////////////


int SortByEndpoints(
			const Rule &rule1,
			const Rule &rule2,
			const bool isAsc,
			const bool isInput) {

	struct ToString {

		static void Get(
				const ServiceRule::ServiceSet &services,
				std::wstring &buffer) {
			const size_t size = services.GetSize();
			std::set<std::wstring> ris;
			ris.insert(wxT("A"));
			for (unsigned int i = 0; i < size; ++i) {
				ris.insert(services[i].name.GetCStr());
			}
			buffer = boost::join(ris, " ");
		}

		static void Get(
					const RuleEndpointCollection &endpoints,
					std::wstring &buffer,
					bool isInput) {
			const size_t size = endpoints.GetSize();
			std::set<std::wstring> ris;
			ris.insert(wxT("B"));
			boost::wregex exp(L"^[^/:]+://");
			for (unsigned int i = 0; i < size; ++i) {
				const RuleEndpoint &ep = endpoints[i];
				std::wstring ri = ep.IsCombined()
					?	ep.GetCombinedResourceIdentifier().GetCStr()
					:	!isInput || ep.GetReadWriteAcceptor() == Endpoint::ACCEPTOR_WRITER
						?	ep.GetWriteResourceIdentifier().GetCStr()
						:	ep.GetReadResourceIdentifier().GetCStr();
				erase_regex(ri, exp);
				ris.insert(ri);
			}
			buffer = boost::join(ris, " ");
		}

	};

	std::wstring buffer1;
	if (IsTunnel(rule1)) {
		ToString::Get(
			isInput
				?	boost::polymorphic_downcast<const TunnelRule *>(&rule1)->GetInputs()
				:	boost::polymorphic_downcast<const TunnelRule *>(&rule1)->GetDestinations(),
			buffer1,
			isInput);
	} else if (isInput) {
		BOOST_ASSERT(IsService(rule1));
		ToString::Get(
			boost::polymorphic_downcast<const ServiceRule *>(&rule1)->GetServices(),
			buffer1);
	}
	std::wstring buffer2;
	if (IsTunnel(rule2)) {
		ToString::Get(
			isInput
				?	boost::polymorphic_downcast<const TunnelRule *>(&rule2)->GetInputs()
				:	boost::polymorphic_downcast<const TunnelRule *>(&rule2)->GetDestinations(),
			buffer2,
			isInput);
	} else if (isInput) {
		BOOST_ASSERT(IsService(rule2));
		ToString::Get(
			boost::polymorphic_downcast<const ServiceRule *>(&rule2)->GetServices(),
			buffer2);
	}
	
	const int result = buffer1 < buffer2
		?	-1
		:	buffer1 > buffer2 ? 1 : 0;
	return result && !isAsc
		?	result < 0 ? 1 : -1
		:	result;

}


int wxCALLBACK SortByName(long item1, long item2, long sortData) {
	if (item1 == item2) {
		return 0;
	}
	const RuleListCtrl::Implementation::ControlsMap::const_iterator pos
		= RuleListCtrl::Implementation::m_controlsMap.find(sortData);
	BOOST_ASSERT(RuleListCtrl::Implementation::m_controlsMap.end() != pos);
	if (pos == RuleListCtrl::Implementation::m_controlsMap.end()) {
		return 0;
	}
	const RuleListCtrl::Implementation &ctrlImpl = *pos->second;
	BOOST_ASSERT(item1 < long(ctrlImpl.m_rulesBind.size()));
	BOOST_ASSERT(item2 < long(ctrlImpl.m_rulesBind.size()));
	const WString &name1 = ctrlImpl.m_rulesBind[item1]->second->GetName();
	const WString &name2 = ctrlImpl.m_rulesBind[item2]->second->GetName();
	const int result = name1 < name2
		?	-1
		:	name1 > name2 ? 1 : 0;
	return result && !ctrlImpl.m_columnSortDirection[RuleListCtrl::COLUMN_NAME]
	?	result < 0 ? 1 : -1
		:	result;
}

int wxCALLBACK SortByInputs(long item1, long item2, long sortData) {
	if (item1 == item2) {
		return 0;
	}
	const RuleListCtrl::Implementation::ControlsMap::const_iterator pos
		= RuleListCtrl::Implementation::m_controlsMap.find(sortData);
	BOOST_ASSERT(RuleListCtrl::Implementation::m_controlsMap.end() != pos);
	if (pos == RuleListCtrl::Implementation::m_controlsMap.end()) {
		return 0;
	}
	const RuleListCtrl::Implementation &ctrlImpl = *pos->second;
	BOOST_ASSERT(item1 < long(ctrlImpl.m_rulesBind.size()));
	BOOST_ASSERT(item2 < long(ctrlImpl.m_rulesBind.size()));
	return SortByEndpoints(
		*ctrlImpl.m_rulesBind[item1]->second,
		*ctrlImpl.m_rulesBind[item2]->second,
		ctrlImpl.m_columnSortDirection[RuleListCtrl::COLUMN_INPUTS],
		true);
}

int wxCALLBACK SortByDestintations(long item1, long item2, long sortData) {
	if (item1 == item2) {
		return 0;
	}
	const RuleListCtrl::Implementation::ControlsMap::const_iterator pos
		= RuleListCtrl::Implementation::m_controlsMap.find(sortData);
	BOOST_ASSERT(RuleListCtrl::Implementation::m_controlsMap.end() != pos);
	if (pos == RuleListCtrl::Implementation::m_controlsMap.end()) {
		return 0;
	}
	const RuleListCtrl::Implementation &ctrlImpl = *pos->second;
	BOOST_ASSERT(item1 < long(ctrlImpl.m_rulesBind.size()));
	BOOST_ASSERT(item2 < long(ctrlImpl.m_rulesBind.size()));
	return SortByEndpoints(
		*ctrlImpl.m_rulesBind[item1]->second,
		*ctrlImpl.m_rulesBind[item2]->second,
		ctrlImpl.m_columnSortDirection[RuleListCtrl::COLUMN_DESTINATIONS],
		false);
}

int wxCALLBACK SortByState(long item1, long item2, long sortData) {
	if (item1 == item2) {
		return 0;
	}
	const RuleListCtrl::Implementation::ControlsMap::const_iterator pos
		= RuleListCtrl::Implementation::m_controlsMap.find(sortData);
	BOOST_ASSERT(RuleListCtrl::Implementation::m_controlsMap.end() != pos);
	if (pos == RuleListCtrl::Implementation::m_controlsMap.end()) {
		return 0;
	}
	const RuleListCtrl::Implementation &ctrlImpl = *pos->second;
	BOOST_ASSERT(item1 < long(ctrlImpl.m_rulesBind.size()));
	BOOST_ASSERT(item2 < long(ctrlImpl.m_rulesBind.size()));
	const NotAppliedRulesUuids::const_iterator notAppliedRulesEnd
		= ctrlImpl.m_notAppliedRules.end();
	NotAppliedRulesUuids::const_iterator notAppliedPos
		= ctrlImpl.m_notAppliedRules.find(
			ctrlImpl.m_rulesBind[item1]->second->GetUuid().GetCStr());
	const int state1 = notAppliedPos != notAppliedRulesEnd
		?	notAppliedPos->second
		:	ctrlImpl.m_rulesBind[item1]->second->IsEnabled()
			?	-1
			:	-2;
	notAppliedPos
		= ctrlImpl.m_notAppliedRules.find(
			ctrlImpl.m_rulesBind[item2]->second->GetUuid().GetCStr());
	const int state2 = notAppliedPos != notAppliedRulesEnd
		?	notAppliedPos->second
		:	ctrlImpl.m_rulesBind[item2]->second->IsEnabled()
			?	-1
			:	-2;
	const char result = state1 < state2
		?	-1
		:	state1 > state2 ? 1 : 0;
	return result && !ctrlImpl.m_columnSortDirection[RuleListCtrl::COLUMN_STATE]
		?	result < 0 ? 1 : -1
		:	result;
}

//////////////////////////////////////////////////////////////////////////
