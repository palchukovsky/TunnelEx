/**************************************************************************
 *   Created: 2010/09/18 17:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "TunnelRuleShortDlg.hpp"
#include "LicenseRestrictionDlg.hpp"
#include "LicensePolicies.hpp"
#include "RuleUtils.hpp"
#include "Validators.hpp"
#include "Application.hpp"

#include "Modules/Pathfinder/PathfinderEndpointAddress.hpp"
#include "Modules/Serial/SerialEndpointAddress.hpp"
#include "Modules/Upnp/UpnpEndpointAddress.hpp"

using namespace TunnelEx;
using namespace TunnelEx::Mods::Inet;
using TunnelEx::Mods::Pathfinder::PathfinderEndpointAddress;
using TunnelEx::Mods::Inet::TcpEndpointAddress;
using TunnelEx::Mods::Inet::UdpEndpointAddress;
using TunnelEx::Mods::Serial::SerialEndpointAddress;
using TunnelEx::Mods::Upnp::UpnpEndpointAddress;
using TunnelEx::Mods::Upnp::UpnpTcpEndpointAddress;
using TunnelEx::Mods::Upnp::UpnpUdpEndpointAddress;

struct TunnelRuleShortDlg::Licenses {

	explicit Licenses(ServiceAdapter &service)
			: proxy(LicenseState(service)),
			proxyCascade(LicenseState(service)),
			ftp(LicenseState(service)),
			ssl(LicenseState(service)) {
		//...//
	}

	Licensing::ProxyLicense proxy;
	Licensing::ProxyCascadeLicense proxyCascade;
	Licensing::FtpTunnelLicense ftp;
	Licensing::SslLicense ssl;

};

BEGIN_EVENT_TABLE(TunnelRuleShortDlg, TunnelRuleShortDlg::Base)
	EVT_BUTTON(		wxID_OK,		TunnelRuleShortDlg::OnOk)
	EVT_COMMAND(	wxID_ANY, wxEVT_COMMAND_TEXT_ENTER, TunnelRuleShortDlg::OnOk)
	EVT_CHOICE(		TunnelRuleShortDlg::CONTROL_ID_NETWORK_ADAPTER,	TunnelRuleShortDlg::OnNetworkAdapterChange)
	EVT_TEXT(		TunnelRuleShortDlg::CONTROL_ID_PORT_INPUT,			TunnelRuleShortDlg::OnInputPortChanged)
	EVT_TEXT(		TunnelRuleShortDlg::CONTROL_ID_PORT_DESTINATION,	TunnelRuleShortDlg::OnDestinationPortChanged)
	EVT_CHECKBOX(	TunnelRuleShortDlg::CONTROL_ID_PROXY_USE,	TunnelRuleShortDlg::OnProxyUseToggle)
	EVT_BUTTON(		TunnelRuleShortDlg::CONTROL_ID_PROXY_SETTINGS,		TunnelRuleShortDlg::OnProxySettings)
	EVT_CHECKBOX(	TunnelRuleShortDlg::CONTROL_ID_PATHFINDER_USE, TunnelRuleShortDlg::OnPathfinderUseToggle)
	EVT_BUTTON(		TunnelRuleShortDlg::CONTROL_ID_ADVANCED_MODE,		TunnelRuleShortDlg::OnAdvancedMode)
	EVT_BUTTON(		TunnelRuleShortDlg::CONTROL_ID_PREV_STEP,			TunnelRuleShortDlg::OnPrevStep)
	EVT_RADIOBUTTON(TunnelRuleShortDlg::CONTROL_ID_TYPE,				TunnelRuleShortDlg::OnTypeChange)
	EVT_CHECKBOX(	TunnelRuleShortDlg::CONTROL_ID_INPUT_SSL_USE,			TunnelRuleShortDlg::OnUseInputSslToggle)
	EVT_CHECKBOX(	TunnelRuleShortDlg::CONTROL_ID_DESTINATION_SSL_USE,		TunnelRuleShortDlg::OnUseDestintationSslToggle)
	EVT_BUTTON(		TunnelRuleShortDlg::CONTROL_ID_INPUT_SSL_SETTINGS,	TunnelRuleShortDlg::OnInputSslSettings)
	EVT_BUTTON(		TunnelRuleShortDlg::CONTROL_ID_DESTINATION_SSL_SETTINGS, TunnelRuleShortDlg::OnDestintationSslSettings)
END_EVENT_TABLE()

TunnelRuleShortDlg::TunnelRuleShortDlg(
			ServiceWindow &service,
			wxWindow *const parent)
		:  Base(wxT("New tunnel rule"), service, parent),
		m_isAdvancedMode(false),
		m_step(1),
		m_isLicenseValid(true),
		m_isUpnpDevChecked(false) {
	GetService().GetNetworkAdapters(true, m_serviceNetworkAdapters);
	SetRule(std::auto_ptr<Rule>(new TunnelRule));
}

TunnelRuleShortDlg::TunnelRuleShortDlg(
			ServiceWindow &service,
			wxWindow *const parent,
			const TunnelRule &rule)
		:  Base(
			!RuleUtils::IsFtpTunnelIsOnInRule(rule)
				?	wxT("Edit tunnel rule")
				:	wxT("Edit FTP tunnel rule"),
			service,
			parent,
			rule),
		m_isAdvancedMode(false),
		m_isLicenseValid(true),
		m_isUpnpDevChecked(false) {
	GetService().GetNetworkAdapters(true, m_serviceNetworkAdapters);
}

TunnelRuleShortDlg::~TunnelRuleShortDlg() {
	//...//
}

void TunnelRuleShortDlg::Init() {

	m_licenses.reset(new Licenses(GetService()));
	
	Base::Init();

	if (IsNewRule()) {
		wxSize maxPanel(0, 0);
		foreach (wxPanel *const panel, m_allPanels) {
			if (panel == m_ruleEnableBox) {
				continue;
			}
			maxPanel.Set(
				std::max(maxPanel.GetX(), panel->GetSize().GetX()),
				std::max(maxPanel.GetY(), panel->GetSize().GetY()));
		}
		foreach (wxPanel *const panel, m_allPanels) {
			if (panel != m_ruleEnableBox) {
				if (panel == m_typeBox) {
					const int border = wxGetApp().GetTheme().GetTopSizerBorder();
					const int heightCorrect
						=	- border
							- GetGeneralSettingsSize().GetHeight()
							- m_ruleEnableBox->GetSize().GetHeight();
					wxSize size(maxPanel);
					size.SetHeight(size.GetHeight() + heightCorrect);
					panel->SetMinSize(size);
				} else {
					panel->SetMinSize(maxPanel);
				}
			}
			panel->Hide();
		}
	} else {
		foreach (wxPanel *const panel, m_allPanels) {
			if (panel == m_ruleEnableBox) {
				continue;
			}
			panel->Hide();
		}
		ReadRule();
	}
	
	UpdateVisible();
	Center();

}

void TunnelRuleShortDlg::OnTypeChange(wxCommandEvent &) {
	UpdateVisible();
}

void TunnelRuleShortDlg::OnNetworkAdapterChange(wxCommandEvent &) {
	if (RuleUtils::IsUpnpAdapterSelected(*m_inputAdapter)) {
		if (!m_isUpnpDevChecked || !GetService().GetCachedUpnpDeviceExternalIp()) {
			wxString localIp;
			wxString externalIp;
			if (!GetService().GetUpnpStatus(localIp, externalIp)) {
				wxLogWarning(
					wxT("Could not find router with the UPnP support in the local network."));
			}
			m_isUpnpDevChecked = true;
		}
		wxString externalIp = *GetService().GetCachedUpnpDeviceExternalIp();
		if (!externalIp.IsEmpty()) {
			WFormat message(L"External IP address %1% will be used.");
			message % externalIp.c_str();
			wxMessageBox(
				wxString(message.str()),
				wxT("UPnP Router Port Mapping"),
				wxOK | wxICON_INFORMATION,
				this);
		}
	} 
}

void TunnelRuleShortDlg::OnPortChanged(wxTextCtrl &port, wxString &valid) const {
	std::wstring checkValue = port.GetValue().c_str();
	boost::trim(checkValue);
	if (checkValue.empty()) {
		return;
	}
	if (!boost::regex_match(checkValue, boost::wregex(L"\\d+"))) {
		const long pos = std::max(long(0), port.GetInsertionPoint() - 1);
		port.ChangeValue(valid);
		port.SetInsertionPoint(std::min(port.GetLastPosition(), pos));
	} else {
		valid = checkValue;
	}
}

void TunnelRuleShortDlg::OnInputPortChanged(wxCommandEvent &) {
	OnPortChanged(*m_inputPort, m_inputPortValid);
}

void TunnelRuleShortDlg::OnDestinationPortChanged(wxCommandEvent &) {
	OnPortChanged(*m_destinationPort, m_destinationPortValid);
}

void TunnelRuleShortDlg::OnProxyUseToggle(wxCommandEvent &) {
	if (	m_destinationProxyUse->GetValue()
			&& !m_licenses->proxy.IsFeatureAvailable(true)) {
		LicenseRestrictionDlg(GetServiceWindow(),this, m_licenses->proxy, true)
			.ShowModal();
	}
	if (	m_destinationProxyUse->GetValue()
			&& !m_licenses->proxy.IsFeatureValueAvailable(true)
			&& !wxGetApp().IsUnlimitedModeActive()) {
		m_destinationProxyUse->SetValue(false);
	}
	if (	m_destinationProxyUse->GetValue()
			&& !m_licenses->proxy.IsFeatureValueAvailable(true)
			&& !wxGetApp().IsUnlimitedModeActive()) {
		return;
	}
	if (	m_destinationProxyUse->GetValue()
			&& !m_destinationPathfinderUse->GetValue()) {
		RuleUtils::ShowWarnAboutFtpOverHttp(
			this,
			wxT("Proxy"),
			m_typeFtp->GetValue());
	}
	if (	m_destinationProxyUse->GetValue()
			&& m_proxyCascade.size() == 0) {
		m_destinationProxyUse->SetValue(
			RuleUtils::ShowProxySettingsDialog(
				GetServiceWindow(),
				this,
				m_licenses->proxy,
				m_licenses->proxyCascade,
				m_proxyCascade));
	}
	if (!m_destinationProxyUse->GetValue()) {
		m_proxyCascade.resize(0);
	}
	m_destinationProxySettings->Enable(m_destinationProxyUse->GetValue());
}

void TunnelRuleShortDlg::OnProxySettings(wxCommandEvent &) {
	RuleUtils::ShowProxySettingsDialog(
		GetServiceWindow(),
		this,
		m_licenses->proxy,
		m_licenses->proxyCascade,
		m_proxyCascade);
}

void TunnelRuleShortDlg::OnPathfinderUseToggle(wxCommandEvent &) {
	if (	m_destinationPathfinderUse->GetValue()
			&& !m_licenses->proxy.IsFeatureAvailable(true)) {
		LicenseRestrictionDlg(GetServiceWindow(), this, m_licenses->proxy, true)
			.ShowModal();
		if (!wxGetApp().IsUnlimitedModeActive()) {
			m_destinationPathfinderUse->SetValue(false);
			return;
		}
	}
	if (	m_destinationPathfinderUse->GetValue()
			&& !m_destinationProxyUse->GetValue()) {
		RuleUtils::ShowWarnAboutFtpOverHttp(
			this,
			wxT("Pathfinder"),
			m_typeFtp->GetValue());
	}
}

bool TunnelRuleShortDlg::IsLicenseValid() const {
	return m_isLicenseValid;
}

std::auto_ptr<wxSizer> TunnelRuleShortDlg::CreateControlContent() {

	const Theme &theme = wxGetApp().GetTheme();
	const wxSizerFlags center = wxSizerFlags(0).Center();

	std::auto_ptr<wxBoxSizer> topSizer(new wxBoxSizer(wxVERTICAL));

	m_typeBox = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0);
	m_ruleEnableBox = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0);
	m_tcpUdpFtpBox
		= new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0);
	m_sslInputSubPanel
		= new wxPanel(m_tcpUdpFtpBox, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0);
	m_sslDestinationSubPanel
		= new wxPanel(m_tcpUdpFtpBox, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0);
	m_proxySubPanel
		= new wxPanel(m_tcpUdpFtpBox, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0);
	m_pipeBox
		= new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0);
	m_serialBox
		= new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0);

	{

		std::auto_ptr<wxBoxSizer> typeTopBox(new wxBoxSizer(wxHORIZONTAL));

		std::auto_ptr<wxBoxSizer> box(new wxBoxSizer(wxVERTICAL));
		const wxSizerFlags buttonFlags
			= wxSizerFlags(1).Expand().Border(wxALL, theme.GetDlgBorder());
		m_typeTcp = new wxRadioButton(
			m_typeBox,
			CONTROL_ID_TYPE,
			wxT("TCP"),
			wxDefaultPosition, 
			wxDefaultSize,
			wxRB_GROUP);
		box->Add(m_typeTcp, buttonFlags);
		m_typeUdp = new wxRadioButton(m_typeBox, CONTROL_ID_TYPE, wxT("UDP"));
		box->Add(m_typeUdp, buttonFlags);
		m_typeFtp = new wxRadioButton(m_typeBox, CONTROL_ID_TYPE, wxT("FTP"));
		box->Add(m_typeFtp, buttonFlags);
		m_typePipe = new wxRadioButton(m_typeBox, CONTROL_ID_TYPE, wxT("Named Pipe"));
		box->Add(m_typePipe, buttonFlags);
		m_typeSerial = new wxRadioButton(m_typeBox, CONTROL_ID_TYPE, wxT("Serial Line"));
		box->Add(m_typeSerial, buttonFlags);
		if (IsNewRule()) {
			const wxString templateType = wxGetApp().GetConfig().Read(
				wxT("/Rule/Template/Endpoint/Type/Input"));
			if (templateType.IsEmpty() || templateType == wxT("TCP")) {
				m_typeTcp->SetValue(true);
			} else if (templateType == wxT("UDP")) {
				m_typeUdp->SetValue(true);
			} else if (templateType == wxT("FTP")) {
				m_typeFtp->SetValue(true);
			} else if (templateType == wxT("pipe")) {
				m_typePipe->SetValue(true);
			} else if (templateType == wxT("serial")) {
				m_typeSerial->SetValue(true);
			} else {
				assert(false);
				m_typeTcp->SetValue(true);
			}
		} else {
			m_typeTcp->SetValue(true);
		}

		typeTopBox->Add(
			box.get(),
			wxSizerFlags(1).Expand().Border(wxLEFT | wxRIGHT, theme.GetDlgBorder() * 2));
		box.release();

		typeTopBox->Add(
			new wxStaticLine(m_typeBox, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVERTICAL),
			wxSizerFlags(0).Expand().Border(wxLEFT | wxRIGHT, theme.GetDlgBorder()));

		const int textWidth = 160;

		box.reset(new wxBoxSizer(wxVERTICAL));
		m_typeDescription = new wxStaticText(
			m_typeBox,
			wxID_ANY,
			wxEmptyString,
			wxDefaultPosition,
			wxSize(textWidth, 130));
		box->Add(m_typeDescription, wxSizerFlags(1).Expand());

		m_typeFtpLink = new wxHyperlinkCtrl(
			m_typeBox,
			wxID_ANY,
			wxT("More about FTP tunneling"),
			wxT("http://") TUNNELEX_DOMAIN_W wxT("/product/ftp-tunneling?about"),
			wxDefaultPosition,
			wxSize(textWidth, -1),
			 wxNO_BORDER | wxHL_CONTEXTMENU | wxHL_ALIGN_LEFT);
		box->Add(m_typeFtpLink);
		
		typeTopBox->Add(box.get(), wxSizerFlags(0).Expand());
		box.release();

		wxStaticBoxSizer &group
			= *new wxStaticBoxSizer(wxHORIZONTAL, m_typeBox, wxT("Type"));
		group.Add(typeTopBox.get(), theme.GetStaticBoxFlags());
		typeTopBox.release();

		m_typeBox->SetSizer(&group);

	}

	// input: /////////////////////////////////////////////////////////////

	// TCP/UDP/FTP: ///////////////////////////////////////////////////////
	{
		wxStaticBoxSizer &inputGroup
			= *new wxStaticBoxSizer(wxHORIZONTAL, m_tcpUdpFtpBox, wxT("Input"));
		{
			std::auto_ptr<wxBoxSizer> topBox(new wxBoxSizer(wxVERTICAL));
			wxStaticText &adapterLabel
				= *new wxStaticText(m_tcpUdpFtpBox, wxID_ANY, wxT("Adapter:"));
			{
				std::auto_ptr<wxBoxSizer> lineBox(new wxBoxSizer(wxHORIZONTAL));
				lineBox->Add(&adapterLabel, center);
				m_inputAdapter = &RuleUtils::CreateAdapterSelector(
					m_serviceNetworkAdapters,
					m_tcpUdpFtpBox,
					CONTROL_ID_NETWORK_ADAPTER);
				m_inputAdapter->SetSelection(0);
				lineBox->Add(m_inputAdapter, wxSizerFlags(center).Proportion(1));
				lineBox->Add(theme.GetDlgBorder(), 0);
				lineBox->Add(new wxStaticText(m_tcpUdpFtpBox, wxID_ANY, wxT("Port:")), center);
				m_inputPort = new wxTextCtrl(
					m_tcpUdpFtpBox,
					CONTROL_ID_PORT_INPUT,
					wxEmptyString,
					wxDefaultPosition,
					RuleUtils::GetPortFieldSize(),
					wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
					NetworPortValidator(true));
				m_inputPort->SetToolTip(wxT("Endpoint network port."));
				lineBox->Add(m_inputPort, center);
				topBox->Add(lineBox.get(), wxSizerFlags(0).Expand());
				lineBox.release();
			}
			{
				std::auto_ptr<wxBoxSizer> sslBox(new wxBoxSizer(wxVERTICAL));
				std::auto_ptr<wxBoxSizer> lineBox(new wxBoxSizer(wxHORIZONTAL));
				lineBox->Add(adapterLabel.GetSize().GetWidth(), 0);
				m_inputSslUse = new wxCheckBox(
					m_sslInputSubPanel,
					CONTROL_ID_INPUT_SSL_USE,
					wxT("Secure connection (SSL/TLS)"),
					wxDefaultPosition,
					wxDefaultSize,
					wxCHK_2STATE);
				lineBox->Add(m_inputSslUse, center);
				lineBox->Add(theme.GetDlgBorder(), 0);
				m_inputSslSettings = new wxButton(
					m_sslInputSubPanel,
					CONTROL_ID_INPUT_SSL_SETTINGS,
					wxT("Settings..."));
				m_inputSslSettings->Enable(false);
				m_inputSslSettings->SetToolTip(wxT("Secure connection settings."));
				lineBox->Add(theme.GetDlgBorder(), 0);
				lineBox->Add(m_inputSslSettings, center);
				lineBox->AddStretchSpacer(1);
				sslBox->Add(0, theme.GetDlgBorder());
				sslBox->Add(lineBox.get(), wxSizerFlags(0).Expand());
				lineBox.release();
				m_sslInputSubPanel->SetSizer(sslBox.get());
				sslBox.release();
			}
			topBox->Add(m_sslInputSubPanel, wxSizerFlags(0).Expand());
			inputGroup.Add(topBox.get(), theme.GetStaticBoxFlags());
			topBox.release();
		}
		wxStaticBoxSizer &destGroup
			= *new wxStaticBoxSizer(wxHORIZONTAL, m_tcpUdpFtpBox);
		{
			std::auto_ptr<wxBoxSizer> topBox(new wxBoxSizer(wxVERTICAL));
			wxStaticText &hostLabel
				= *new wxStaticText(m_tcpUdpFtpBox, wxID_ANY, wxT("Host:"));
			{
				std::auto_ptr<wxBoxSizer> lineBox(new wxBoxSizer(wxHORIZONTAL));
				lineBox->Add(&hostLabel, center);
				m_destinationHost = new wxTextCtrl(
					m_tcpUdpFtpBox,
					wxID_ANY,
					wxEmptyString,
					wxDefaultPosition,
					wxDefaultSize,
					wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
					HostValidator(true));
				m_destinationHost->SetToolTip(
					wxT("Hostname or IP address. It can be any local or remote hostname or IP address."));	
				lineBox->Add(m_destinationHost, wxSizerFlags(center).Proportion(1));
				lineBox->Add(theme.GetDlgBorder(), 0);
				lineBox->Add(new wxStaticText(m_tcpUdpFtpBox, wxID_ANY, wxT("Port:")), center);
				m_destinationPort = new wxTextCtrl(
					m_tcpUdpFtpBox,
					CONTROL_ID_PORT_DESTINATION,
					wxEmptyString,
					wxDefaultPosition,
					RuleUtils::GetPortFieldSize(),
					wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
					NetworPortValidator(true));
				m_destinationPort->SetToolTip(wxT("Endpoint network port."));
				lineBox->Add(m_destinationPort, center);
				topBox->Add(lineBox.get(), wxSizerFlags(0).Expand());
				lineBox.release();
			}
			m_destinationSslSettings = new wxButton(
				m_sslDestinationSubPanel,
				CONTROL_ID_DESTINATION_SSL_SETTINGS,
				wxT("Settings..."));
			{
				std::auto_ptr<wxBoxSizer> sslBox(new wxBoxSizer(wxVERTICAL));
				std::auto_ptr<wxBoxSizer> lineBox(new wxBoxSizer(wxHORIZONTAL));
				lineBox->Add(
					hostLabel.GetSize().GetWidth(),
					m_destinationSslSettings->GetSize().GetHeight());
				m_destinationSslUse = new wxCheckBox(
					m_sslDestinationSubPanel,
					CONTROL_ID_DESTINATION_SSL_USE,
					wxT("Secure connection (SSL/TLS)"),
					wxDefaultPosition,
					wxDefaultSize,
					wxCHK_2STATE);
				lineBox->Add(m_destinationSslUse, wxSizerFlags(center));
				lineBox->Add(theme.GetDlgBorder(), 0);
				m_destinationSslSettings->Enable(false);
				m_destinationSslSettings->SetToolTip(wxT("Secure connection settings."));
				lineBox->Add(theme.GetDlgBorder(), 0);
				lineBox->Add(m_destinationSslSettings, center);
				sslBox->Add(0, theme.GetDlgBorder());
				sslBox->Add(lineBox.get(), wxSizerFlags(0).Expand());
				lineBox.release();
				m_sslDestinationSubPanel->SetSizer(sslBox.get());
				sslBox.release();
			}
			topBox->Add(m_sslDestinationSubPanel, wxSizerFlags(0).Expand());
			{
				std::auto_ptr<wxBoxSizer> proxyBox(new wxBoxSizer(wxVERTICAL));
				wxHyperlinkCtrl *link;
				{
					std::auto_ptr<wxBoxSizer> lineBox(new wxBoxSizer(wxHORIZONTAL));
					lineBox->Add(
						hostLabel.GetSize().GetWidth(),
						m_destinationSslSettings->GetSize().GetHeight());
					m_destinationProxyUse = new wxCheckBox(
						m_proxySubPanel,
						CONTROL_ID_PROXY_USE,
						wxT("Use HTTP proxy server"),
						wxDefaultPosition,
						wxDefaultSize,
						wxCHK_2STATE);
					m_destinationProxyUse->SetToolTip(
						wxT("Make connection through HTTP proxy server.")
							wxT(" Endpoint, which use proxy server,")
							wxT(" could not be used for connections accepting."));
					lineBox->Add(m_destinationProxyUse, center);
					m_destinationProxySettings = new wxButton(
						m_proxySubPanel,
						CONTROL_ID_PROXY_SETTINGS,
						wxT("Settings..."));
					m_destinationProxySettings->Enable(false);
					m_destinationProxySettings->SetToolTip(wxT("Proxy server settings."));
					assert(
						m_destinationSslSettings->GetSize().GetHeight()
						== m_destinationProxySettings->GetSize().GetHeight());
					lineBox->Add(theme.GetDlgBorder(), 0);
					lineBox->Add(m_destinationProxySettings, center);
					lineBox->Add(theme.GetDlgBorder(), 0);
					link = new wxHyperlinkCtrl(
						m_proxySubPanel,
						wxID_ANY,
						wxT("About HTTP tunneling"),
						wxT("http://") TUNNELEX_DOMAIN_W wxT("/product/http-tunneling?about"));
					m_proxyPanelLinks.push_back(link);
					lineBox->Add(link, center);
					proxyBox->Add(0, theme.GetDlgBorder());
					proxyBox->Add(lineBox.get());
					lineBox.release();
				}
				{
					std::auto_ptr<wxBoxSizer> lineBox(new wxBoxSizer(wxHORIZONTAL));
					lineBox->Add(
						hostLabel.GetSize().GetWidth(),
						m_destinationSslSettings->GetSize().GetHeight());
					m_destinationPathfinderUse = new wxCheckBox(
						m_proxySubPanel,
						CONTROL_ID_PATHFINDER_USE,
						wxT("Try to find path with Pathfinder online service"),
						wxDefaultPosition,
						wxDefaultSize,
						wxCHK_2STATE);
					m_destinationPathfinderUse->SetToolTip(
						wxT("Pathfinder - a ") TUNNELEX_NAME_W wxT(" online service,")
							wxT(" which helps to find a path for connection to a server on the Internet.")
							wxT(" This is useful in cases where the security policy")
							wxT(" on the local network does not allow to connect")
							wxT(" to some Internet service or network port."));
					lineBox->Add(m_destinationPathfinderUse, center);
					lineBox->Add(0, theme.GetDlgBorder());
					link = new wxHyperlinkCtrl(
						m_proxySubPanel,
						wxID_ANY,
						wxT("About Pathfinder Online Service"),
						wxT("http://") TUNNELEX_DOMAIN_W wxT("/product/pathfinder?about"));
					m_proxyPanelLinks.push_back(link);
					lineBox->Add(link, center);
					proxyBox->Add(0, theme.GetDlgBorder());
					proxyBox->Add(lineBox.get(), wxSizerFlags(0).Expand());
					lineBox.release();
				}
				m_proxySubPanel->SetSizer(proxyBox.get());
				proxyBox.release();
			}
			topBox->Add(m_proxySubPanel, wxSizerFlags(0).Expand());
			m_destinationTcpUdpFtpBox = destGroup.GetStaticBox();
			destGroup.Add(topBox.get(), theme.GetStaticBoxFlags());
			topBox.release();
		}
		std::auto_ptr<wxBoxSizer> topBox(new wxBoxSizer(wxVERTICAL));
		topBox->Add(&inputGroup, wxSizerFlags(0).Expand());
		topBox->Add(
			&destGroup,
			wxSizerFlags(0).Expand().Border(wxTOP, theme.GetTopSizerBorder()));
		m_tcpUdpFtpBox->SetSizer(topBox.get());
		topBox.release();
	}
	
	// pipe: ///////////////////////////////////////////////////////
	{

		std::auto_ptr<wxBoxSizer> box(new wxBoxSizer(wxHORIZONTAL));
		box->Add(
			new wxStaticText(m_pipeBox, wxID_ANY, wxT("Pipe name:")),
			center);
		m_inputPipe = new wxTextCtrl(
			m_pipeBox,
			wxID_ANY,
			wxEmptyString,
			wxDefaultPosition,
			wxDefaultSize,
			wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
			PipeValidator(true));
		m_inputPipe->SetToolTip(
			wxT("Each named pipe has a unique name that distinguishes it from other named pipes in the system's list of named objects."));
		box->Add(m_inputPipe, wxSizerFlags(center).Proportion(1));
		wxStaticBoxSizer &inputGroup
			= *new wxStaticBoxSizer(wxHORIZONTAL, m_pipeBox, wxT("Input"));
		inputGroup.Add(box.get(), theme.GetStaticBoxFlags());
		box.release();
		
		box.reset(new wxBoxSizer(wxHORIZONTAL));
		box->Add(
			new wxStaticText(m_pipeBox, wxID_ANY, wxT("Pipe name:")),
			center);
		m_destinationPipe = new wxTextCtrl(
			m_pipeBox,
			wxID_ANY,
			wxEmptyString,
			wxDefaultPosition,
			wxDefaultSize,
			wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
			PipeValidator(true));
		m_destinationPipe->SetToolTip(
			wxT("Each named pipe has a unique name that distinguishes it from other named pipes in the system's list of named objects."));
		box->Add(m_destinationPipe, wxSizerFlags(center).Proportion(1));
		wxStaticBoxSizer &destGroup
			= *new wxStaticBoxSizer(wxHORIZONTAL, m_pipeBox, wxT("Destination"));
		destGroup.Add(box.get(), theme.GetStaticBoxFlags());
		box.release();

		box.reset(new wxBoxSizer(wxVERTICAL));
		box->Add(&inputGroup, wxSizerFlags(0).Expand());
		box->Add(
			&destGroup,
			wxSizerFlags(0).Expand().Border(wxTOP, theme.GetTopSizerBorder()));
		m_pipeBox->SetSizer(box.get());
		box.release();

	}
	
	// serial: ////////////////////////////////////////////////////////
	{

		const wxSize localFieldsSize(61, -1);
		std::auto_ptr<wxBoxSizer> groupBox(new wxBoxSizer(wxVERTICAL));

		std::auto_ptr<wxBoxSizer> box(new wxBoxSizer(wxHORIZONTAL));
		box->Add(
			new wxStaticText(m_serialBox, wxID_ANY, wxT("Serial line:")), center);
		m_inputSerialLine = new wxTextCtrl(
			m_serialBox,
			wxID_ANY,
			wxEmptyString,
			wxDefaultPosition,
			wxDefaultSize,
			wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
			NotEmptyValidator(wxT("serial line"), true));
		m_inputSerialLine->SetToolTip(wxT("Serial line to connect."));
		box->Add(m_inputSerialLine, wxSizerFlags(center).Proportion(1));
		groupBox->Add(box.get(), wxSizerFlags(0).Expand());
		box.release();

		wxStaticBoxSizer &confInputGroup = *new wxStaticBoxSizer(
			wxVERTICAL,
			m_serialBox,
			wxT("Serial line configuration"));
		box.reset(new wxBoxSizer(wxHORIZONTAL));

		box->Add(new wxStaticText(m_serialBox, wxID_ANY, wxT("Speed (baud):")), center);
		m_inputSerialBaudRate = new wxTextCtrl(
			m_serialBox,
			wxID_ANY,
			wxEmptyString,
			wxDefaultPosition,
			localFieldsSize,
			wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
			NumericValidator(wxT("speed (baud)"), true));
		box->Add(m_inputSerialBaudRate, center);

		box->AddSpacer(theme.GetDlgBorder());
		box->Add(new wxStaticText(m_serialBox, wxID_ANY, wxT("Data bits:")), center);
		m_inputSerialDataBits = new wxTextCtrl(
			m_serialBox,
			wxID_ANY,
			wxEmptyString,
			wxDefaultPosition,
			localFieldsSize,
			wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
			NumericValidator(wxT("data bits"), true));
		box->Add(m_inputSerialDataBits, center);

		box->AddSpacer(theme.GetDlgBorder());
		box->Add(new wxStaticText(m_serialBox, wxID_ANY, wxT("Stop bits:")), center);
		m_inputSerialStopBits = new wxTextCtrl(
			m_serialBox,
			wxID_ANY,
			wxEmptyString,
			wxDefaultPosition,
			localFieldsSize,
			wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
			NumericValidator(wxT("stop bits"), true));
		box->Add(m_inputSerialStopBits, center);

		confInputGroup.Add(box.get(), theme.GetStaticBoxFlags());
		box.release();

		box.reset(new wxBoxSizer(wxHORIZONTAL));
		box->Add(new wxStaticText(m_serialBox, wxID_ANY, wxT("Parity:")), center);
		m_inputSerialParity = &RuleUtils::CreateSerialConfParity(m_serialBox);
		box->Add(m_inputSerialParity, center);
		
		box->AddSpacer(theme.GetDlgBorder());
		box->Add(new wxStaticText(m_serialBox, wxID_ANY, wxT("Flow control:")), center);
		m_inputSerialFlowControl
			= &RuleUtils::CreateSerialConfFlowControl(m_serialBox);
		box->Add(m_inputSerialFlowControl, center);

		confInputGroup.Add(box.get(), theme.GetStaticBoxFlags());
		box.release();

		groupBox->AddSpacer(theme.GetDlgBorder());
		groupBox->Add(&confInputGroup, wxSizerFlags(0).Expand());
		wxStaticBoxSizer &inputGroup
			= *new wxStaticBoxSizer(wxHORIZONTAL, m_serialBox, wxT("Input"));
		inputGroup.Add(groupBox.get(), theme.GetStaticBoxFlags());
		groupBox.release();

		//////////////////////////////////////////////////////////////////////////

		groupBox.reset(new wxBoxSizer(wxVERTICAL));

		box.reset(new wxBoxSizer(wxHORIZONTAL));
		box->Add(
			new wxStaticText(m_serialBox, wxID_ANY, wxT("Serial line:")), center);
		m_destinationSerialLine = new wxTextCtrl(
			m_serialBox,
			wxID_ANY,
			wxEmptyString,
			wxDefaultPosition,
			wxDefaultSize,
			wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
			NotEmptyValidator(wxT("serial line"), true));
		m_destinationSerialLine->SetToolTip(wxT("Serial line to connect."));
		box->Add(m_destinationSerialLine, wxSizerFlags(center).Proportion(1));
		groupBox->Add(box.get(), wxSizerFlags(0).Expand());
		box.release();

		wxStaticBoxSizer &confDestGroup = *new wxStaticBoxSizer(
			wxVERTICAL,
			m_serialBox,
			wxT("Serial line configuration"));
		box.reset(new wxBoxSizer(wxHORIZONTAL));

		box->Add(new wxStaticText(m_serialBox, wxID_ANY, wxT("Speed (baud):")), center);
		m_destinationSerialBaudRate = new wxTextCtrl(
			m_serialBox,
			wxID_ANY,
			wxEmptyString,
			wxDefaultPosition,
			localFieldsSize,
			wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
			NumericValidator(wxT("speed (baud)"), true));
		box->Add(m_destinationSerialBaudRate, center);

		box->AddSpacer(theme.GetDlgBorder());
		box->Add(new wxStaticText(m_serialBox, wxID_ANY, wxT("Data bits:")), center);
		m_destinationSerialDataBits = new wxTextCtrl(
			m_serialBox,
			wxID_ANY,
			wxEmptyString,
			wxDefaultPosition,
			localFieldsSize,
			wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
			NumericValidator(wxT("data bits"), true));
		box->Add(m_destinationSerialDataBits, center);

		box->AddSpacer(theme.GetDlgBorder());
		box->Add(new wxStaticText(m_serialBox, wxID_ANY, wxT("Stop bits:")), center);
		m_destinationSerialStopBits = new wxTextCtrl(
			m_serialBox,
			wxID_ANY,
			wxEmptyString,
			wxDefaultPosition,
			localFieldsSize,
			wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
			NumericValidator(wxT("stop bits"), true));
		box->Add(m_destinationSerialStopBits, center);

		confDestGroup.Add(box.get(), theme.GetStaticBoxFlags());
		box.release();

		box.reset(new wxBoxSizer(wxHORIZONTAL));
		box->Add(new wxStaticText(m_serialBox, wxID_ANY, wxT("Parity:")), center);
		m_destinationSerialParity
			= &RuleUtils::CreateSerialConfParity(m_serialBox);
		box->Add(m_destinationSerialParity, center);
		
		box->AddSpacer(theme.GetDlgBorder());
		box->Add(new wxStaticText(m_serialBox, wxID_ANY, wxT("Flow control:")), center);
		m_destinationSerialFlowControl
			= &RuleUtils::CreateSerialConfFlowControl(m_serialBox);
		box->Add(m_destinationSerialFlowControl, center);

		confDestGroup.Add(box.get(), theme.GetStaticBoxFlags());
		box.release();

		groupBox->AddSpacer(theme.GetDlgBorder());
		groupBox->Add(&confDestGroup, wxSizerFlags(0).Expand());
		wxStaticBoxSizer &destGroup
			= *new wxStaticBoxSizer(wxHORIZONTAL, m_serialBox, wxT("Destination"));
		destGroup.Add(groupBox.get(), theme.GetStaticBoxFlags());
		groupBox.release();

		std::auto_ptr<wxBoxSizer> topBox(new wxBoxSizer(wxVERTICAL));
		topBox->Add(&inputGroup, wxSizerFlags(1).Expand());
		topBox->Add(
			&destGroup,
			wxSizerFlags(1).Expand().Border(wxTOP, theme.GetTopSizerBorder()));

		m_serialBox->SetSizer(topBox.get());
		topBox.release();

	}

	m_allPanels.push_back(m_typeBox);
	m_allPanels.push_back(m_tcpUdpFtpBox);
	m_allPanels.push_back(m_pipeBox);
	m_allPanels.push_back(m_serialBox);
	m_allPanels.push_back(m_ruleEnableBox);

	const wxSizerFlags flags = wxSizerFlags(0).Expand();
	foreach (wxPanel *const panel, m_allPanels) {
		topSizer->Add(panel, flags);
	}

	return topSizer;

}

const wxChar * TunnelRuleShortDlg::GetHelpPath() const {
	return wxT("rule");
}

bool TunnelRuleShortDlg::Save(TunnelEx::Rule &newAbstractRule) const {

	TunnelRule &newRule = *boost::polymorphic_downcast<TunnelRule *>(&newAbstractRule);
	
	bool hasChanges = false;

	if (newRule.GetInputs().GetSize() > 0) {
		if (newRule.GetInputs().GetSize() > 1) {
			while (newRule.GetInputs().GetSize() > 1) {
				newRule.GetInputs().Remove(newRule.GetInputs().GetSize() - 1);
			}
			hasChanges = true;
		}
		if (	!newRule.GetInputs()[0].IsCombined()
				|| !newRule.GetInputs()[0].IsCombinedAcceptor()) {
			newRule.GetInputs().Remove(0);
			hasChanges = true;
		}
	}
	if (newRule.GetDestinations().GetSize() > 0) {
		if (newRule.GetDestinations().GetSize() > 1) {
			hasChanges = true;
			while (newRule.GetDestinations().GetSize() > 1) {
				newRule.GetDestinations().Remove(
					newRule.GetDestinations().GetSize() - 1);
			}
		}
		if (	!newRule.GetDestinations()[0].IsCombined()
				|| newRule.GetDestinations()[0].IsCombinedAcceptor()) {
			newRule.GetDestinations().Remove(0);
			hasChanges = true;
		}
	}

	WString inputRi;
	WString destRi;

	if (m_typeTcp->GetValue() || m_typeUdp->GetValue() || m_typeFtp->GetValue()) {
		
		assert(m_inputSslUse->GetValue() == !m_inputCertificate.IsEmpty());
		assert(
			m_inputSslUse->GetValue()
			|| m_inputRemoteCertificates.GetSize() == 0);

		assert(m_destinationSslUse->GetValue() == !m_destinationCertificate.IsEmpty());
		assert(
			m_destinationSslUse->GetValue()
			|| m_destinationRemoteCertificates.GetSize() == 0);

		const unsigned short inputPort
			= RuleUtils::ConvertPort(m_inputPort->GetValue());
		const unsigned short destPort
			= RuleUtils::ConvertPort(m_destinationPort->GetValue());
		
		if (RuleUtils::IsUpnpAdapterSelected(*m_inputAdapter)) {
			inputRi = !m_typeUdp->GetValue()
				?	UpnpTcpEndpointAddress::CreateResourceIdentifier(
						inputPort,
						m_inputCertificate,
						m_inputRemoteCertificates)
				:	UpnpUdpEndpointAddress::CreateResourceIdentifier(
						inputPort,
						m_inputCertificate,
						m_inputRemoteCertificates);
		} else {
			std::list<texs__NetworkAdapterInfo>::const_iterator adapter
				= m_serviceNetworkAdapters.begin();
			advance(adapter, m_inputAdapter->GetCurrentSelection());
			const wxString adapterStr = wxString::FromAscii(adapter->id.c_str());
			inputRi = !m_typeUdp->GetValue()
				?	TcpEndpointAddress::CreateResourceIdentifier(
						adapterStr.c_str(),
						inputPort,
						L"*",
						m_inputCertificate,
						m_inputRemoteCertificates)
				:	UdpEndpointAddress::CreateResourceIdentifier(
						adapterStr.c_str(),
						inputPort,
						L"*");
		}

		if (m_typeUdp->GetValue()) {
			destRi = UdpEndpointAddress::CreateResourceIdentifier(
				m_destinationHost->GetValue().c_str(),
				destPort);
		} else {
			
			typedef boost::function<WString()> SimpleFabric;
			typedef boost::function<WString(const ProxyList &)> ProxyFabric;
			SimpleFabric simpleFabric;
			ProxyFabric proxyFabric;
			if (!m_destinationPathfinderUse->GetValue()) {
				simpleFabric = boost::bind(
					&TcpEndpointAddress::CreateResourceIdentifier,
					std::wstring(m_destinationHost->GetValue().c_str()),
					destPort,
					boost::cref(m_destinationCertificate),
					boost::cref(m_destinationRemoteCertificates));
				proxyFabric = boost::bind(
					static_cast<
							WString(*)(
								const std::wstring &,
								NetworkPort,
								const SslCertificateId &,
								const SslCertificateIdCollection &,
								const ProxyList &)>(
						&TcpEndpointAddress::CreateResourceIdentifier),
					std::wstring(m_destinationHost->GetValue().c_str()),
					destPort,
					boost::cref(m_destinationCertificate),
					boost::cref(m_destinationRemoteCertificates),
					_1);
			} else {
				simpleFabric = boost::bind(
					&PathfinderEndpointAddress::CreateResourceIdentifier,
					std::wstring(m_destinationHost->GetValue().c_str()),
					destPort,
					boost::cref(m_destinationCertificate),
					boost::cref(m_destinationRemoteCertificates));
				proxyFabric = boost::bind(
					&PathfinderEndpointAddress::CreateResourceIdentifier,
					std::wstring(m_destinationHost->GetValue().c_str()),
					destPort,
					boost::cref(m_destinationCertificate),
					boost::cref(m_destinationRemoteCertificates),
					_1);
			}
			if (m_destinationProxyUse->GetValue() && m_proxyCascade.size() > 0) {
				ProxyList proxyList;
				foreach (const ProxyDlg::Info &proxyInfo, m_proxyCascade) {
					Proxy proxy;
					proxy.host = proxyInfo.host.c_str();
					proxy.port = RuleUtils::ConvertPort(proxyInfo.port);
					if (proxyInfo.isAuthInUse) {
						proxy.user = proxyInfo.user.c_str();
						proxy.password = proxyInfo.password.c_str();
					}
					proxyList.push_back(proxy);
				}
				destRi = proxyFabric(proxyList);
			} else {
				destRi = simpleFabric();
			}

		}

	} else if (m_typePipe->GetValue()) {
		inputRi = L"pipe://";
		inputRi += m_inputPipe->GetValue().c_str();
		destRi = L"pipe://";
		destRi += m_destinationPipe->GetValue().c_str();
	} else if (m_typeSerial->GetValue()) {
		inputRi = RuleUtils::CreateSerialResourceIdentifier(
			m_inputSerialLine->GetValue(),
			m_inputSerialBaudRate->GetValue(),
			m_inputSerialDataBits->GetValue(),
			m_inputSerialStopBits->GetValue(),
			m_inputSerialParity->GetStringSelection(),
			m_inputSerialFlowControl->GetStringSelection());
		destRi = RuleUtils::CreateSerialResourceIdentifier(
			m_destinationSerialLine->GetValue(),
			m_destinationSerialBaudRate->GetValue(),
			m_destinationSerialDataBits->GetValue(),
			m_destinationSerialStopBits->GetValue(),
			m_destinationSerialParity->GetStringSelection(),
			m_destinationSerialFlowControl->GetStringSelection());
	}

	assert(!inputRi.IsEmpty());
	assert(!destRi.IsEmpty());
	if (newRule.GetInputs().GetSize() > 0) {
		if (	newRule.GetInputs()[0].GetCombinedAddress()->GetResourceIdentifier()
				!= inputRi) {
			newRule.GetInputs()[0].SetCombinedResourceIdentifier(inputRi, true);
			hasChanges = true;
		}
	} else {
		newRule.GetInputs().Append(RuleEndpoint(inputRi, true));
		hasChanges = true;
	}
	if (newRule.GetDestinations().GetSize() > 0) {
		if (	newRule.GetDestinations()[0].GetCombinedAddress()->GetResourceIdentifier()
				!= destRi) {
			newRule.GetDestinations()[0].SetCombinedResourceIdentifier(destRi, false);
			hasChanges = true;
		}
	} else {
		newRule.GetDestinations().Append(RuleEndpoint(destRi, false));
		hasChanges = true;
	}

	if (m_typeFtp->GetValue() != RuleUtils::IsFtpTunnelIsOnInRule(newRule)) {
		RuleUtils::SaveFtpInRule(m_typeFtp->GetValue(), newRule);
		hasChanges = true;
	}

	RuleUtils::CheckInactiveAdapterWarning(newRule, GetService());

	return hasChanges;

}

void TunnelRuleShortDlg::SaveTemplate() const {

	wxString type;
	if (m_typeTcp->GetValue()) {
		type = wxT("TCP");
	} else if (m_typeUdp->GetValue()) {
		type = wxT("UDP");
	} else if (m_typeFtp->GetValue()) {
		type = wxT("FTP");
	} else if (m_typePipe->GetValue()) {
		type = wxT("pipe");
	} else if (m_typeSerial->GetValue()) {
		type = wxT("serial");
	} else {
		assert(false);
		return;
	}

	Config &config = wxGetApp().GetConfig();
	config.Write(wxT("/Rule/Template/Endpoint/Type/Input"), type);

	Base::SaveTemplate();

}

bool TunnelRuleShortDlg::CheckRule(const TunnelRule &rule) {

	if (rule.GetInputs().GetSize() != 1) {
		return false;
	} else if (rule.GetDestinations().GetSize() != 1) {
		return false;
	} else if (!rule.GetInputs()[0].IsCombined()) {
		return false;
	} else if (!rule.GetDestinations()[0].IsCombined()) {
		return false;
	} else if (!rule.GetInputs()[0].IsCombinedAcceptor()) {
		return false;
	} else if (rule.GetDestinations()[0].IsCombinedAcceptor()) {
		return false;
	} else if (RuleUtils::IsSortDestinationsByPingIsOnInRule(rule)) {
		return false;
	} else if (TunnelRule().GetErrorsTreatment() != rule.GetErrorsTreatment()) {
		return false;
	} else if (!RuleUtils::GetEndpoindLogPath(rule.GetInputs()[0]).IsEmpty()) {
		return false;
	} else if (!RuleUtils::GetEndpoindLogPath(rule.GetDestinations()[0]).IsEmpty()) {
		return false;
	}

	std::wstring resourceIdentifier(
		rule.GetInputs()[0].GetCombinedResourceIdentifier().GetCStr());
	const boost::wregex exp(L"([^:/]+)://.+");
	boost::wsmatch what;
	std::wstring inputProto;
	assert(boost::regex_match(resourceIdentifier, what, exp));
	if (boost::regex_match(resourceIdentifier, what, exp)) {	
		inputProto = what[1].str();
		if (boost::iequals(inputProto, L"upnp_tcp")) {
			inputProto = TcpEndpointAddress::GetProto();
		} else if (boost::iequals(inputProto, L"upnp_udp")) {
			inputProto = UdpEndpointAddress::GetProto();
		}
	}
	resourceIdentifier
		= rule.GetDestinations()[0].GetCombinedResourceIdentifier().GetCStr();
	std::wstring destProto;
	assert(boost::regex_match(resourceIdentifier, what, exp));
	if (boost::regex_match(resourceIdentifier, what, exp)) {
		destProto = what[1].str();
		if (boost::iequals(destProto, L"pathfinder")) {
			destProto = TcpEndpointAddress::GetProto();
		}
	}
	return !inputProto.empty() && boost::iequals(inputProto, destProto);

}

void TunnelRuleShortDlg::ReadRule() {
	
	assert(!IsNewRule());
	assert(CheckRule(GetRule()));
	assert(GetRule().GetInputs().GetSize() == 1);
	assert(GetRule().GetDestinations().GetSize() == 1);

	if (RuleUtils::IsFtpTunnelIsOnInRule(GetRule())) {
		assert(
			GetRule().GetInputs().GetSize() == 1
			&&	GetRule().GetInputs()[0].IsCombined()
			&&	(GetRule().GetInputs()[0].CheckCombinedAddressType<TcpEndpointAddress>()
				|| GetRule().GetInputs()[0].CheckCombinedAddressType<UpnpEndpointAddress>()));
		assert(
			GetRule().GetDestinations().GetSize() == 1
			&&	GetRule().GetDestinations()[0].IsCombined()
			&&	(GetRule().GetDestinations()[0].CheckCombinedAddressType<TcpEndpointAddress>()
				|| GetRule().GetInputs()[0].CheckCombinedAddressType<PathfinderEndpointAddress>()));
		m_typeFtp->SetValue(true);
	}
	
	if (GetRule().GetInputs().GetSize() > 0) {
		assert(GetRule().GetInputs()[0].IsCombined());
		assert(GetRule().GetInputs()[0].IsCombinedAcceptor());
		if (GetRule().GetInputs()[0].IsCombined() && GetRule().GetInputs()[0].IsCombinedAcceptor()) {
			if (GetRule().GetInputs()[0].CheckCombinedAddressType<InetEndpointAddress>()) {
				if (GetRule().GetInputs()[0].CheckCombinedAddressType<TcpEndpointAddress>()) {
					if (!m_typeFtp->GetValue()) {
						m_typeTcp->SetValue(true);
					}
				} else if (
						GetRule().GetInputs()[0].CheckCombinedAddressType<UdpEndpointAddress>()
						|| GetRule().GetInputs()[0].CheckCombinedAddressType<UpnpUdpEndpointAddress>()) {
					m_typeUdp->SetValue(true);
				} else {
					assert(false);
				}
				const InetEndpointAddress &addr
					= GetRule().GetInputs()[0].GetCombinedTypedAddress<InetEndpointAddress>();
				RuleUtils::SelectAdapter(
					*m_inputAdapter,
					addr.GetAdapter(),
					m_serviceNetworkAdapters);
				m_inputPort->SetValue(boost::lexical_cast<std::wstring>(addr.GetPort()));
				m_inputPortValid = m_inputPort->GetValue();
				if (GetRule().GetInputs()[0].CheckCombinedAddressType<TcpEndpointAddress>()) {
					const TcpEndpointAddress &addr
						= GetRule().GetInputs()[0].GetCombinedTypedAddress<TcpEndpointAddress>();
					m_inputCertificate = addr.GetCertificate();
					if (!m_inputCertificate.IsEmpty()) {
						m_inputSslUse->SetValue(true);
						m_inputSslSettings->Enable(true);
						m_inputRemoteCertificates = addr.GetRemoteCertificates();
					}
				}
			} else if (GetRule().GetInputs()[0].CheckCombinedAddressType<UpnpEndpointAddress>()) {
				if (GetRule().GetInputs()[0].CheckCombinedAddressType<UpnpTcpEndpointAddress>()) {
					if (!m_typeFtp->GetValue()) {
						m_typeTcp->SetValue(true);
					}
				} else if (GetRule().GetInputs()[0].CheckCombinedAddressType<UpnpUdpEndpointAddress>()) {
					m_typeUdp->SetValue(true);
				} else {
					assert(false);
				}
				const UpnpEndpointAddress &addr
					= GetRule().GetInputs()[0].GetCombinedTypedAddress<UpnpEndpointAddress>();
				RuleUtils::SelectUpnpAdapter(*m_inputAdapter);
				m_inputPort->SetValue(boost::lexical_cast<std::wstring>(addr.GetExternalPort()));
				m_inputPortValid = m_inputPort->GetValue();
			} else if (GetRule().GetInputs()[0].CheckCombinedAddressType<SerialEndpointAddress>()) {
				m_typeSerial->SetValue(true);
				const SerialEndpointAddress &addr
					= GetRule().GetInputs()[0].GetCombinedTypedAddress<SerialEndpointAddress>();
				m_inputSerialLine->SetValue(addr.GetLine());
				m_inputSerialBaudRate->SetValue(boost::lexical_cast<std::wstring>(addr.GetBaudRate()));
				m_inputSerialDataBits->SetValue(boost::lexical_cast<std::wstring>(addr.GetDataBits()));
				m_inputSerialStopBits->SetValue(boost::lexical_cast<std::wstring>(addr.GetStopBits()));
				typedef std::map<wxString, SerialEndpointAddress::Parity> ParityMap;
				ParityMap parityMap;
				RuleUtils::GetSerialParityValsMap(parityMap);
				foreach (const ParityMap::value_type &val, parityMap) {
					if (val.second == addr.GetParity()) {
						m_inputSerialParity->SetStringSelection(val.first);
						break;
					}
				}
				typedef std::map<wxString, SerialEndpointAddress::FlowControl> FcMap;
				FcMap fcMap;
				RuleUtils::GetSerialFlowControlValsMap(fcMap);
				foreach (const FcMap::value_type &val, fcMap) {
					if (val.second == addr.GetFlowControl()) {
						m_inputSerialFlowControl->SetStringSelection(val.first);
						break;
					}
				}
			} else {
				const std::wstring resourceIdentifier(
					GetRule().GetInputs()[0].GetCombinedResourceIdentifier().GetCStr());
				const boost::wregex exp(L"([^:/]+)://(.+)");
				boost::wsmatch what;
				if (	boost::regex_match(resourceIdentifier, what, exp)
						&& !wxString(what[1].str()).CompareTo(wxT("pipe"), wxString::ignoreCase)) {
					m_typePipe->SetValue(true);
					m_inputPipe->SetValue(what[2].str());
				} else {
					assert(false);
				}
			} 
		}
	}

	if (GetRule().GetDestinations().GetSize() > 0) {
		assert(GetRule().GetDestinations()[0].IsCombined());
		assert(!GetRule().GetDestinations()[0].IsCombinedAcceptor());
		if (GetRule().GetDestinations()[0].IsCombined() && !GetRule().GetDestinations()[0].IsCombinedAcceptor()) {
			if (m_typeTcp->GetValue() || m_typeUdp->GetValue() || m_typeFtp->GetValue()) {
				assert(GetRule().GetDestinations()[0].CheckCombinedAddressType<InetEndpointAddress>());
				if (GetRule().GetDestinations()[0].CheckCombinedAddressType<InetEndpointAddress>()) {
					const InetEndpointAddress &addr
						= GetRule().GetDestinations()[0].GetCombinedTypedAddress<InetEndpointAddress>();
					m_destinationHost->SetValue(addr.GetHostName());
					m_destinationPort->SetValue(boost::lexical_cast<std::wstring>(addr.GetPort()));
					m_destinationPortValid = m_destinationPort->GetValue();
					if (GetRule().GetDestinations()[0].CheckCombinedAddressType<TcpEndpointAddress>()) {
						const TcpEndpointAddress &addr
							= GetRule().GetDestinations()[0].GetCombinedTypedAddress<TcpEndpointAddress>();
						m_destinationCertificate = addr.GetCertificate();
						if (!m_destinationCertificate.IsEmpty()) {
							m_destinationSslUse->SetValue(true);
							m_destinationSslSettings->Enable(true);
							m_destinationRemoteCertificates
								= addr.GetRemoteCertificates();
						}
					}
					if (GetRule().GetDestinations()[0].CheckCombinedAddressType<PathfinderEndpointAddress>()) {
						m_destinationPathfinderUse->SetValue(true);
					}
					if (GetRule().GetDestinations()[0].CheckCombinedAddressType<TcpEndpointAddress>()) {
						const TcpEndpointAddress &addr
							= GetRule().GetDestinations()[0].GetCombinedTypedAddress<TcpEndpointAddress>();
						m_destinationProxyUse->SetValue(addr.GetProxyList().size() > 0);
						m_destinationProxySettings->Enable(m_destinationProxyUse->GetValue());
						foreach (const Proxy &proxy, addr.GetProxyList()) {
							ProxyDlg::Info proxyInfo;
							proxyInfo.host = proxy.host;
							proxyInfo.port = boost::lexical_cast<std::wstring>(proxy.port);
							proxyInfo.isAuthInUse = !proxy.user.empty();
							proxyInfo.user = proxy.user;
							proxyInfo.password = proxy.password;
							m_proxyCascade.push_back(proxyInfo);
						}
					}
				}
			} else if (m_typePipe->GetValue()) {
				const std::wstring resourceIdentifier(
					GetRule().GetDestinations()[0].GetCombinedResourceIdentifier().GetCStr());
				const boost::wregex exp(L"([^:/]+)://(.+)");
				boost::wsmatch what;
				if (	boost::regex_match(resourceIdentifier, what, exp)
						&& !wxString(what[1].str()).CompareTo(wxT("pipe"), wxString::ignoreCase)) {
					m_destinationPipe->SetValue(what[2].str());
				} else {
					assert(false);
				}
			} else if (m_typeSerial->GetValue()) {
				assert(GetRule().GetDestinations()[0].CheckCombinedAddressType<SerialEndpointAddress>());
				if (GetRule().GetDestinations()[0].CheckCombinedAddressType<SerialEndpointAddress>()) {
					const SerialEndpointAddress &addr
						= GetRule().GetDestinations()[0].GetCombinedTypedAddress<SerialEndpointAddress>();
					m_destinationSerialLine->SetValue(addr.GetLine());
					m_destinationSerialBaudRate->SetValue(boost::lexical_cast<std::wstring>(addr.GetBaudRate()));
					m_destinationSerialDataBits->SetValue(boost::lexical_cast<std::wstring>(addr.GetDataBits()));
					m_destinationSerialStopBits->SetValue(boost::lexical_cast<std::wstring>(addr.GetStopBits()));
					typedef std::map<wxString, SerialEndpointAddress::Parity> ParityMap;
					ParityMap parityMap;
					RuleUtils::GetSerialParityValsMap(parityMap);
					foreach (const ParityMap::value_type &val, parityMap) {
						if (val.second == addr.GetParity()) {
							m_destinationSerialParity->SetStringSelection(val.first);
							break;
						}
					}
					typedef std::map<wxString, SerialEndpointAddress::FlowControl> FcMap;
					FcMap fcMap;
					RuleUtils::GetSerialFlowControlValsMap(fcMap);
					foreach (const FcMap::value_type &val, fcMap) {
						if (val.second == addr.GetFlowControl()) {
							m_destinationSerialFlowControl->SetStringSelection(val.first);
							break;
						}
					}
				}
			} else {
				assert(false);
			}
		}
	}

}

void TunnelRuleShortDlg::UpdateVisible() {
	wxWindowUpdateLocker freeze(this);
	if (IsNewRule()) {
		UpdateVisibleNewRule();
	} else {
		UpdateVisibleRuleEdit();
	}
	GetSizer()->SetSizeHints(this);
	Layout();
	CheckLicense();
}

void TunnelRuleShortDlg::UpdateVisibleRuleEdit() {

	if (	m_typeTcp->GetValue()
			|| m_typeUdp->GetValue()
			|| m_typeFtp->GetValue()) {

		m_tcpUdpFtpBox->Show();

		if (!m_typeUdp->GetValue()) {
			const wxChar *sslLabel;
			if (m_typeFtp->GetValue()) {
				sslLabel = wxT("FTP over SSL/TLS (FTPS)");
			} else {
				sslLabel = wxT("Secure connection (SSL/TLS)");
			}
			m_inputSslUse->SetLabel(sslLabel);
			m_destinationSslUse->SetLabel(sslLabel);
			m_sslInputSubPanel->Show();
			m_sslDestinationSubPanel->Show();
		} else {
			m_sslInputSubPanel->Hide();
			m_sslDestinationSubPanel->Hide();
		}

		if (m_typeFtp->GetValue()) {
			m_destinationTcpUdpFtpBox->SetLabel(wxT("FTP server"));
		} else {
			m_destinationTcpUdpFtpBox->SetLabel(wxT("Destination"));
		}

		const bool showProxy = !m_typeUdp->GetValue(); 
		m_proxySubPanel->Show(showProxy);
		foreach (wxHyperlinkCtrl *const link, m_proxyPanelLinks) {
			link->Show(showProxy && IsLicenseValid());
		}

	} else {
		m_tcpUdpFtpBox->Hide();
	}

	{
		const wxSize minSize = m_tcpUdpFtpBox->GetMinSize();
		m_tcpUdpFtpBox->GetSizer()->SetSizeHints(m_tcpUdpFtpBox);
		m_tcpUdpFtpBox->SetMinSize(minSize);
	}

	m_pipeBox->Show(m_typePipe->GetValue());
	m_serialBox->Show(m_typeSerial->GetValue());
	m_prevStepButton->Hide();
	m_okButton->Enable(IsLicenseValid());

}

void TunnelRuleShortDlg::UpdateVisibleNewRule() {
	switch (m_step) {
		default:
			assert(false);
		case 1:
			{
				ShowGeneralSettings();
				m_typeBox->Show();
				if (m_typeTcp->GetValue() || m_typeUdp->GetValue()) {
					m_typeDescription->SetLabel(
						wxT("Redirects an network traffic to another port")
							wxT(" on local or remote system."));
					m_typeFtpLink->Hide();
				} else if (m_typeFtp->GetValue()) {
					m_typeDescription->SetLabel(
						wxT("Forwards an FTP connection to another host,")
							wxT(" to a real FTP server."));
					m_typeFtpLink->Show();
				} else if (m_typePipe->GetValue()) {
					m_typeDescription->SetLabel(wxT("Redirects Windows Named Pipe traffic."));
					m_typeFtpLink->Hide();
				} else if (m_typeSerial->GetValue()) {
					m_typeDescription->SetLabel(wxT("Redirects Serial Port (COM) traffic."));
					m_typeFtpLink->Hide();
				} else {
					assert(false);
				}
				const wxSize typeBoxMinSize = m_typeBox->GetMinSize();
				m_typeBox->GetSizer()->SetSizeHints(m_typeBox);
				m_typeBox->SetMinSize(typeBoxMinSize);
				m_ruleEnableBox->Show();
				m_tcpUdpFtpBox->Hide();
				m_pipeBox->Hide();
				m_serialBox->Hide();
				m_prevStepButton->Enable(false);
				m_okButton->Enable(true);
				m_okButton->SetLabel(wxT("Next >"));
			}
			break;
		case 2:
			ShowGeneralSettings(false);
			m_typeBox->Hide();
			m_ruleEnableBox->Hide();
			if (	m_typeTcp->GetValue()
					|| m_typeUdp->GetValue()
					|| m_typeFtp->GetValue()) {
				
				m_tcpUdpFtpBox->Show();
				
				if (!m_typeUdp->GetValue()) {
					m_sslInputSubPanel->Show();
					m_sslDestinationSubPanel->Show();
					const wxChar *sslLabel;
					if (m_typeFtp->GetValue()) {
						sslLabel = wxT("FTP over SSL/TLS (FTPS)");
					} else {
						sslLabel = wxT("Secure connection (SSL/TLS)");
					}
					m_inputSslUse->SetLabel(sslLabel);
					m_destinationSslUse->SetLabel(sslLabel);
					m_proxySubPanel->Show();
				} else {
					m_sslInputSubPanel->Hide();
					m_sslDestinationSubPanel->Hide();
					m_proxySubPanel->Hide();
				}

				if (m_typeFtp->GetValue()) {
					m_destinationTcpUdpFtpBox->SetLabel(wxT("FTP server"));
				} else {
					m_destinationTcpUdpFtpBox->SetLabel(wxT("Destination"));
				}
				
				const bool showProxy = !m_typeUdp->GetValue(); 
				m_proxySubPanel->Show(showProxy);
				foreach (wxHyperlinkCtrl *const link, m_proxyPanelLinks) {
					link->Show(showProxy && IsLicenseValid());
				}
				
			} else {
				m_tcpUdpFtpBox->Hide();
			}
			{
				const wxSize minSize = m_tcpUdpFtpBox->GetMinSize();
				m_tcpUdpFtpBox->GetSizer()->SetSizeHints(m_tcpUdpFtpBox);
				m_tcpUdpFtpBox->SetMinSize(minSize);
			}
			m_pipeBox->Show(m_typePipe->GetValue());
			m_serialBox->Show(m_typeSerial->GetValue());
			m_prevStepButton->Enable(true);
			m_okButton->Enable(IsLicenseValid());
			m_okButton->SetLabel(wxT("Finish"));
			break;
	}
}

void TunnelRuleShortDlg::OnAdvancedMode(wxCommandEvent &evnt) {
	const int answer = wxMessageBox(
			wxT("Save current changes?"),
			wxT("Rule edit"),
			wxYES_NO | wxCANCEL | wxICON_QUESTION,
			this);
	switch (answer) {
		default:
			assert(false);
		case wxCANCEL:
			return;
		case wxYES:
			if (IsNewRule() && m_step == GetFirstStep()) {
				m_step = GetLastStep();
				UpdateVisible();
			}
			SetReturnCode(0);
			OnOk(evnt);
			break;
		case wxNO:
			SetReturnCode(0);
			OnCancel(evnt);
			break;
	}
	if (GetReturnCode() != 0) {
		m_isAdvancedMode = true;
	}
}

void TunnelRuleShortDlg::OnPrevStep(wxCommandEvent &) {
	assert(m_step > 1);
	--m_step;
	UpdateVisible();
}

std::auto_ptr<wxSizer> TunnelRuleShortDlg::CreateControlOptions() {
	return std::auto_ptr<wxSizer>();
}

std::auto_ptr<wxSizer> TunnelRuleShortDlg::CreateControlRuleInfo() {
	wxButton &ctrl
		= *new wxButton(this, CONTROL_ID_ADVANCED_MODE, wxT("Advanced mode..."));
	std::auto_ptr<wxBoxSizer> result(new wxBoxSizer(wxHORIZONTAL));
	result->Add(&ctrl, wxSizerFlags(0).Center().Expand());
	return result;
}

std::auto_ptr<wxSizer> TunnelRuleShortDlg::CreateButtons() {
	std::auto_ptr<wxStdDialogButtonSizer> box(new wxStdDialogButtonSizer);
	m_prevStepButton = new wxButton(this, CONTROL_ID_PREV_STEP, wxT("< Previous"));
	box->Add(m_prevStepButton);
	m_okButton = new wxButton(this, wxID_OK);
	box->AddButton(m_okButton);
	box->AddButton(new wxButton(this, wxID_CANCEL));
	box->AddButton(new wxButton(this, wxID_HELP));
	m_okButton->SetDefault();
	m_okButton->SetFocus();
	SetAffirmativeId(wxID_OK);
	box->Realize();
	return box;
}

void TunnelRuleShortDlg::OnOk(wxCommandEvent &evt) {
	if (!IsNewRule() || m_step >= GetLastStep()) {
		Base::OnOk(evt);
	} else {
		++m_step;
		UpdateVisible();
	}
}

void TunnelRuleShortDlg::Cancel() {
	if (!m_isAdvancedMode) {
		RuleUtils::CheckInactiveAdapterWarning(GetRule(), GetService());
	}
	Base::Cancel();
}

void TunnelRuleShortDlg::CheckLicense() {
	
	const bool prevVal = m_isLicenseValid;
	
	if (!m_typeFtp->GetValue()) {
		m_isLicenseValid = true;
	} else {
		bool newVal = m_licenses->ftp.IsFeatureAvailable(true);
		if (!newVal && newVal != m_isLicenseValid) {
			LicenseRestrictionDlg(GetServiceWindow(), this, m_licenses->ftp, false)
				.ShowModal();
			newVal = wxGetApp().IsUnlimitedModeActive();
		}
		m_isLicenseValid = newVal;
	}

	if (m_isLicenseValid != prevVal) {
		foreach (wxPanel *const panel, m_allPanels) {
			if (panel == m_typeBox) {
				continue;
			}
			panel->Enable(m_isLicenseValid);
		}
		EnableGeneralSettings(m_isLicenseValid);
	}

}

void TunnelRuleShortDlg::OnUseInputSslToggle(wxCommandEvent &) {
	OnUseSslToggle(
		*m_inputSslUse,
		*m_inputSslSettings,
		m_inputCertificate,
		m_inputRemoteCertificates);
}

void TunnelRuleShortDlg::OnUseDestintationSslToggle(wxCommandEvent &) {
	OnUseSslToggle(
		*m_destinationSslUse,
		*m_destinationSslSettings,
		m_destinationCertificate,
		m_destinationRemoteCertificates);
}

void TunnelRuleShortDlg::OnUseSslToggle(
			wxCheckBox &checkBox,
			wxButton &button,
			SslCertificateId &certificate,
			SslCertificateIdCollection &remoteCertificates)
		const {
	if (checkBox.GetValue()) {
		if (!m_licenses->ssl.IsFeatureAvailable(true)) {
			checkBox.SetValue(wxGetApp().IsUnlimitedModeActive());
			LicenseRestrictionDlg(
					const_cast<TunnelRuleShortDlg *>(this)->GetServiceWindow(),
					const_cast<TunnelRuleShortDlg *>(this),
					m_licenses->ssl,
					true)
				.ShowModal();
		}
		if (checkBox.GetValue()) {
			certificate = TcpEndpointAddress::GetAnonymousSslCertificateMagicName();
		}
	} else {
		certificate.Clear();
		remoteCertificates.SetSize(0);
	}
	button.Enable(checkBox.GetValue());
}

void TunnelRuleShortDlg::OnInputSslSettings(wxCommandEvent &) {
	OnSslSettings(true, m_inputCertificate, m_inputRemoteCertificates);
}

void TunnelRuleShortDlg::OnDestintationSslSettings(wxCommandEvent &) {
	OnSslSettings(false, m_destinationCertificate, m_destinationRemoteCertificates);
}

void TunnelRuleShortDlg::OnSslSettings(
			bool isServer,
			SslCertificateId &certificate,
			SslCertificateIdCollection &remoteCertificates)
		const {
	RuleUtils::ShowSslSettingsDialog(
		const_cast<TunnelRuleShortDlg *>(this)->GetServiceWindow(),
		const_cast<TunnelRuleShortDlg *>(this),
		isServer,
		m_licenses->ssl,
		certificate,
		remoteCertificates);
}
