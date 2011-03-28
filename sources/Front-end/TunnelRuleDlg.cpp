 /**************************************************************************
 *   Created: 2008/01/10 17:38
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: TunnelRuleDlg.cpp 1082 2010-12-02 07:57:11Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "TunnelRuleDlg.hpp"
#include "EndpointDlg.hpp"
#include "RuleUtils.hpp"
#include "LicenseRestrictionDlg.hpp"
#include "Application.hpp"
#include "LicensePolicies.hpp"

#include "Modules/Inet/InetEndpointAddress.hpp"
#include "Modules/Serial/SerialEndpointAddress.hpp"
#include "Modules/Upnp/UpnpEndpointAddress.hpp"

#include "Version/Version.h"

//////////////////////////////////////////////////////////////////////////

using namespace std;
using namespace boost;
using namespace TunnelEx;
using Mods::Inet::InetEndpointAddress;
using Mods::Serial::SerialEndpointAddress;
using Mods::Upnp::UpnpEndpointAddress;

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(TunnelRuleDlg, TunnelRuleDlg::Base)

	EVT_LISTBOX_DCLICK(TunnelRuleDlg::CONTROL_ID_INPUTS, TunnelRuleDlg::OnEditInput)
	EVT_LISTBOX_DCLICK(TunnelRuleDlg::CONTROL_ID_DESTINATIONS, TunnelRuleDlg::OnEditDestination)
	EVT_LISTBOX(TunnelRuleDlg::CONTROL_ID_INPUTS, TunnelRuleDlg::OnInputSelectionChange) 
	EVT_LISTBOX(TunnelRuleDlg::CONTROL_ID_DESTINATIONS, TunnelRuleDlg::OnDestinationSelectionChange) 

	EVT_BUTTON(TunnelRuleDlg::CONTROL_ID_ADD_INPUT, TunnelRuleDlg::OnAddInput)
	EVT_BUTTON(TunnelRuleDlg::CONTROL_ID_ADD_DESTINATION, TunnelRuleDlg::OnAddDestination)
	EVT_BUTTON(TunnelRuleDlg::CONTROL_ID_EDIT_INPUT, TunnelRuleDlg::OnEditInput)
	EVT_BUTTON(TunnelRuleDlg::CONTROL_ID_EDIT_DESTINATION, TunnelRuleDlg::OnEditDestination)
	EVT_BUTTON(TunnelRuleDlg::CONTROL_ID_REMOVE_SELECTED_INPUTS, TunnelRuleDlg::OnRemoveSelectedInputs)
	EVT_BUTTON(TunnelRuleDlg::CONTROL_ID_REMOVE_SELECTED_DESTINATIONS, TunnelRuleDlg::OnRemoveSelectedDestinations)
	EVT_BUTTON(TunnelRuleDlg::CONTROL_ID_MOVE_SELECTED_DESTINATIONS_UP, TunnelRuleDlg::OnMoveSelectedDestinationsUp)
	EVT_BUTTON(TunnelRuleDlg::CONTROL_ID_MOVE_SELECTED_DESTINATIONS_DOWN, TunnelRuleDlg::OnMoveSelectedDestinationsDown)

END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////

TunnelRuleDlg::TunnelRuleDlg(
			ServiceWindow &service,
			wxWindow *parent,
			bool isFtpTunnel)
		: Base(wxEmptyString, service,  parent),
		m_isLicenseValid(true),
		m_isFtpTunnel(isFtpTunnel),
		m_isUpnpDevRequested(false) {
	SetRule(auto_ptr<Rule>(new TunnelRule));
	CheckLicense();
}

TunnelRuleDlg::TunnelRuleDlg(
			ServiceWindow &service,
			wxWindow *parent,
			const TunnelRule &rule)
		: Base(wxEmptyString, service, parent, rule),
		m_isLicenseValid(true),
		m_isFtpTunnel(RuleUtils::IsFtpTunnelIsOnInRule(GetRule())),
		m_isUpnpDevRequested(false) {
	CheckLicense();
}

TunnelRuleDlg::~TunnelRuleDlg() {
	//...//
}

void TunnelRuleDlg::Init() {
	SetTitle(GetRuleTitle());
	Base::Init();
}

void TunnelRuleDlg::CheckLicense() {
	if (m_isFtpTunnel) {
		Licensing::FtpTunnelLicense *ftpTunnelLicense(
			new Licensing::FtpTunnelLicense(LicenseState(GetService())));
		if (!ftpTunnelLicense->IsFeatureAvailable(true)) {
			LicenseRestrictionDlg(GetServiceWindow(), this, *ftpTunnelLicense, false)
				.ShowModal();
			if (m_isLicenseValid && !wxGetApp().IsUnlimitedModeActive()) {
				m_isLicenseValid = false;
			}
		}
	}
}

void TunnelRuleDlg::OnEditInput(wxCommandEvent &) {
	EditSelectedInput();
}

void TunnelRuleDlg::OnEditDestination(wxCommandEvent &) {
	EditSelectedDestination();
}

void TunnelRuleDlg::OnInputSelectionChange(wxCommandEvent &event) {
	wxArrayInt selections;
	polymorphic_downcast<wxListBox *>(FindWindow(CONTROL_ID_INPUTS))
		->GetSelections(selections);
	EnableInputEndpointListButtons(event.IsSelection(), selections.size() == 1);
}

void TunnelRuleDlg::OnDestinationSelectionChange(wxCommandEvent &event) {
	wxArrayInt selections;
	polymorphic_downcast<wxListBox *>(FindWindow(CONTROL_ID_DESTINATIONS))
		->GetSelections(selections);
	const unsigned int allCount
		= polymorphic_downcast<wxListBox *>(FindWindow(CONTROL_ID_DESTINATIONS))
			->GetCount();
	EnableDestinationEndpointListButtons(
		event.IsSelection(),
		allCount > selections.size(),
		selections.size() == 1);
}

void TunnelRuleDlg::OnAddInput(wxCommandEvent &) {
	AddEndpoint(CONTROL_ID_INPUTS, GetRule().GetInputs());
}

void TunnelRuleDlg::OnAddDestination(wxCommandEvent &) {
	AddEndpoint(CONTROL_ID_DESTINATIONS, GetRule().GetDestinations());
}

void TunnelRuleDlg::OnRemoveSelectedInputs(wxCommandEvent &) {
	RemoveSelectedEndpoints(
		CONTROL_ID_INPUTS,
		GetRule().GetInputs(),
		bind(&TunnelRule::SetInputs, &GetRule(), _1),
		wxT("Do you realy want to remove selected input endpoints?"));
	wxArrayInt selections;
	polymorphic_downcast<wxListBox *>(FindWindow(CONTROL_ID_INPUTS))
		->GetSelections(selections);
	EnableInputEndpointListButtons(selections.size() > 0, selections.size() == 1);
}

void TunnelRuleDlg::OnRemoveSelectedDestinations(wxCommandEvent &) {
	RemoveSelectedEndpoints(
		CONTROL_ID_DESTINATIONS,
		GetRule().GetDestinations(),
		bind(&TunnelRule::SetDestinations, &GetRule(), _1),
		wxT("Do you realy want to remove selected destination endpoints?"));
	wxArrayInt selections;
	polymorphic_downcast<wxListBox *>(FindWindow(CONTROL_ID_INPUTS))
		->GetSelections(selections);
	const unsigned int allCount
		= polymorphic_downcast<wxListBox *>(FindWindow(CONTROL_ID_DESTINATIONS))
			->GetCount();
	EnableDestinationEndpointListButtons(
		selections.size() > 0,
		allCount > selections.size(),
		selections.size() == 1);
}

void TunnelRuleDlg::OnMoveSelectedDestinationsUp(wxCommandEvent &) {
	MoveSelectedEndpoints(
		CONTROL_ID_DESTINATIONS,
		true,
		GetRule().GetDestinations(),
		bind(&TunnelRule::SetDestinations, &GetRule(), _1));
}

void TunnelRuleDlg::OnMoveSelectedDestinationsDown(wxCommandEvent &) {
	MoveSelectedEndpoints(
		CONTROL_ID_DESTINATIONS,
		false,
		GetRule().GetDestinations(),
		bind(&TunnelRule::SetDestinations, &GetRule(), _1));
}


bool TunnelRuleDlg::IsLicenseValid() const {
	return m_isLicenseValid;
}

auto_ptr<wxSizer> TunnelRuleDlg::CreateControlAdditionalOptions() {
	wxCheckBox &ctrl = *new wxCheckBox(
		this,
		CONTROL_ID_SORT_DESTINATIONS_BY_PING,
		wxT("Sort destination list by ping time"),
		wxDefaultPosition,
		wxDefaultSize,
		wxCHK_2STATE);
	ctrl.Enable(m_isLicenseValid);
	ctrl.SetValue(RuleUtils::IsSortDestinationsByPingIsOnInRule(GetRule()));
	ctrl.SetToolTip(
		wxT("Periodically ping endpoints from the destination list ")
			wxT("and move items with the less round trip time to the top."));
	auto_ptr<wxBoxSizer> result(new wxBoxSizer(wxHORIZONTAL));
	result->Add(&ctrl, wxSizerFlags(0).Center());
	return result;
}

auto_ptr<wxSizer> TunnelRuleDlg::CreateControlRuleInfo() {
	if (!m_isFtpTunnel) {
		return auto_ptr<wxSizer>();
	}
	const wxSizerFlags flags = wxSizerFlags(0).Center();
	auto_ptr<wxBoxSizer> result(new wxBoxSizer(wxHORIZONTAL));
	result->Add(
		new wxStaticText(
			this,
			wxID_ANY,
			wxT("This is the FTP tunnel rule.")),
		flags);
	result->AddSpacer(wxGetApp().GetTheme().GetDlgBorder());
	result->Add(
		new wxHyperlinkCtrl(
			this,
			wxID_ANY,
			wxT("About FTP tunneling."),
			//! @todo: URL hardcode [2009/12/06 16:55]
			wxT("http://") TUNNELEX_DOMAIN_W wxT("/product/ftp-tunneling?about")),
		flags );
	return result;
}

wxArrayString TunnelRuleDlg::ConvertEndpointCollectionToStrings(
			const RuleEndpointCollection &endpoints,
			bool isInput)
		const {
	const size_t size = endpoints.GetSize();
	wxArrayString result;
	result.Alloc(size);
	for (unsigned int i = 0; i < size; ++i) {
		result.Add(ConvertEndpointToStrings(endpoints[i], isInput));
	}
	return result;
}

wxControl & TunnelRuleDlg::CreateControlEnpointList(
			const wxWindowID id,
			RuleEndpointCollection &collection,
			const wxString &validateError) {

	class Validator : public wxValidator {
	public:
		Validator(const wxString &errorText)
				: m_errorText(errorText) {
			//...//
		}
		virtual bool Validate(wxWindow *) {
			wxListBox *listBox = dynamic_cast<wxListBox *>(GetWindow());
			BOOST_ASSERT(listBox);
			const bool result = !(listBox && !listBox->GetCount());
			if (!result) {
				wxLogWarning(m_errorText);
			}
			return result;
		}
		virtual wxObject * Clone() const {
			return new Validator(m_errorText);
		}
		virtual bool TransferToWindow() {
			return true;
		}
	private:
		const wxString m_errorText;
	} validator(validateError);

	wxControl &ctrl = *new wxListBox(
		this,
		id,
		wxDefaultPosition,
		wxSize(300, -1),
		ConvertEndpointCollectionToStrings(collection, id == CONTROL_ID_INPUTS),
		wxLB_EXTENDED,
		validator);
	ctrl.Enable(IsLicenseValid());
	if (!m_isFtpTunnel) {
		ctrl.SetToolTip(
			TUNNELEX_NAME_W wxT(" will listen to all network endpoints from the input ")
				wxT("list and redirect all incoming connections to the first ")
				wxT("available endpoint from the destination list."));
	} else {
		ctrl.SetToolTip(
			TUNNELEX_NAME_W wxT(" will listen to all network endpoints from the input ")
				wxT("list and redirect all incoming connections to the first ")
				wxT("available FTP server from the FTP server list."));
	}

	return ctrl;

}

auto_ptr<wxSizer> TunnelRuleDlg::CreateControlContent() {

	const Theme &theme = wxGetApp().GetTheme();

	const wxSize listButtonSize(theme.GetIconButtonSize());
	const wxSizerFlags listButtonsFlags = wxSizerFlags(0).Bottom().Center();

	wxBitmap addButtonIcon;
	theme.GetAddItemButton(addButtonIcon);
	wxBitmap addButtonDisabledIcon;
	theme.GetAddItemButtonDisabled(addButtonDisabledIcon);
	wxBitmap editButtonIcon;
	theme.GetEditItemButton(editButtonIcon);
	wxBitmap editButtonDisabledIcon;
	theme.GetEditItemButtonDisabled(editButtonDisabledIcon);
	wxBitmap removeButtonIcon;
	theme.GetRemoveItemButton(removeButtonIcon);
	wxBitmap removeButtonDisabledIcon;
	theme.GetRemoveItemButtonDisabled(removeButtonDisabledIcon);

	// input list:
	
	auto_ptr<wxBoxSizer> listBox(new wxBoxSizer(wxHORIZONTAL));
	listBox->Add(
		&CreateControlEnpointList(
			CONTROL_ID_INPUTS,
			GetRule().GetInputs(),
			wxT("Please, provide one or more input endpoints.")),
		wxSizerFlags(1).Expand());
	
	auto_ptr<wxBoxSizer> listButtonsBox(new wxBoxSizer(wxVERTICAL));
	
	wxBitmapButton *button = new wxBitmapButton(
		this,
		CONTROL_ID_EDIT_INPUT,
		editButtonIcon,
		wxDefaultPosition,
		listButtonSize);
	button->SetBitmapDisabled(editButtonDisabledIcon);
	button->Enable(false);
	button->SetToolTip(wxT("Edit selected input endpoint."));
	listButtonsBox->Add(button, listButtonsFlags);

	listButtonsBox->AddSpacer(theme.GetDlgBorder() / 2);

	button = new wxBitmapButton(
		this,
		CONTROL_ID_ADD_INPUT,
		addButtonIcon,
		wxDefaultPosition,
		listButtonSize);
	button->SetBitmapDisabled(addButtonDisabledIcon);
	button->Enable(m_isLicenseValid);
	button->SetToolTip(wxT("Add new enpoint into the input list."));
	listButtonsBox->Add(button, listButtonsFlags);

	button = new wxBitmapButton(
		this,
		CONTROL_ID_REMOVE_SELECTED_INPUTS,
		removeButtonIcon,
		wxDefaultPosition,
		listButtonSize);
	button->SetBitmapDisabled(removeButtonDisabledIcon);
	button->Enable(false);
	button->SetToolTip(wxT("Remove selected enpoints from the input list."));
	listButtonsBox->Add(button, listButtonsFlags);

	listBox->AddSpacer(theme.GetDlgBorder());
	listBox->Add(listButtonsBox.get(), wxSizerFlags(0).Bottom());
	listButtonsBox.release();
	wxStaticBoxSizer &inputGroup
		= *new wxStaticBoxSizer(wxHORIZONTAL, this, wxT("Input list"));
	inputGroup.Add(listBox.get(), theme.GetStaticBoxFlags());
	listBox.release();

	// destination list:

	listBox.reset(new wxBoxSizer(wxHORIZONTAL));
	listBox->Add(
		&CreateControlEnpointList(
			CONTROL_ID_DESTINATIONS,
			GetRule().GetDestinations(),
			wxT("Please, provide one or more destination endpoints.")),
		wxSizerFlags(1).Expand());

	listButtonsBox.reset(new wxBoxSizer(wxVERTICAL));	

	// buttons for destination list:
	wxBitmap arrowButtonIcon;
	theme.GetArrowUpButton(arrowButtonIcon);
	button = new wxBitmapButton(
		this,
		CONTROL_ID_MOVE_SELECTED_DESTINATIONS_UP,
		arrowButtonIcon,
		wxDefaultPosition,
		listButtonSize);
	theme.GetArrowUpButtonDisabled(arrowButtonIcon);
	button->SetBitmapDisabled(arrowButtonIcon);
	button->Enable(false);
	button->SetToolTip(
		!m_isFtpTunnel
			?	wxT("Move selected endpoints up.")
			:	wxT("Move selected FTP servers up."));
	listButtonsBox->Add(button, listButtonsFlags);

	theme.GetArrowDownButton(arrowButtonIcon);
	button = new wxBitmapButton(
		this,
		CONTROL_ID_MOVE_SELECTED_DESTINATIONS_DOWN,
		arrowButtonIcon,
		wxDefaultPosition,
		listButtonSize);
	theme.GetArrowDownButtonDisabled(arrowButtonIcon);
	button->SetBitmapDisabled(arrowButtonIcon);
	button->Enable(false);
	button->SetToolTip(
		!m_isFtpTunnel
			?	wxT("Move selected endpoints down.")
			:	wxT("Move selected FTP servers down."));
	listButtonsBox->Add(button, listButtonsFlags);

	listButtonsBox->AddSpacer(theme.GetDlgBottomBorder() / 2);

	button = new wxBitmapButton(
		this,
		CONTROL_ID_EDIT_DESTINATION,
		editButtonIcon,
		wxDefaultPosition,
		listButtonSize);
	button->SetBitmapDisabled(editButtonDisabledIcon);
	button->Enable(false);
	button->SetToolTip(
		!m_isFtpTunnel
			?	wxT("Edit selected destintation endpoint.")
			:	wxT("Edit selected FTP server."));
	listButtonsBox->Add(button, listButtonsFlags);

	listButtonsBox->AddSpacer(theme.GetDlgBottomBorder() / 2);

	button = new wxBitmapButton(
		this,
		CONTROL_ID_ADD_DESTINATION,
		addButtonIcon,
		wxDefaultPosition,
		listButtonSize);
	button->SetBitmapDisabled(addButtonDisabledIcon);
	button->Enable(m_isLicenseValid);
	button->SetToolTip(
		!m_isFtpTunnel
			?	wxT("Add new enpoint into the destination list.")
			:	wxT("Add new FTP server into the list."));
	listButtonsBox->Add(button, listButtonsFlags);

	button = new wxBitmapButton(
		this,
		CONTROL_ID_REMOVE_SELECTED_DESTINATIONS,
		removeButtonIcon,
		wxDefaultPosition,
		listButtonSize);
	button->SetBitmapDisabled(removeButtonDisabledIcon);
	button->Enable(false);
	button->SetToolTip(
		!m_isFtpTunnel
			?	wxT("Remove selected enpoints from the destination list.")
			:	wxT("Remove selected FTP servers from the list."));
	listButtonsBox->Add(button, listButtonsFlags);

	listBox->AddSpacer(theme.GetDlgBorder());
	listBox->Add(listButtonsBox.get(), wxSizerFlags(0).Bottom());
	listButtonsBox.release();
	wxStaticBoxSizer &destGroup = *new wxStaticBoxSizer(
		wxHORIZONTAL,
		this,
		!m_isFtpTunnel ? wxT("Destination list") : wxT("FTP server list"));
	destGroup.Add(listBox.get(), theme.GetStaticBoxFlags());
	listBox.release();

	auto_ptr<wxSizer> topBox(new wxBoxSizer(wxHORIZONTAL));
	topBox->Add(&inputGroup, wxSizerFlags(1).Expand());
	topBox->AddSpacer(theme.GetDlgBorder());
	topBox->Add(&destGroup, wxSizerFlags(1).Expand());
	return topBox;

}

void TunnelRuleDlg::Cancel() {
	RuleUtils::CheckInactiveAdapterWarning(GetRule(), GetService());
	Base::Cancel();
}

wxString TunnelRuleDlg::GetRuleTitle() const {
	return IsNewRule()
		?	!m_isFtpTunnel
			?	wxT("New tunnel rule")
				:	wxT("New rule for FTP tunnel")
			:	!m_isFtpTunnel
				?	wxT("Edit tunnel rule")
				:	wxT("Edit FTP tunnel rule");
}

const wxChar * TunnelRuleDlg::GetHelpPath() const {
	return !m_isFtpTunnel
		?	wxT("dialogs/rule-advanced")
		:	wxT("dialogs/ftp-tunnel-rule");
}

wxString TunnelRuleDlg::ConvertEndpointToStrings(
			const RuleEndpoint &ep,
			bool isInput)
		const {
	wxString result;
	if (ep.IsCombined()) {
		if (ep.CheckCombinedAddressType<InetEndpointAddress>()) {
			result
				= ep.GetCombinedTypedAddress<InetEndpointAddress>()
					.GetHumanReadable(bind(&TunnelRuleDlg::SearchNetworkAdapter, this, _1));
		} else if (ep.CheckCombinedAddressType<SerialEndpointAddress>()) {
			result
				= ep.GetCombinedTypedAddress<SerialEndpointAddress>()
					.GetHumanReadable();
		} else if (ep.CheckCombinedAddressType<UpnpEndpointAddress>()) {
			result = ep
				.GetCombinedTypedAddress<UpnpEndpointAddress>()
				.GetHumanReadable(GetExternalIpAddress());
		} else {
			result = ep.GetCombinedResourceIdentifier().GetCStr();
		}
	} else {
		const wxString writeResourceIdentifier
			= ep.CheckWriteAddressType<InetEndpointAddress>()
				?	ep.GetWriteTypedAddress<InetEndpointAddress>()
						.GetHumanReadable(bind(&TunnelRuleDlg::SearchNetworkAdapter, this, _1))
				:	ep.CheckWriteAddressType<SerialEndpointAddress>()
					?	ep.GetWriteTypedAddress<SerialEndpointAddress>()
							.GetHumanReadable()
					:	ep.CheckWriteAddressType<UpnpEndpointAddress>()
						?	ep.GetWriteTypedAddress<UpnpEndpointAddress>()
								.GetHumanReadable(GetExternalIpAddress())
						:	ep.GetWriteResourceIdentifier().GetCStr();
		const wxString readResourceIdentifier
			= ep.CheckReadAddressType<InetEndpointAddress>()
				?	ep.GetReadTypedAddress<InetEndpointAddress>()
						.GetHumanReadable(bind(&TunnelRuleDlg::SearchNetworkAdapter, this, _1))
				:	ep.CheckReadAddressType<SerialEndpointAddress>()
					?	ep.GetReadTypedAddress<SerialEndpointAddress>()
							.GetHumanReadable()
					:	ep.CheckReadAddressType<UpnpEndpointAddress>()
						?	ep.GetReadTypedAddress<UpnpEndpointAddress>()
								.GetHumanReadable(GetExternalIpAddress())
						:	ep.GetReadResourceIdentifier().GetCStr();
		if (!isInput || ep.GetReadWriteAcceptor() == Endpoint::ACCEPTOR_WRITER) {
			result = writeResourceIdentifier;
			result += wxT(" (");
			result += readResourceIdentifier;
			result += wxT(")");
		} else {
			result = readResourceIdentifier;
			result += wxT(" (");
			result += writeResourceIdentifier;
			result += wxT(")");
		}
	}
	return result;
}

bool TunnelRuleDlg::Save(Rule &newAbstractRule) const {

	bool hasChanges = false;
	TunnelRule &newRule = *polymorphic_downcast<TunnelRule *>(&newAbstractRule);

	if (m_isFtpTunnel != RuleUtils::IsFtpTunnelIsOnInRule(newRule)) {
		RuleUtils::SaveFtpInRule(m_isFtpTunnel, newRule);
		hasChanges = true;
	}

	wxCheckBox &sortDestinationsByPingCtrl
		= *polymorphic_downcast<wxCheckBox *>(FindWindow(CONTROL_ID_SORT_DESTINATIONS_BY_PING));
	if (sortDestinationsByPingCtrl.GetValue() != RuleUtils::IsSortDestinationsByPingIsOnInRule(GetRule())) {
		TunnelRule::Filters& ruleFilters = newRule.GetFilters();
		if (sortDestinationsByPingCtrl.GetValue()) {
			ruleFilters.Append(L"DestinationsSorter/Ping");
		} else {
			for (unsigned int i = 0; i < ruleFilters.GetSize(); ) {
				if (ruleFilters[i] == L"DestinationsSorter/Ping") {
					ruleFilters.Remove(i);
				} else {
					++i;
				}
			}
		}
		hasChanges = true;
	}
	
	RuleUtils::CheckInactiveAdapterWarning(newRule, GetService());

	return hasChanges;

}

void TunnelRuleDlg::EnableInputEndpointListButtons(
			const bool isEnable,
			const bool isEditable) {
	FindWindow(CONTROL_ID_REMOVE_SELECTED_INPUTS)->Enable(isEnable);
	FindWindow(CONTROL_ID_EDIT_INPUT)->Enable(isEnable && isEditable);
}

void TunnelRuleDlg::EnableDestinationEndpointListButtons(
			const bool isEnable,
			const bool isMovable,
			const bool isEditable) {
	FindWindow(CONTROL_ID_REMOVE_SELECTED_DESTINATIONS)->Enable(isEnable);
	FindWindow(CONTROL_ID_MOVE_SELECTED_DESTINATIONS_UP)->Enable(isEnable && isMovable);
	FindWindow(CONTROL_ID_MOVE_SELECTED_DESTINATIONS_DOWN)->Enable(isEnable && isMovable);
	FindWindow(CONTROL_ID_EDIT_DESTINATION)->Enable(isEnable && isEditable);
}

void TunnelRuleDlg::EditSelectedInput() {
	EditSelectedEndpoint(CONTROL_ID_INPUTS, GetRule().GetInputs());
}

void TunnelRuleDlg::EditSelectedDestination() {
	EditSelectedEndpoint(CONTROL_ID_DESTINATIONS, GetRule().GetDestinations());
}


wstring TunnelRuleDlg::SearchNetworkAdapter(const wstring &wId) const {
	if (wId == L"all") {
		return L"*";
	} else if (wId == L"loopback") {
		return L"127.0.0.1";
	}
	const string id = wxString(wId).ToAscii();
	wstring result;
	list<texs__NetworkAdapterInfo> serviceNetworkAdapters;
	GetService().GetNetworkAdapters(false, serviceNetworkAdapters);
	foreach (const texs__NetworkAdapterInfo &info, serviceNetworkAdapters) {
		if (info.id == id) {
			BOOST_ASSERT(info.name.size() > 0);
			result = wxString::FromAscii(info.ipAddress.c_str()).c_str();
		}
	}
	if (result.empty()) {
		result = L"<unknown>";
	}
	return result;
}

void  TunnelRuleDlg::AddEndpoint(
			const wxWindowID ctrlId,
			RuleEndpointCollection &originalEndpoints) {
	const bool isInput = ctrlId == CONTROL_ID_INPUTS;
	EndpointDlg endpointDlg(GetServiceWindow(), this, isInput, m_isFtpTunnel);
	if (endpointDlg.ShowModal() == wxID_OK) {
		wxListBox &ctrl = *polymorphic_downcast<wxListBox *>(FindWindow(ctrlId));
		ctrl.Insert(
			ConvertEndpointToStrings(endpointDlg.GetEndpoint(), isInput),
			ctrl.GetCount());
		originalEndpoints.Append(endpointDlg.GetEndpoint());
		SetChanged();
	}
}

void TunnelRuleDlg::EditSelectedEndpoint(
			const wxWindowID ctrlId,
			RuleEndpointCollection &originalEndpoints) {

	wxListBox &ctrl = *polymorphic_downcast<wxListBox *>(FindWindow(ctrlId));

	wxArrayInt selections;
	ctrl.GetSelections(selections);
	if (selections.GetCount() != 1) {
		return;
	}

	const bool isInput = ctrlId == CONTROL_ID_INPUTS;
	EndpointDlg endpointDlg(
		GetServiceWindow(),
		this,
		originalEndpoints[selections[0]],
		isInput,
		m_isFtpTunnel);
	if (endpointDlg.ShowModal() == wxID_OK) {
		ctrl.SetString(
			selections[0],
			ConvertEndpointToStrings(endpointDlg.GetEndpoint(), isInput));
		originalEndpoints[selections[0]] = endpointDlg.GetEndpoint();
		SetChanged();
	}

}

void TunnelRuleDlg::RemoveSelectedEndpoints(
			const wxWindowID ctrlId,
			const RuleEndpointCollection &originalEndpoints,
			function<void(const RuleEndpointCollection &)> saver,
			const wxChar *confirmationText) {
	const int answer = wxMessageBox(
		confirmationText,
		wxT("Endpoints removing"),
		wxYES_NO | wxICON_QUESTION | wxNO_DEFAULT | wxCANCEL,
		this);
	if (answer != wxYES) {
		return;
	}		
	wxListBox &ctrl = *polymorphic_downcast<wxListBox *>(FindWindow(ctrlId));
	wxArrayInt selections;
	ctrl.GetSelections(selections);
	BOOST_ASSERT(selections.GetCount() > 0);
	RuleEndpointCollection newEndpoints(originalEndpoints);
	for (size_t i = selections.GetCount() - 1; ; --i) {
		newEndpoints.Remove(selections.Item(i));
		ctrl.Delete(selections.Item(i));
		if (!i) {
			break;
		}
	}
	if (newEndpoints.GetSize() != originalEndpoints.GetSize()) {
		saver(newEndpoints);
		SetChanged();
	}
}

void TunnelRuleDlg::MoveSelectedEndpoints(
			const wxWindowID ctrlId,
			bool up,
			const RuleEndpointCollection &originalEndpoints,
			function<void(const RuleEndpointCollection &)> saver) {

	wxListBox &ctrl = *polymorphic_downcast<wxListBox *>(FindWindow(ctrlId));
	wxArrayInt selections;
	ctrl.GetSelections(selections);
	const long long endpointsNumb = originalEndpoints.GetSize();

	typedef map<unsigned int, unsigned int> ChangesMap;
	ChangesMap selectedIndexes;
	for (size_t i = 0; i < selections.GetCount(); ++i) {
		if (	(up && selections.Item(i) <= 0)
				|| (!up && selections.Item(i) >= endpointsNumb - 1)) {
			return;
		}
		selectedIndexes.insert(
			make_pair(
			up ? selections.Item(i) - 1 : selections.Item(i) + 1,
			selections.Item(i)));
	}

	RuleEndpointCollection newEndpoints(originalEndpoints);
	const long long end = up ? endpointsNumb : -1;
	for (long long i = up ? 0 : endpointsNumb - 1; i != end; ) {
	   ChangesMap::const_iterator pos = selectedIndexes.find(i);
	   if (pos != selectedIndexes.end()) {
		   unsigned int newIndex = up ? i + 1 : i - 1;
		   for ( ; selectedIndexes.find(newIndex) != selectedIndexes.end(); ) {
			   up ? ++newIndex : --newIndex;
		   }
		   newEndpoints[newIndex] = newEndpoints[i];
		   ctrl.SetString(
			   newIndex,
			   ConvertEndpointToStrings(newEndpoints[newIndex], ctrlId == CONTROL_ID_INPUTS));
		   ctrl.SetSelection(newIndex, false);
		   do {
			   newEndpoints[i] = originalEndpoints[pos->second];
			   ctrl.SetString(
				   i,
				   ConvertEndpointToStrings(newEndpoints[i], ctrlId == CONTROL_ID_INPUTS));
			   ctrl.SetSelection(i, true);
			   pos = selectedIndexes.find(up ? ++i : --i);
		   } while (pos != selectedIndexes.end());
	   } else {
		   up ? ++i : --i;
	   }
	}

	saver(newEndpoints);
	SetChanged();

}

wstring TunnelRuleDlg::GetExternalIpAddress() const {
	if (!m_isUpnpDevRequested || !GetService().GetCachedUpnpDeviceExternalIp()) {
		wxString local;
		wxString external;
		GetService().GetUpnpStatus(external, local);
		const_cast<TunnelRuleDlg *>(this)->m_isUpnpDevRequested = true;
	}
	BOOST_ASSERT(GetService().GetCachedUpnpDeviceExternalIp());
	return !GetService().GetCachedUpnpDeviceExternalIp()->IsEmpty()
		?	GetService().GetCachedUpnpDeviceExternalIp()->c_str()
		:	L"<unknown>";

}
