/**************************************************************************
 *   Created: 2010/05/23 5:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "ServiceRuleDlg.hpp"
#include "ServiceAdapter.hpp"
#include "Validators.hpp"
#include "Auto.hpp"
#include "LicenseRestrictionDlg.hpp"
#include "Application.hpp"
#include "RuleUtils.hpp"

#include "LicensePolicies.hpp"

#include "Modules/Upnp/UpnpcService.hpp"

using namespace TunnelEx;

//////////////////////////////////////////////////////////////////////////

ServiceRuleDlg::ServiceRuleDlg(
			const wxString &title,
			ServiceWindow &service,
			wxWindow *parent)
		: RuleDlg(title, service, parent) {
	SetRule(std::auto_ptr<Rule>(new ServiceRule));
}

ServiceRuleDlg::ServiceRuleDlg(
			const wxString &title,
			ServiceWindow &service,
			wxWindow *parent,
			const TunnelEx::Rule &rule)
		: RuleDlg(title, service, parent, rule) {
	//...//
}

ServiceRuleDlg::~ServiceRuleDlg() {
	//...//
}

bool ServiceRuleDlg::Save(Rule &ruleAbstract) const {
	ServiceRule &rule = *boost::polymorphic_downcast<ServiceRule *>(&ruleAbstract);
	const bool isChanged = rule.GetServices().GetSize() != 1;
	rule.GetServices().SetSize(1);
	return SaveService(rule.GetServices()[0]) || isChanged;
}

std::auto_ptr<ServiceRule> ServiceRuleDlg::EditRule(
			ServiceWindow &service,
			wxWindow &parent,
			const TunnelEx::ServiceRule &rule) {
	std::auto_ptr<ServiceRule> result;
	assert(
		rule.GetServices().GetSize() == 1 && rule.GetServices()[0].name == L"Upnpc");
	UpnpServiceRuleDlg dlg(service, &parent, rule);
	if (dlg.ShowModal() == wxID_OK) {
		result.reset(new ServiceRule(dlg.GetRule()));
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(UpnpServiceRuleDlg, ServiceRuleDlg)
	EVT_TEXT(UpnpServiceRuleDlg::CONTROL_ID_EXTERNAL_PORT, UpnpServiceRuleDlg::OnExternalPortChanged)
	EVT_TEXT(UpnpServiceRuleDlg::CONTROL_ID_DESTINATION_PORT, UpnpServiceRuleDlg::OnDestinationPortChanged)
END_EVENT_TABLE()

UpnpServiceRuleDlg::UpnpServiceRuleDlg(
			ServiceWindow &service,
			wxWindow *parent)
		: ServiceRuleDlg(
			wxT("New external port mapping rule (UPnP)"),
			service,
			parent),
		m_isDevExist(false) {
	//...//
}

UpnpServiceRuleDlg::UpnpServiceRuleDlg(
			ServiceWindow &service,
			wxWindow *parent,
			const TunnelEx::Rule &rule)
		: ServiceRuleDlg(
			wxT("Edit external port mapping rule (UPnP)"),
			service,
			parent,
			rule),
		m_isDevExist(false),
		m_isLicenseValid(false) {
	//...//
}

UpnpServiceRuleDlg::~UpnpServiceRuleDlg() {
	//...//
}

void UpnpServiceRuleDlg::Init() {

	m_isLicenseValid = false;
	m_proto = 0;
	m_externalPort = 0;
	m_destinationHost = 0;
	m_destinationPort = 0;
	m_forceRecriation = 0;

	CheckLicense();

	m_isDevExist = GetService().GetUpnpStatus(m_externalIp, m_localIp);

	m_sourceProto = wxT("TCP");
	m_sourceExternalPort;
	if (IsNewRule()) {
		m_sourceDestinationHost = m_localIp;
	}
	m_sourceDestinationPort;
	m_sourceIsForceMode = false;
	if (	GetRule().GetServices().GetSize() > 0
			&& GetRule().GetServices()[0].name == L"Upnpc") {
		using namespace Mods::Upnp;
		Client::Proto proto = Client::PROTO_TCP;
		unsigned short externalPort = 0;
		std::wstring destinationHost;
		unsigned short destinationPort = 0;
		bool isForce = false;
		bool isPersistent = false;
		try {
			UpnpcService::ParseParam(
				GetRule().GetServices()[0].param,
				proto,
				externalPort,
				destinationHost,
				destinationPort,
				isForce,
				isPersistent);
		} catch (const ::TunnelEx::InvalidLinkException &ex) {
			wxLogError(ex.GetWhat());
			wxLogError(wxT("Could not parse rule: the rule has invalid format."));
		}
		m_sourceProto = proto == Client::PROTO_UDP ? wxT("UDP") : wxT("TCP");
		m_sourceExternalPort = boost::lexical_cast<std::wstring>(externalPort);
		m_externalValidPort = m_sourceExternalPort;
		m_sourceDestinationHost = destinationHost;
		m_sourceDestinationPort = boost::lexical_cast<std::wstring>(destinationPort);
		m_destinationValidPort = m_sourceDestinationPort;
		m_sourceIsForceMode = isForce;
	}

	if (!m_isDevExist) {
		wxLogWarning(
			wxT("Could not find router with the UPnP support in the local network."));
	}

	ServiceRuleDlg::Init();
	
}

void UpnpServiceRuleDlg::CheckLicense() {
	m_isLicenseValid = true;
}

std::auto_ptr<wxSizer> UpnpServiceRuleDlg::CreateControlAdditionalOptions() {
	std::auto_ptr<wxBoxSizer> result(new wxBoxSizer(wxHORIZONTAL));
	m_forceRecriation = new wxCheckBox(
		this,
		wxID_ANY,
		wxT("Recreate if external port used by another application"),
		wxDefaultPosition,
		wxDefaultSize,
		wxCHK_2STATE);
	m_forceRecriation->SetValue(m_sourceIsForceMode);
	result->Add(m_forceRecriation);
	return result;
}

std::auto_ptr<wxSizer> UpnpServiceRuleDlg::CreateControlContent() {

	const Theme &theme = wxGetApp().GetTheme();
	
	std::auto_ptr<wxBoxSizer> topBox(new wxBoxSizer(wxVERTICAL));

	{

		std::auto_ptr<wxFlexGridSizer> groupBox(
			new wxFlexGridSizer(2, 2, theme.GetDlgBorder(), 0));
		groupBox->SetFlexibleDirection(wxHORIZONTAL);
		groupBox->AddGrowableCol(1, 1);

		groupBox->Add(
			new wxStaticText(this, wxID_ANY, wxT("Protocol:")),
			wxSizerFlags(0).Align(wxALIGN_RIGHT | wxALIGN_CENTRE_VERTICAL));

		wxArrayString protocols;
		protocols.Add(wxT("TCP"));
		protocols.Add(wxT("UDP"));
		m_proto = new wxChoice(
			this,
			wxID_ANY,
			wxDefaultPosition,
			wxDefaultSize,
			protocols,
			wxTE_NOHIDESEL | wxTE_PROCESS_ENTER);
		m_proto->SetStringSelection(m_sourceProto);
		m_proto->SetToolTip(
			wxT("Allows to specify which protocol the data packet should be using."));
		groupBox->Add(m_proto, wxSizerFlags(0).Center());

		groupBox->Add(
			new wxStaticText(this, wxID_ANY, wxT("IP address:")),
			wxSizerFlags(0).Align(wxALIGN_RIGHT | wxALIGN_CENTRE_VERTICAL));

		std::auto_ptr<wxBoxSizer> addressBox(new wxBoxSizer(wxHORIZONTAL));

		wxTextCtrl &ip = *new wxTextCtrl(
			this,
			wxID_ANY,
			m_externalIp,
			wxDefaultPosition,
			wxDefaultSize,
			wxTE_NOHIDESEL | wxTE_PROCESS_ENTER);
		ip.Enable(false);
		addressBox->Add(&ip, wxSizerFlags(1).Center());

		addressBox->AddSpacer(theme.GetDlgBorder());

		addressBox->Add(
			new wxStaticText(this, wxID_ANY, wxT("Port:")),
			wxSizerFlags(0).Center());

		m_externalPort = new wxTextCtrl(
			this,
			CONTROL_ID_EXTERNAL_PORT,
			m_sourceExternalPort,
			wxDefaultPosition,
			RuleUtils::GetPortFieldSize(),
			wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
			NetworPortValidator(false));
		addressBox->Add(m_externalPort, wxSizerFlags(0).Center());

		groupBox->Add(addressBox.get(), wxSizerFlags(1).Expand());
		addressBox.release();

		wxStaticBoxSizer &group = *new wxStaticBoxSizer(
			wxHORIZONTAL,
			this,
			wxT("External Port"));
		group.Add(groupBox.get(), theme.GetStaticBoxFlags());
		groupBox.release();

		topBox->Add(&group, wxSizerFlags(0).Expand());

	}

	{

		std::auto_ptr<wxBoxSizer> box(new wxBoxSizer(wxHORIZONTAL));

		box->Add(
			new wxStaticText(this, wxID_ANY, wxT("Host:")),
			wxSizerFlags(0).Center());

		m_destinationHost = new wxTextCtrl(
			this,
			wxID_ANY,
			m_sourceDestinationHost,
			wxDefaultPosition,
			wxDefaultSize,
			wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
			HostValidator(false));
		box->Add(m_destinationHost, wxSizerFlags(1).Center());
		
		box->AddSpacer(theme.GetDlgBorder());
		box->Add(
			new wxStaticText(this, wxID_ANY, wxT("Port:")),
			wxSizerFlags(0).Center());
		m_destinationPort = new wxTextCtrl(
			this,
			CONTROL_ID_DESTINATION_PORT,
			m_sourceDestinationPort,
			wxDefaultPosition,
			RuleUtils::GetPortFieldSize(),
			wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
			NetworPortValidator(false));
		box->Add(m_destinationPort, wxSizerFlags(0).Center());

		wxStaticBoxSizer &group = *new wxStaticBoxSizer(
			wxHORIZONTAL,
			this,
			wxT("Destination"));
		group.Add(box.get(), theme.GetStaticBoxFlags());
		box.release();
		
		topBox->Add(
			&group,
			wxSizerFlags(0).Expand().Border(wxTOP, theme.GetTopSizerBorder()));

	}

	return topBox;

}

bool UpnpServiceRuleDlg::IsLicenseValid() const {
	return m_isLicenseValid;
}

const wxChar * UpnpServiceRuleDlg::GetHelpPath(void) const {
	return wxT("dialogs/external-port-mapping-upnp");
}

bool UpnpServiceRuleDlg::SaveService(ServiceRule::Service &service) const {
	
	using namespace TunnelEx::Mods::Upnp;
	
	bool isChanged = false;
	
	if (service.uuid.IsEmpty()) {
		service.uuid = Helpers::Uuid().GetAsString().c_str();
		isChanged = true;
	}
	
	const wxChar *const serviceName = wxT("Upnpc");
	if (service.name != serviceName) {
		isChanged = true;
		service.name = serviceName;
	}

	const unsigned short externalPort
		= RuleUtils::ConvertPort(m_externalPort->GetValue());
	const unsigned short destinationPort
		= RuleUtils::ConvertPort(m_destinationPort->GetValue());

	WString serviceParam = UpnpcService::CreateParam(
		m_proto->GetSelection() == 0 ? Client::PROTO_TCP : Client::PROTO_UDP,
		externalPort,
		m_destinationHost->GetValue().c_str(),
		destinationPort,
		m_forceRecriation->GetValue(),
		true);
	if (service.param != serviceParam) {
		isChanged = true;
		serviceParam.Swap(service.param);
	}

	return isChanged;

}

void UpnpServiceRuleDlg::OnExternalPortChanged(wxCommandEvent &) {
	if (m_externalPort) {
		CheckPortValue(*m_externalPort, m_externalValidPort);
	}
}

void UpnpServiceRuleDlg::OnDestinationPortChanged(wxCommandEvent &) {
	if (m_destinationPort) {
		CheckPortValue(*m_destinationPort, m_destinationValidPort);
	}
}

void UpnpServiceRuleDlg::CheckPortValue(wxTextCtrl &ctrl, wxString &validPort) const {
	std::wstring val = ctrl.GetValue().c_str();
	boost::trim(val);
	if (val.empty()) {
		return;
	}
	if (!boost::regex_match(val, boost::wregex(L"\\d+"))) {
		const long pos = std::max(long(0), ctrl.GetInsertionPoint() - 1);
		ctrl.ChangeValue(validPort);
		ctrl.SetInsertionPoint(std::min(ctrl.GetLastPosition(), pos));
	} else {
		validPort = val;
	}
}
