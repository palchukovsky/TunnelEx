/**************************************************************************
 *   Created: 2008/01/27 5:40
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "EndpointDlg.hpp"
#include "Validators.hpp"
#include "Application.hpp"
#include "RuleUtils.hpp"

#include "LicenseRestrictionDlg.hpp"
#include "LicensePolicies.hpp"

#include "ServiceWindow.hpp"

#include "Modules/Pathfinder/PathfinderEndpointAddress.hpp"
#include "Modules/Inet/InetEndpointAddress.hpp"
#include "Modules/Serial/SerialEndpointAddress.hpp"
#include "Modules/Upnp/UpnpEndpointAddress.hpp"

using namespace TunnelEx;
using namespace TunnelEx::Mods;
using namespace TunnelEx::Mods::Inet;
using TunnelEx::Mods::Pathfinder::PathfinderEndpointAddress;
using TunnelEx::Mods::Inet::UdpEndpointAddress;
using TunnelEx::Mods::Serial::SerialEndpointAddress;
using TunnelEx::Mods::Upnp::UpnpEndpointAddress;
using TunnelEx::Mods::Upnp::UpnpTcpEndpointAddress;
using TunnelEx::Mods::Upnp::UpnpUdpEndpointAddress;

//////////////////////////////////////////////////////////////////////////

struct EndpointDlg::Licenses {

	explicit Licenses(ServiceAdapter &service)
			: rwSplit(LicenseState(service)),
			proxy(LicenseState(service)),
			proxyCascade(LicenseState(service)),
			ssl(LicenseState(service)) {
		//...//
	}

	Licensing::EndpointIoSeparationLicense rwSplit;
	Licensing::ProxyLicense proxy;
	Licensing::ProxyCascadeLicense proxyCascade;
	Licensing::SslLicense ssl;

};

//////////////////////////////////////////////////////////////////////////

void EndpointDlg::EndpointInfoItem::SetAdapter(
			const wxString &newAdapter,
			const std::list<texs__NetworkAdapterInfo> &adapters) {
	RuleUtils::SelectAdapter(*adapterInput, newAdapter, adapters);
}

void EndpointDlg::EndpointInfoItem::SetUpnpAdapter() {
	RuleUtils::SelectUpnpAdapter(*adapterInput);
}

EndpointDlg::EndpointType EndpointDlg::EndpointInfoItem::GetType() const {
	return static_cast<EndpointDlg::EndpointType>(typeInput->GetCurrentSelection());
}

void EndpointDlg::EndpointInfoItem::SetType(EndpointType type) {
	return typeInput->SetSelection(static_cast<int>(type));
}

bool EndpointDlg::EndpointInfoItem::IsAccepting() const {
	return
		isAcceptingInput->GetCurrentSelection() == 0
		&&	(GetType() == ENDPOINT_TYPE_NETWORK
			|| GetType() == ENDPOINT_TYPE_PIPE);

}
void EndpointDlg::EndpointInfoItem::SetAccepting(bool flag) {
	isAcceptingInput->SetSelection(flag ? 0 : 1);
	isAcceptingUdp->SetSelection(flag ? 0 : 1);
}

void EndpointDlg::EndpointInfoItem::EnableProxy(bool isEnable) {
	proxyUseInput->SetValue(isEnable);
	proxySettings->Enable(isEnable);
	if (!isEnable) {
		proxyCascade.resize(0);
	}
}

void EndpointDlg::EndpointInfoItem::ResetProxy() {
	EnableProxy(false);
}

void EndpointDlg::EndpointInfoItem::EnablePathfinder(bool isEnable) {
	pathfinderUseInput->SetValue(isEnable);
}

void EndpointDlg::EndpointInfoItem::EnableSsl(bool isEnable) {
	sslUseInput->SetValue(isEnable);
	sslSettings->Enable(isEnable);
	if (isEnable) {
		if (certificate.IsEmpty()) {
			certificate  = TcpEndpointAddress::GetAnonymousSslCertificateMagicName();
		}
	} else {
		certificate.Clear();
		remoteCertificates.SetSize(0);
	}
}

void EndpointDlg::EndpointInfoItem::EnableSsl(
			const SslCertificateId &newCertificate,
			const SslCertificateIdCollection &newRemoteCertificates) {
	assert(!newCertificate.IsEmpty() || newRemoteCertificates.GetSize() == 0);
	const bool isEnabled = !newCertificate.IsEmpty();
	sslUseInput->SetValue(isEnabled);
	sslSettings->Enable(isEnabled);
	certificate = newCertificate;
	remoteCertificates = newRemoteCertificates;
}

void EndpointDlg::EndpointInfoItem::ResetSsl() {
	EnableSsl(false);
}

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(EndpointDlg, wxDialog)
	EVT_BUTTON(		wxID_OK,									EndpointDlg::OnOk)
	EVT_BUTTON(		wxID_CANCEL,								EndpointDlg::OnCancel)
	EVT_BUTTON(		wxID_HELP,									EndpointDlg::OnHelp)
	EVT_COMMAND(	wxID_ANY, wxEVT_COMMAND_TEXT_ENTER,			EndpointDlg::OnOk)
	EVT_CHOICE(		EndpointDlg::CONTROL_ID_TYPE,				EndpointDlg::OnEndpointTypeChange)
	EVT_CHOICE(		EndpointDlg::CONTROL_ID_TYPE_WRITE,			EndpointDlg::OnEndpointTypeChange)
	EVT_CHOICE(		EndpointDlg::CONTROL_ID_NETWORK_PROTOCOL,	EndpointDlg::OnEndpointTypeChange)
	EVT_CHOICE(		EndpointDlg::CONTROL_ID_NETWORK_PROTOCOL_WRITE, EndpointDlg::OnEndpointTypeChange)
	EVT_CHOICE(		EndpointDlg::CONTROL_ID_NETWORK_ADAPTER,	EndpointDlg::OnNetworkAdapterChange)
	EVT_CHOICE(		EndpointDlg::CONTROL_ID_NETWORK_ADAPTER_WRITE, EndpointDlg::OnNetworkAdapterChange)
	EVT_CHECKBOX(	EndpointDlg::CONTROL_ID_LOGTRAFFIC_TOGGLE,	EndpointDlg::OnLogToggle)
	EVT_BUTTON(		EndpointDlg::CONTROL_ID_LOGTRAFFIC_FOLDER_SELECT, EndpointDlg::OnLogFolderBrowse)
	EVT_TEXT(		EndpointDlg::CONTROL_ID_PORT,				EndpointDlg::OnCombinedOrReadPortChanged)
	EVT_TEXT(		EndpointDlg::CONTROL_ID_PORT_WRITE,			EndpointDlg::OnWritePortChanged)
	EVT_CHOICE(		EndpointDlg::CONTROL_ID_READ_WRITE_TYPE,	EndpointDlg::OnEndpointReadWriteTypeChange)
	EVT_CHOICE(		EndpointDlg::CONTROL_ID_ACCEPTING,			EndpointDlg::OnAcceptingToggle)
	EVT_CHOICE(		EndpointDlg::CONTROL_ID_ACCEPTING_WRITE,	EndpointDlg::OnAcceptingWriteToggle)
	EVT_CHOICE(		EndpointDlg::CONTROL_ID_ACCEPTING_UDP,		EndpointDlg::OnAcceptingToggle)
	EVT_CHOICE(		EndpointDlg::CONTROL_ID_ACCEPTING_UDP_WRITE, EndpointDlg::OnAcceptingWriteToggle)
	EVT_CHECKBOX(	EndpointDlg::CONTROL_ID_SSL_USE,			EndpointDlg::OnSslUseToggle)
	EVT_CHECKBOX(	EndpointDlg::CONTROL_ID_SSL_USE_WRITE,		EndpointDlg::OnSslUseWriteToggle)
	EVT_BUTTON(		EndpointDlg::CONTROL_ID_SSL_SETTINGS,		EndpointDlg::OnSslSettings)
	EVT_BUTTON(		EndpointDlg::CONTROL_ID_SSL_SETTINGS_WRITE,	EndpointDlg::OnSslSettingsWrite)
	EVT_CHECKBOX(	EndpointDlg::CONTROL_ID_PROXY_USE,			EndpointDlg::OnProxyUseToggle)
	EVT_CHECKBOX(	EndpointDlg::CONTROL_ID_PROXY_USE_WRITE,	EndpointDlg::OnProxyUseWriteToggle)
	EVT_BUTTON(		EndpointDlg::CONTROL_ID_PROXY_SETTINGS,		EndpointDlg::OnProxySettings)
	EVT_BUTTON(		EndpointDlg::CONTROL_ID_PROXY_SETTINGS_WRITE, EndpointDlg::OnProxySettingsWrite)
	EVT_CHECKBOX(	EndpointDlg::CONTROL_ID_PATHFINDER_USE, EndpointDlg::OnPathfinderUseToggle)
	EVT_CHECKBOX(	EndpointDlg::CONTROL_ID_PATHFINDER_USE_WRITE, EndpointDlg::OnIsPathfinderUseWriteToggle)
END_EVENT_TABLE()

EndpointDlg::EndpointDlg(
			ServiceWindow &service,
			wxWindow *parent,
			bool isInputEndpoint,
			bool isFtp)
		: wxDialog(parent, wxID_ANY, wxEmptyString),
		m_hasChanges(true),
		m_isInputEndpoint(isInputEndpoint),
		m_isFtpEndpoint(isFtp),
		m_isNewEndpoint(true),
		m_service(service),
		m_licenses(new Licenses(m_service.GetService())),
		m_isUpnpDevChecked(false) {
	Init();
}

EndpointDlg::EndpointDlg(
			ServiceWindow &service,
			wxWindow *parent,
			RuleEndpoint &endpoint,
			bool isInputEndpoint,
			bool isFtp)
		: wxDialog(parent, wxID_ANY, wxEmptyString),
		m_endpoint(endpoint),
		m_hasChanges(false),
		m_isInputEndpoint(isInputEndpoint),
		m_isFtpEndpoint(isFtp),
		m_isNewEndpoint(false),
		m_service(service),
		m_licenses(new Licenses(m_service.GetService())),
		m_isUpnpDevChecked(false) {
	Init();
}

EndpointDlg::~EndpointDlg() {
	//...//
}

void EndpointDlg::Init() {

	m_service.GetService().GetNetworkAdapters(true, m_serviceNetworkAdapters);

	SetSize(535, 550);
	SetTitle(
		m_isInputEndpoint
			?	wxT("Tunnel output new endpoint setup")
			:	!m_isFtpEndpoint
				?	wxT("Tunnel input endpoint setup")
				:	wxT("FTP server setup"));

	m_borderWidth = 10;
	m_internalBorderWidth = 3;

	m_endpointsInfo[0].isReadOrCombined = true;
	m_endpointsInfo[1].isReadOrCombined = false;

	ReadLogInfo();
	CreateControls();
	ReadEndpointInfo();

	m_isFirstTimeReadWriteTypeChanged = !IsSplitEndpoint();

	ShowControls();

	ShowSplitRwLicenseRestriction();

}

bool EndpointDlg::SaveEndpointAddress(
			bool isOrigEndpointCombined,
			bool isNewEndpointCombined,
			const EndpointInfoItem &info,
			boost::function<SharedPtr<EndpointAddress>(void)> getAddressFunc,
			TunnelEx::WString &resultResourceIdentifier)
		const {
	
	std::wostringstream resourceIdentifier;
	switch (info.GetType()) {

		default:
			assert(false);

		case ENDPOINT_TYPE_NETWORK:
			{
				
				const unsigned short port
					= RuleUtils::ConvertPort(info.portInput->GetValue());
				
				if (	!m_isFtpEndpoint
						&& info.IsAccepting()
						&& RuleUtils::IsUpnpAdapterSelected(*info.adapterInput)) {

					assert(info.adapterInput->GetCount() > 0);

					if (info.networkProtoInput->GetStringSelection() == wxT("TCP")) {
						resourceIdentifier << UpnpTcpEndpointAddress::CreateResourceIdentifier(
								port,
								info.certificate,
								info.remoteCertificates)
							.GetCStr();
					} else {
						assert(info.networkProtoInput->GetStringSelection() == wxT("UDP"));
						resourceIdentifier << UpnpUdpEndpointAddress::CreateResourceIdentifier(
								port,
								info.certificate,
								info.remoteCertificates)
							.GetCStr();
					}
				
				} else if (info.networkProtoInput->GetStringSelection() == wxT("TCP")) {
					
					typedef boost::function<WString()>
						SimpleResourceIdentifierFabric;
					typedef boost::function<WString(const ProxyList &)>
						ProxyResourceIdentifierFabric;

					SimpleResourceIdentifierFabric simpleResourceIdentifierFabric;
					ProxyResourceIdentifierFabric proxyResourceIdentifierFabric;
					if (!info.pathfinderUseInput->GetValue()) {
						simpleResourceIdentifierFabric = boost::bind(
							&TcpEndpointAddress::CreateResourceIdentifier,
							std::wstring(info.hostInput->GetValue().c_str()),
							port,
							boost::cref(info.certificate),
							boost::cref(info.remoteCertificates));
						proxyResourceIdentifierFabric = boost::bind(
							static_cast<
									WString(*)(
										const std::wstring &,
										NetworkPort,
										const SslCertificateId &,
										const SslCertificateIdCollection &,
										const ProxyList &)>(
								&TcpEndpointAddress::CreateResourceIdentifier),
							std::wstring(info.hostInput->GetValue().c_str()),
							port,
							boost::cref(info.certificate),
							boost::cref(info.remoteCertificates),
							_1);
					} else {
						simpleResourceIdentifierFabric = boost::bind(
							&PathfinderEndpointAddress::CreateResourceIdentifier,
							std::wstring(info.hostInput->GetValue().c_str()),
							port,
							boost::cref(info.certificate),
							boost::cref(info.remoteCertificates));
						proxyResourceIdentifierFabric = boost::bind(
							&PathfinderEndpointAddress::CreateResourceIdentifier,
							std::wstring(info.hostInput->GetValue().c_str()),
							port,
							boost::cref(info.certificate),
							boost::cref(info.remoteCertificates),
							_1);
					}
					
					if (info.IsAccepting() && m_isInputEndpoint) {
						std::list<texs__NetworkAdapterInfo>::const_iterator adapter
							= m_serviceNetworkAdapters.begin();
						advance(adapter, info.adapterInput->GetCurrentSelection());
						resourceIdentifier
							<< TcpEndpointAddress::CreateResourceIdentifier(
									wxString::FromAscii(adapter->id.c_str()).c_str(),
									port,
									L"*",
									info.certificate,
									info.remoteCertificates)
								.GetCStr();
					} else if (
							info.proxyUseInput->GetValue()
							&& info.proxyCascade.size() > 0) {
						ProxyList proxyList;
						foreach (
								const ProxyDlg::Info &proxyInfo,
								info.proxyCascade) {
							Proxy proxy;
							proxy.host = proxyInfo.host.c_str();
							proxy.port = RuleUtils::ConvertPort(proxyInfo.port);
							if (proxyInfo.isAuthInUse) {
								proxy.user = proxyInfo.user.c_str();
								proxy.password = proxyInfo.password.c_str();
							}
							proxyList.push_back(proxy);
						}
						resourceIdentifier
							<< proxyResourceIdentifierFabric(proxyList).GetCStr();
					} else {
						resourceIdentifier
							<< simpleResourceIdentifierFabric().GetCStr();
					}
				} else if (info.networkProtoInput->GetStringSelection() == wxT("UDP")) {
					if (info.IsAccepting() && m_isInputEndpoint) {
						std::list<texs__NetworkAdapterInfo>::const_iterator adapter
							= m_serviceNetworkAdapters.begin();
						std::advance(adapter, info.adapterInput->GetCurrentSelection());
						resourceIdentifier
							<< UdpEndpointAddress::CreateResourceIdentifier(
									wxString::FromAscii(adapter->id.c_str())
										.c_str(),
									port,
									L"*")
								.GetCStr();
					} else {
						resourceIdentifier
							<< UdpEndpointAddress::CreateResourceIdentifier(
									info.hostInput->GetValue().c_str(),
									port)
								.GetCStr();
					}
				} else {
					assert(false);
				}
			}
			break;

		case ENDPOINT_TYPE_PIPE:
			resourceIdentifier << L"pipe://" << info.pipeInput->GetValue().c_str();
			break;

		case ENDPOINT_TYPE_SERIAL:
			resourceIdentifier << RuleUtils::CreateSerialResourceIdentifier(
					info.serialLineInput->GetValue(),
					info.serialBaudRateInput->GetValue(),
					info.serialDataBitsInput->GetValue(),
					info.serialStopBitsInput->GetValue(),
					info.serialParityInput->GetStringSelection(),
					info.serialFlowControlInput->GetStringSelection())
				.GetCStr();
			break;

	}

	const bool hasChanged
		=	isOrigEndpointCombined != isNewEndpointCombined
			|| (!getAddressFunc()
				|| getAddressFunc()->GetResourceIdentifier()
					!= resourceIdentifier.str().c_str());
	resultResourceIdentifier = resourceIdentifier.str().c_str();

	return hasChanged;

}

void EndpointDlg::OnOk(wxCommandEvent &) {
	
	if (!Validate()) {
		return;
	}

	RuleEndpoint newEndpoint(m_endpoint);
	bool hasChanges = m_endpoint.IsCombined() == !IsSplitEndpoint();
	if (!IsSplitEndpoint()) {
		WString resourceIdentifier;
		hasChanges = SaveEndpointAddress(
			m_endpoint.IsCombined(),
			true,
			m_endpointsInfo[0],
			boost::bind(&RuleEndpoint::GetCombinedAddress, &newEndpoint),
			resourceIdentifier);
		const bool isAcceptor = m_isInputEndpoint && m_endpointsInfo[0].IsAccepting();
		hasChanges
			= hasChanges
				||	!m_endpoint.IsCombined()
				||	m_endpoint.IsCombinedAcceptor() != isAcceptor;
		newEndpoint.SetCombinedResourceIdentifier(resourceIdentifier, isAcceptor);
	} else {
		WString readResourceIdentifier;
		hasChanges = SaveEndpointAddress(
			m_endpoint.IsCombined(),
			false,
			m_endpointsInfo[0],
			boost::bind(&RuleEndpoint::GetReadAddress, &newEndpoint),
			readResourceIdentifier);
		WString writeResourceIdentifier;
		hasChanges
			= SaveEndpointAddress(
				m_endpoint.IsCombined(),
				false,
				m_endpointsInfo[1],
				boost::bind(&RuleEndpoint::GetWriteAddress, &newEndpoint),
				writeResourceIdentifier)
			|| hasChanges;
		Endpoint::Acceptor acceptor = Endpoint::ACCEPTOR_NONE;
		if (m_isInputEndpoint) {
			if (m_endpointsInfo[0].IsAccepting()) {
				acceptor = Endpoint::ACCEPTOR_READER;
			} else if (m_endpointsInfo[1].IsAccepting()) {
				acceptor = Endpoint::ACCEPTOR_WRITER;
			}
			hasChanges = hasChanges || acceptor != m_endpoint.GetReadWriteAcceptor();
		}
		newEndpoint.SetReadWriteResourceIdentifiers(
			readResourceIdentifier,
			writeResourceIdentifier,
			acceptor);
	}

	RuleEndpoint::Listeners &listeners = newEndpoint.GetPreListeners();
	if (m_logToggle->GetValue() != m_logInfoSource.isOn) {
		if (!m_logToggle->GetValue()) {
			for (unsigned int i = 0; i < listeners.GetSize(); ++i) {
				if (listeners[i].name == L"TrafficLogger/File") {
					listeners.Remove(i);
				} else {
					++i;
				}
			}
		} else {
			RuleEndpoint::ListenerInfo info;
			info.name = L"TrafficLogger/File";
			info.param = m_logFolderInput->GetValue();
			listeners.Append(info);
		}
		hasChanges = true;
	} else if (m_logToggle->GetValue() && m_logInfoSource.path != m_logFolderInput->GetValue()) {
		size_t i;
		const size_t listenersNumb = listeners.GetSize();
		for (i = 0; i < listenersNumb; ++i) {
			if (listeners[i].name == L"TrafficLogger/File") {
				listeners[i].param = m_logFolderInput->GetValue();
				hasChanges = true;
				break;
			}
		}
		assert(listenersNumb > i);
	}

	if (hasChanges) {
		SaveEnpointTemplate();
		std::swap(m_endpoint, newEndpoint);
		m_hasChanges = true;
	}

	CheckInactiveAdapterWarning(m_endpoint);
	
	EndModal(m_hasChanges ? wxID_OK : wxID_CANCEL);

}

void EndpointDlg::OnCancel(wxCommandEvent &) {
	CheckInactiveAdapterWarning(m_endpoint);
	EndModal(wxID_CANCEL);
}

void EndpointDlg::SaveEnpointTemplate() const {

	struct Util {
		static wxString InfoToTypeName(
					const EndpointInfoItem &info,
					const bool isFtp) {
			wxString result;
			switch (info.GetType()) {
				case ENDPOINT_TYPE_NETWORK:
					result = info.networkProtoInput->GetStringSelection();
					assert(!(isFtp && result != wxT("TCP")));
					if (result == wxT("TCP") && isFtp) {
						result = wxT("FTP");
					}
					break;
				case ENDPOINT_TYPE_PIPE:
					assert(!isFtp);
					result = wxT("pipe");
					break;
				case ENDPOINT_TYPE_SERIAL:
					assert(!isFtp);
					result = wxT("serial");
					break;
				default:
					assert(false);
			}
			return result;
		}
	};

	if (!IsSplitEndpoint()) {
		const wxString type = Util::InfoToTypeName(m_endpointsInfo[0], m_isFtpEndpoint);
		if (!type.IsEmpty()) {
			wxGetApp().GetConfig().Write(
				wxT("/Rule/Template/Endpoint/Type/Destination"),
				type);
			if (m_isInputEndpoint) {
				wxGetApp().GetConfig().Write(
					wxT("/Rule/Template/Endpoint/Type/Input"),
					type);
			}
		}
		wxGetApp().GetConfig().Write(
			wxT("/Rule/Template/Endpoint/Acceptor"),
			m_endpointsInfo[0].IsAccepting()
				?	wxT("reader")
				:	wxT("none"));
	} else {
		const wxString readType = Util::InfoToTypeName(m_endpointsInfo[0], m_isFtpEndpoint);
		const wxString writeType = Util::InfoToTypeName(m_endpointsInfo[1], m_isFtpEndpoint);
		if (!readType.IsEmpty() && readType == writeType) {
			wxGetApp().GetConfig().Write(
				wxT("/Rule/Template/Endpoint/Type/Destination"),
				readType);
			if (m_isInputEndpoint) {
				wxGetApp().GetConfig().Write(
					wxT("/Rule/Template/Endpoint/Type/Input"),
					readType);
			}
		}
		const wxChar *const acceptor = m_endpointsInfo[0].IsAccepting()
			?	wxT("reader")
			:	m_endpointsInfo[1].IsAccepting()
				?	wxT("writer")
				:	wxT("none");
		wxGetApp().GetConfig().Write(
			wxT("/Rule/Template/Endpoint/Acceptor"),
			acceptor);
	}

}

void EndpointDlg::OnHelp(wxCommandEvent &) {
	wxGetApp().DisplayHelp(
		!m_isFtpEndpoint
			?	wxT("dialogs/custom-endpoint")
			:	m_isInputEndpoint
				?	wxT("dialogs/ftp-tunnel-input")
				:	wxT("dialogs/ftp-server-setup"));
}

bool EndpointDlg::IsInputEndpoint() const {
	return m_isInputEndpoint;
}

void EndpointDlg::OnLogToggle(wxCommandEvent &) {
	const bool isEnabled = m_logToggle->GetValue();
	m_logFolderLabel->Enable(isEnabled);
	m_logFolderInput->Enable(isEnabled);
	m_logBrowseButton->Enable(isEnabled);
}

void EndpointDlg::OnWritePortChanged(wxCommandEvent &) {
	OnPortChanged(m_endpointsInfo[1]);
}

void EndpointDlg::OnCombinedOrReadPortChanged(wxCommandEvent &) {
	OnPortChanged(m_endpointsInfo[0]);
}

void EndpointDlg::OnPortChanged(EndpointInfoItem &info) const {
	std::wstring checkValue = info.portInput->GetValue().c_str();
	boost::trim(checkValue);
	if (checkValue.empty()) {
		return;
	}
	if (!boost::regex_match(checkValue, boost::wregex(L"\\d+"))) {
		const long pos = std::max(long(0), info.portInput->GetInsertionPoint() - 1);
		info.portInput->ChangeValue(info.portValid);
		info.portInput->SetInsertionPoint(std::min(info.portInput->GetLastPosition(), pos));
	} else {
		info.portValid = checkValue;
	}
}

void EndpointDlg::OnEndpointTypeChange(wxCommandEvent &) {
	ShowControls();
}

void EndpointDlg::OnEndpointReadWriteTypeChange(wxCommandEvent &) {
	const bool isWasCombined = !IsSplitEndpoint();
	if (	m_isFirstTimeReadWriteTypeChanged
			&& isWasCombined
			&& IsSplitEndpoint()) {
		m_endpointsInfo[1].SetType(m_endpointsInfo[0].GetType());
		m_isFirstTimeReadWriteTypeChanged = false;
	}
	ShowControls();
	ShowSplitRwLicenseRestriction();
}

void EndpointDlg::OnLogFolderBrowse(wxCommandEvent &) {
	wxDirDialog dirDlg(this, wxT("Choose a folder"), m_logFolderInput->GetValue());
	if (dirDlg.ShowModal() == wxID_OK) {
		 m_logFolderInput->SetValue(dirDlg.GetPath());
	}
}

void EndpointDlg::CreateControls() {
	wxWindowUpdateLocker freeze(this);
	CreateControlReadWriteTypeEndpoint();
	CreateControlEndpoint(m_endpointsInfo[0]);
	CreateControlEndpoint(m_endpointsInfo[1]);
	CreateControlLog();
	CreateControlDialogButtons();
}

void EndpointDlg::ShowControls() {

	{

		wxWindowUpdateLocker freeze(this);

		int y = 0;
		
		ShowControlReadWriteTypeEndpoint(y);
		ShowControlEndpoint(m_endpointsInfo[0], y);
		ShowControlEndpoint(m_endpointsInfo[1], y);
		ShowControlLog(y);
		ShowControlDialogButtons(
			(IsSplitEndpoint()
				&& !m_licenses->rwSplit.IsFeatureAvailable(true)
				&& !wxGetApp().IsUnlimitedModeActive()),
			y);

		wxSize size(GetClientSize());
		size.SetHeight(y + (m_borderWidth * 2));
		SetClientSize(size);

	}

	wxGetApp().Yield(true);
	CheckControlValues(m_endpointsInfo[0]);
	CheckControlValues(m_endpointsInfo[1]);

}

void EndpointDlg::CheckControlValues(EndpointInfoItem &info) {
	
	if (	info.externalIpInput->IsShown()
			&& info.externalIpInput->GetValue().IsEmpty()) {
		if (!m_isUpnpDevChecked || !m_service.GetService().GetCachedUpnpDeviceExternalIp()) {
			wxString localIp;
			wxString externalIp;
			if (!m_service.GetService().GetUpnpStatus(localIp, externalIp)) {
				wxLogWarning(
					wxT("Could not find router with the UPnP support in the local network."));
			}
			m_isUpnpDevChecked = true;
		}
		info.externalIpInput->SetValue(*m_service.GetService().GetCachedUpnpDeviceExternalIp());
		if (info.externalIpInput->GetValue().IsEmpty()) {
			info.externalIpInput->SetValue(wxT("unknown"));
		}
	}

}

void EndpointDlg::CreateControlReadWriteTypeEndpoint() {

	const wxSize clientSize(GetClientSize());
	wxPoint pos(m_borderWidth, m_borderWidth);

	m_readWriteLabel
		= new wxStaticText(this, -1, wxT("Reading and writing:"), pos);
	pos.x = m_readWriteLabel->GetPosition().x + m_internalBorderWidth + m_readWriteLabel->GetSize().x;

	wxArrayString types;
	types.Add(wxT("one endpoint"));
	types.Add(wxT("I/O channels separation"));
	m_readWriteInput = new wxChoice(
		this,
		CONTROL_ID_READ_WRITE_TYPE,
		pos,
		wxDefaultSize,
		types,
		wxTE_NOHIDESEL | wxTE_PROCESS_ENTER);
	m_readWriteInput->SetSelection(IsSplitEndpoint() ? 1 : 0);

	CenterControls(*m_readWriteLabel, *m_readWriteInput);

}

void EndpointDlg::ShowControlReadWriteTypeEndpoint(int &y) {
	const bool show = !m_isFtpEndpoint;
	m_readWriteLabel->Show(show);
	m_readWriteInput->Show(show);
	if (!show) {
		return;
	}
	y += m_borderWidth + m_readWriteInput->GetSize().GetHeight();
}

void EndpointDlg::ShowControlEndpoint(EndpointInfoItem &info, int &y) {

	if (IsSplitEndpoint()) {
		assert(!m_isFtpEndpoint);
		info.group->SetLabel(
			info.isReadOrCombined
				?	wxT("Endpoint: reading")
				:	wxT("Endpoint: writing"));
	} else if (!info.isReadOrCombined) {
		info.group->Show(false);
		info.networkProtoInput->Show(false);
		info.networkProtoLabel->Show(false);
		info.adapterLabel->Show(false);
		info.adapterInput->Show(false);
		info.hostLabel->Show(false);
		info.hostInput->Show(false);
		info.externalIpLabel->Show(false);
		info.externalIpInput->Show(false);
		info.portLabel->Show(false);
		info.portInput->Show(false);
		info.pipeLabel->Show(false);
		info.pipeInput->Show(false);
		info.typeLabel->Show(false);
		info.typeInput->Show(false);
		info.isAcceptingLabel->Show(false);
		info.isAcceptingInput->Show(false);
		info.isAcceptingUdp->Show(false);
		info.sslUseInput->Show(false);
		info.sslSettings->Show(false);
		info.proxyUseInput->Show(false);
		info.proxySettings->Show(false);
		info.proxyLink->Show(false);
		info.pathfinderUseInput->Show(false);
		info.pathfinderLink->Show(false);
		info.serialLineLabel->Show(false);
		info.serialLineInput->Show(false);
		info.serialConfGroup->Show(false);
		info.serialBaudRateLabel->Show(false);
		info.serialBaudRateInput->Show(false);
		info.serialDataBitsLabel->Show(false);
		info.serialDataBitsInput->Show(false);
		info.serialStopBitsLabel->Show(false);
		info.serialStopBitsInput->Show(false);
		info.serialParityLabel->Show(false);
		info.serialParityInput->Show(false);
		info.serialFlowControlLabel->Show(false);
		info.serialFlowControlInput->Show(false);
		return;
	} else {
		info.group->SetLabel(wxT("Endpoint"));
	}

	y += m_borderWidth;
	int x = 0;
	info.group->GetPosition(&x, 0);
	info.group->SetPosition(wxPoint(x, y));
	info.group->Show(true);
	y += m_borderWidth;

	ShowControlEndpointType(info, y);
	ShowControlEndpointNetworkProto(info, y);
	ShowControlEndpointAction(info, y);
	ShowControlEndpointNetworkAddress(info, y);
	ShowControlEndpointSsl(info, y);
	ShowControlEndpointProxy(info, y);
	ShowControlEndpointPathfinder(info, y);
	ShowControlEndpointPipe(info, y);
	ShowControlEndpointSerial(info, y);

	wxSize gropSize = info.group->GetSize();
	gropSize.SetHeight(y + m_borderWidth - info.group->GetPosition().y);
	info.group->SetSize(gropSize);
	y += m_borderWidth;

}

void EndpointDlg::ShowControlEndpointType(EndpointInfoItem &info, int &y) {
	const bool show = !m_isFtpEndpoint;
	info.typeLabel->Show(show);
	info.typeInput->Show(show);
	if (!show) {
		return;
	}
	y += m_borderWidth;
	int x;
	info.typeLabel->GetPosition(&x, 0);
	info.typeLabel->SetPosition(wxPoint(x, y));
	info.typeInput->GetPosition(&x, 0);
	info.typeInput->SetPosition(wxPoint(x, y));
	CenterControls(*info.typeLabel, *info.typeInput);
	y += info.typeInput->GetSize().GetHeight();
}

void EndpointDlg::ShowControlEndpointNetworkProto(EndpointInfoItem &info, int &y) {
	const bool show = !m_isFtpEndpoint && info.GetType() == ENDPOINT_TYPE_NETWORK;
	info.networkProtoLabel->Show(show);
	info.networkProtoInput->Show(show);
	if (!show) {
		return;
	}
	y += m_borderWidth;
	int x;
	info.networkProtoLabel->GetPosition(&x, 0);
	info.networkProtoLabel->SetPosition(wxPoint(x, y));
	info.networkProtoInput->GetPosition(&x, 0);
	info.networkProtoInput->SetPosition(wxPoint(x, y));
	CenterControls(*info.networkProtoLabel, *info.networkProtoInput);
	y += info.networkProtoInput->GetSize().GetHeight();
}

void EndpointDlg::ShowControlEndpointAction(EndpointInfoItem &info, int &y) {
	const bool show
		=	!m_isFtpEndpoint
			&&	m_isInputEndpoint
			&& (info.GetType() == ENDPOINT_TYPE_NETWORK
				|| info.GetType() == ENDPOINT_TYPE_PIPE);
	info.isAcceptingLabel->Show(show);
	if (!show) {
		info.isAcceptingInput->Show(false);
		info.isAcceptingUdp->Show(false);
		return;
	}
	y += m_borderWidth;
	int x;
	info.isAcceptingLabel->GetPosition(&x, 0);
	info.isAcceptingLabel->SetPosition(wxPoint(x, y));
	info.isAcceptingInput->GetPosition(&x, 0);
	if (info.GetType() == ENDPOINT_TYPE_NETWORK) {
		if (info.networkProtoInput->GetStringSelection() == wxT("TCP")) {
			info.isAcceptingInput->SetPosition(wxPoint(x, y));
			CenterControls(*info.isAcceptingLabel, *info.isAcceptingInput);
			info.isAcceptingInput->Show(true);
			info.isAcceptingUdp->Show(false);
		} else {
			assert(info.networkProtoInput->GetStringSelection() == wxT("UDP"));
			info.isAcceptingUdp->SetPosition(wxPoint(x, y));
			CenterControls(*info.isAcceptingLabel, *info.isAcceptingUdp);
			info.isAcceptingInput->Show(false);
			info.isAcceptingUdp->Show(true);
		}
	} else if (info.GetType() == ENDPOINT_TYPE_PIPE) {
		info.isAcceptingInput->SetPosition(wxPoint(x, y));
		CenterControls(*info.isAcceptingLabel, *info.isAcceptingInput);
		info.isAcceptingInput->Show(true);
		info.isAcceptingUdp->Show(false);
	} else {
		info.isAcceptingInput->Show(false);
		info.isAcceptingUdp->Show(false);
		assert(false);
	}
	y += info.isAcceptingInput->GetSize().GetHeight();
}

void EndpointDlg::ShowControlEndpointNetworkAddress(EndpointInfoItem &info, int &y) {
	
	const bool show = info.GetType() == ENDPOINT_TYPE_NETWORK;
	const bool isAdapter = show && m_isInputEndpoint && info.IsAccepting();
	const bool isUpnp
		= !m_isFtpEndpoint 
			&& isAdapter
			&& info.adapterInput->GetCurrentSelection() + 1
				== long(info.adapterInput->GetCount());
	
	info.adapterLabel->Show(isAdapter);
	info.adapterInput->Show(isAdapter);
	info.hostLabel->Show(show && !isAdapter);
	info.hostInput->Show(show && !isAdapter);
	info.portLabel->Show(show);
	info.portInput->Show(show);
	info.externalIpLabel->Show(show && isUpnp);
	info.externalIpInput->Show(show && isUpnp);
	if (!show) {
		return;
	}

	y += m_borderWidth;
	int x;
	if (!isAdapter) {
		info.hostLabel->GetPosition(&x, 0);
		info.hostLabel->SetPosition(wxPoint(x, y));
		info.hostInput->GetPosition(&x, 0);
		info.hostInput->SetPosition(wxPoint(x, y));
		CenterControls(*info.hostLabel, *info.hostInput);
	} else {
		info.adapterLabel->GetPosition(&x, 0);
		info.adapterLabel->SetPosition(wxPoint(x, y));
		info.adapterInput->GetPosition(&x, 0);
		info.adapterInput->SetPosition(wxPoint(x, y));
		if (!isUpnp) {
			std::list<texs__NetworkAdapterInfo>::const_iterator adapter
				= m_serviceNetworkAdapters.begin();
			std::advance(adapter, info.adapterInput->GetCurrentSelection());
			wxString toolTip = wxString::FromAscii(adapter->name.c_str());
			if (!toolTip.empty() && !adapter->ipAddress.empty()) {
				toolTip += wxT(": ");
			}
			toolTip += wxString::FromAscii(adapter->ipAddress.c_str());
			info.adapterInput->SetToolTip(toolTip);
		}
		CenterControls(*info.adapterLabel, *info.adapterInput);
	}

	if (isUpnp) {

		y += info.adapterInput->GetSize().GetHeight();
		y += m_borderWidth;

		info.externalIpLabel->GetPosition(&x, 0);
		info.externalIpLabel->SetPosition(wxPoint(x, y));
		info.externalIpInput->GetPosition(&x, 0);
		info.externalIpInput->SetPosition(wxPoint(x, y));
		CenterControls(*info.externalIpLabel, *info.externalIpInput);

	}

	info.portLabel->GetPosition(&x, 0);
	info.portLabel->SetPosition(wxPoint(x, y));
	info.portInput->GetPosition(&x, 0);
	info.portInput->SetPosition(wxPoint(x, y));
	CenterControls(*info.portLabel, *info.portInput);
	
	y += info.portInput->GetSize().GetHeight();

}

void EndpointDlg::CenterControls(wxControl &forMoving, const wxControl &forTemplate) {
	if (forMoving.GetSize().GetHeight() < forTemplate.GetSize().GetHeight()) {
		wxPoint pos(forMoving.GetPosition());
		pos.y += (forTemplate.GetSize().GetHeight() - forMoving.GetSize().GetHeight()) / 2;
		forMoving.SetPosition(pos);
	}
}

void EndpointDlg::ShowControlEndpointSerial(EndpointInfoItem &info, int &y) {

	const bool show = !m_isFtpEndpoint && info.GetType() == ENDPOINT_TYPE_SERIAL;
	
	info.serialLineLabel->Show(show);
	info.serialLineInput->Show(show);
	info.serialConfGroup->Show(show);
	info.serialBaudRateLabel->Show(show);
	info.serialBaudRateInput->Show(show);
	info.serialDataBitsLabel->Show(show);
	info.serialDataBitsInput->Show(show);
	info.serialStopBitsLabel->Show(show);
	info.serialStopBitsInput->Show(show);
	info.serialParityLabel->Show(show);
	info.serialParityInput->Show(show);
	info.serialFlowControlLabel->Show(show);
	info.serialFlowControlInput->Show(show);

	if (!show) {
		return;
	}

	y += m_borderWidth;
	int x = 0;
		
	info.serialLineLabel->GetPosition(&x, 0);
	info.serialLineLabel->SetPosition(wxPoint(x, y));
	info.serialLineInput->GetPosition(&x, 0);
	info.serialLineInput->SetPosition(wxPoint(x, y));
	CenterControls(*info.serialLineLabel, *info.serialLineInput);
	
	y += info.serialLineInput->GetSize().GetHeight() + m_borderWidth;

	info.serialConfGroup->GetPosition(&x, 0);
	info.serialConfGroup->SetPosition(wxPoint(x, y));
	
	y += m_borderWidth * 2;

	info.serialBaudRateLabel->GetPosition(&x, 0);
	info.serialBaudRateLabel->SetPosition(wxPoint(x, y));
	info.serialBaudRateInput->GetPosition(&x, 0);
	info.serialBaudRateInput->SetPosition(wxPoint(x, y));
	CenterControls(*info.serialBaudRateLabel, *info.serialBaudRateInput);

	info.serialDataBitsLabel->GetPosition(&x, 0);
	info.serialDataBitsLabel->SetPosition(wxPoint(x, y));
	info.serialDataBitsInput->GetPosition(&x, 0);
	info.serialDataBitsInput->SetPosition(wxPoint(x, y));
	CenterControls(*info.serialDataBitsLabel, *info.serialDataBitsInput);

	info.serialStopBitsLabel->GetPosition(&x, 0);
	info.serialStopBitsLabel->SetPosition(wxPoint(x, y));
	info.serialStopBitsInput->GetPosition(&x, 0);
	info.serialStopBitsInput->SetPosition(wxPoint(x, y));
	CenterControls(*info.serialStopBitsLabel, *info.serialStopBitsInput);

	y += info.serialStopBitsInput->GetSize().GetHeight() + m_borderWidth;

	info.serialParityLabel->GetPosition(&x, 0);
	info.serialParityLabel->SetPosition(wxPoint(x, y));
	info.serialParityInput->GetPosition(&x, 0);
	info.serialParityInput->SetPosition(wxPoint(x, y));
	CenterControls(*info.serialParityLabel, *info.serialParityInput);

	info.serialFlowControlLabel->GetPosition(&x, 0);
	info.serialFlowControlLabel->SetPosition(wxPoint(x, y));
	info.serialFlowControlInput->GetPosition(&x, 0);
	info.serialFlowControlInput->SetPosition(wxPoint(x, y));
	CenterControls(*info.serialFlowControlLabel, *info.serialFlowControlInput);

	y += info.serialFlowControlInput->GetSize().GetHeight() + m_borderWidth;

}

void EndpointDlg::ShowControlEndpointPipe(EndpointInfoItem &info, int &y) {
	const bool show = !m_isFtpEndpoint && info.GetType() == ENDPOINT_TYPE_PIPE;
	info.pipeLabel->Show(show);
	info.pipeInput->Show(show);
	if (!show) {
		return;
	}
	y += m_borderWidth;
	int x;
	info.pipeLabel->GetPosition(&x, 0);
	info.pipeLabel->SetPosition(wxPoint(x, y));
	info.pipeInput->GetPosition(&x, 0);
	info.pipeInput->SetPosition(wxPoint(x, y));
	CenterControls(*info.pipeLabel, *info.pipeInput);
	y += info.pipeInput->GetSize().GetHeight();
}

void EndpointDlg::ShowControlEndpointSsl(EndpointInfoItem &info, int &y) {
	const bool show
		=	info.GetType() == ENDPOINT_TYPE_NETWORK
			&& info.networkProtoInput->GetStringSelection() == wxT("TCP");
	info.sslUseInput->Show(show);
	info.sslSettings->Show(show);
	if (!show) {
		return;
	}
	y += m_borderWidth;
	int x;
	info.sslUseInput->GetPosition(&x, 0);
	info.sslUseInput->SetPosition(wxPoint(x, y));
	info.sslSettings->GetPosition(&x, 0);
	info.sslSettings->SetPosition(wxPoint(x, y));
	CenterControls(*info.sslUseInput, *info.sslSettings);
	y += info.sslSettings->GetSize().GetHeight();
}


void EndpointDlg::ShowControlEndpointProxy(EndpointInfoItem &info, int &y) {
	const bool show
		= info.GetType() == ENDPOINT_TYPE_NETWORK
			&& info.networkProtoInput->GetStringSelection() == wxT("TCP")
			&& (!info.IsAccepting() || !m_isInputEndpoint);
	info.proxyUseInput->Show(show);
	info.proxySettings->Show(show);
	info.proxyLink->Show(show);
	if (!show) {
		return;
	}
	y += m_borderWidth;
	int x;
	info.proxyUseInput->GetPosition(&x, 0);
	info.proxyUseInput->SetPosition(wxPoint(x, y));
	info.proxySettings->GetPosition(&x, 0);
	info.proxySettings->SetPosition(wxPoint(x, y));
	info.proxyLink->GetPosition(&x, 0);
	info.proxyLink->SetPosition(wxPoint(x, y));
	CenterControls(*info.proxyUseInput, *info.proxySettings);
	CenterControls(*info.proxyLink, *info.proxySettings);
	y += info.proxySettings->GetSize().GetHeight();
}

void EndpointDlg::ShowControlEndpointPathfinder(EndpointInfoItem &info, int &y) {

	const bool show
		= info.GetType() == ENDPOINT_TYPE_NETWORK
		&& info.networkProtoInput->GetStringSelection() == wxT("TCP")
		&& (!info.IsAccepting() || !m_isInputEndpoint);

	info.pathfinderUseInput->Show(show);
	info.pathfinderLink->Show(show);

	if (!show) {
		return;
	}
	y += m_borderWidth;
	int x;
	info.pathfinderUseInput->GetPosition(&x, 0);
	info.pathfinderUseInput->SetPosition(wxPoint(x, y));
	info.pathfinderLink->GetPosition(&x, 0);
	info.pathfinderLink->SetPosition(wxPoint(x, y));
	CenterControls(*info.pathfinderLink, *info.pathfinderUseInput);
	y += info.pathfinderUseInput->GetSize().GetHeight();

}

void EndpointDlg::CreateControlEndpoint(EndpointInfoItem &info) {

	const int labelsWidth = !m_isFtpEndpoint
		?	58
		:	!m_isInputEndpoint ? 80 : 80 - 37;
	const int groupWidth = GetClientSize().GetWidth() - (m_borderWidth * 2);

	info.group = new wxStaticBox(
		this,
		wxID_ANY,
		wxEmptyString,
		wxPoint(m_borderWidth, -1),
		wxSize(groupWidth, -1));

	CreateControlEndpointType(info, groupWidth, labelsWidth);
	CreateControlEndpointNetworkProto(info, groupWidth, labelsWidth);
	CreateControlEndpointAction(info, groupWidth, labelsWidth);
	CreateControlEndpointNetworkAddress(info, groupWidth, labelsWidth);
	CreateControlEndpointSsl(info, groupWidth, labelsWidth);
	CreateControlEndpointProxy(info, groupWidth, labelsWidth);
	CreateControlEndpointPathfinder(info, groupWidth, labelsWidth);
	CreateControlEndpointPipe(info, groupWidth, labelsWidth);
	CreateControlEndpointSerial(info, groupWidth, labelsWidth);

}
	
void EndpointDlg::CreateControlEndpointType(
			EndpointInfoItem &info,
			int,
			int labelWidth) {
	info.typeLabel = new wxStaticText(
		this,
		-1,
		wxT("Type:"),
		wxPoint(m_borderWidth * 2, -1),
		wxSize(labelWidth, -1),
		wxALIGN_RIGHT);
	wxArrayString types;
	types.Add(wxT("network connection"));
	types.Add(wxT("named pipe"));
	types.Add(wxT("serial"));
	info.typeInput = new wxChoice(
		this,
		info.isReadOrCombined ? CONTROL_ID_TYPE : CONTROL_ID_TYPE_WRITE,
		wxPoint(m_internalBorderWidth + (m_borderWidth * 2) + labelWidth, -1),
		wxDefaultSize,
		types,
		wxTE_NOHIDESEL | wxTE_PROCESS_ENTER);
	info.typeInput->SetToolTip(
		wxT("Allows to specify which type of the endpoint will be used."));
}

void EndpointDlg::CreateControlEndpointNetworkProto(
			EndpointInfoItem &info,
			int,
			int labelWidth) {
	info.networkProtoLabel = new wxStaticText(
		this,
		wxID_ANY,
		wxT("Protocol:"),
		wxPoint(m_borderWidth * 2, -1),
		wxSize(labelWidth, -1),
		wxALIGN_RIGHT);
	wxArrayString protocols;
	protocols.Add(wxT("TCP"));
	protocols.Add(wxT("UDP"));
	info.networkProtoInput = new wxChoice(
		this,
		info.isReadOrCombined ? CONTROL_ID_NETWORK_PROTOCOL : CONTROL_ID_NETWORK_PROTOCOL_WRITE,
		wxPoint(m_internalBorderWidth + (m_borderWidth * 2) + labelWidth, -1),
		wxDefaultSize,
		protocols,
		wxTE_NOHIDESEL | wxTE_PROCESS_ENTER);
	info.networkProtoInput->SetToolTip(
		wxT("Allows to specify which protocol the data packet should be using."));
}

void EndpointDlg::CreateControlEndpointAction(
			EndpointInfoItem &info,
			int,
			int labelWidth) {
	info.isAcceptingLabel = new wxStaticText(
		this,
		wxID_ANY,
		wxT("Action:"),
		wxPoint(m_borderWidth * 2, -1),
		wxSize(labelWidth, -1),
		wxALIGN_RIGHT);
	wxPoint pos(m_internalBorderWidth + (m_borderWidth * 2) + labelWidth, -1);
	{
		wxArrayString list;
		list.Add(wxT("wait and accept connections"));
		list.Add(wxT("open a connection to this endpoint"));
		info.isAcceptingInput = new wxChoice(
			this,
			info.isReadOrCombined ? CONTROL_ID_ACCEPTING : CONTROL_ID_ACCEPTING_WRITE,
			pos,
			wxDefaultSize,
			list,
			wxCHK_2STATE);
	}
	{
		wxArrayString list;
		list.Add(wxT("wait and accept datagram"));
		list.Add(wxT("send datagram to this endpoint"));
		info.isAcceptingUdp = new wxChoice(
			this,
			info.isReadOrCombined ? CONTROL_ID_ACCEPTING_UDP : CONTROL_ID_ACCEPTING_UDP_WRITE,
			pos,
			wxDefaultSize,
			list,
			wxCHK_2STATE);
	}
	const int width = std::max(
		info.isAcceptingInput->GetSize().GetWidth(),
		info.isAcceptingUdp->GetSize().GetWidth());
	info.isAcceptingInput->SetSize(
		wxSize(
			width,
			info.isAcceptingInput->GetSize().GetHeight()));
	info.isAcceptingUdp->SetSize(
		wxSize(
			width,
			info.isAcceptingUdp->GetSize().GetHeight()));
}

void EndpointDlg::CreateControlEndpointNetworkAddress(
			EndpointInfoItem &info,
			int groupWidth,
			int labelWidth) {
	info.portLabel = new wxStaticText(this, wxID_ANY, wxT("Port:"));
	info.portLabel->SetPosition(
		wxPoint(
			groupWidth - info.portLabel->GetSize().GetWidth()
				- RuleUtils::GetPortFieldSize().GetWidth()
				- m_internalBorderWidth,
			-1));
	// Adapter: //////////////////////////////////////////////////////////
	info.adapterLabel = new wxStaticText(
		this,
		wxID_ANY,
		wxT("Adapter:"),
		wxPoint(m_borderWidth * 2, -1),
		wxSize(labelWidth, -1),
		wxALIGN_RIGHT);
	info.adapterInput = &RuleUtils::CreateAdapterSelector(
		m_serviceNetworkAdapters,
		this,
		info.isReadOrCombined ? CONTROL_ID_NETWORK_ADAPTER : CONTROL_ID_NETWORK_ADAPTER_WRITE,
		m_isFtpEndpoint,
		wxPoint((m_borderWidth * 2) + m_internalBorderWidth + labelWidth, -1),
		wxSize(
			groupWidth - (m_borderWidth * 3)
				- (m_internalBorderWidth * 2) - labelWidth
				- RuleUtils::GetPortFieldSize().GetWidth()
				- info.portLabel->GetSize().GetWidth(),
			-1));
	// Host: //////////////////////////////////////////////////////////
	info.hostLabel = new wxStaticText(
		this,
		wxID_ANY,
		!m_isFtpEndpoint ? wxT("Host:") : wxT("FTP server host:"),
		wxPoint(m_borderWidth * 2, -1),
		wxSize(labelWidth, -1),
		wxALIGN_RIGHT);
	info.hostInput = new wxTextCtrl(
		this,
		wxID_ANY,
		wxEmptyString,
		wxPoint((m_borderWidth * 2) + m_internalBorderWidth + labelWidth, -1),
		wxSize(
			groupWidth - (m_borderWidth * 3)
				- (m_internalBorderWidth * 2) - labelWidth
				- RuleUtils::GetPortFieldSize().GetWidth()
				- info.portLabel->GetSize().GetWidth(),
			-1),
		wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
		HostValidator(true));
	info.hostInput->SetToolTip(
		wxT("Hostname or IP address. It can be any local or remote hostname or IP address."));	
	// Port: ///////////////////////////////////////////////////////////
	info.portInput = new wxTextCtrl(
		this,
		info.isReadOrCombined ? CONTROL_ID_PORT : CONTROL_ID_PORT_WRITE,
		wxEmptyString,
		wxPoint(groupWidth - RuleUtils::GetPortFieldSize().GetWidth(), -1),
		RuleUtils::GetPortFieldSize(),
		wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
		NetworPortValidator(true));
	info.portInput->SetToolTip(wxT("Endpoint network port."));
	// External IP: ///////////////////////////////////////////////////////
	info.externalIpLabel = new wxStaticText(
		this,
		wxID_ANY,
		wxT("External IP:"),
		wxPoint(m_borderWidth * 2, -1),
		wxSize(labelWidth, -1),
		wxALIGN_RIGHT);
	info.externalIpInput = new wxTextCtrl(
		this,
		wxID_ANY,
		wxEmptyString,
		wxPoint((m_borderWidth * 2) + m_internalBorderWidth + labelWidth, -1),
		wxSize(
			groupWidth - (m_borderWidth * 3)
				- (m_internalBorderWidth * 2) - labelWidth
				- RuleUtils::GetPortFieldSize().GetWidth()
				- info.portLabel->GetSize().GetWidth(),
			-1),
		wxTE_NOHIDESEL | wxTE_PROCESS_ENTER);
	info.externalIpInput->Enable(false);
	info.externalIpInput->SetToolTip(wxT("External IP address."));	
}

void EndpointDlg::CreateControlEndpointSerial(
			EndpointInfoItem &info,
			int groupWidth,
			int labelWidth) {
	
	info.serialLineLabel = new wxStaticText(
		this,
		wxID_ANY,
		wxT("Serial line:"),
		wxPoint(m_borderWidth * 2, -1),
		wxSize(labelWidth, -1));
	info.serialLineInput = new wxTextCtrl(
		this,
		wxID_ANY,
		wxEmptyString,
		wxPoint((m_borderWidth * 2) + m_internalBorderWidth + labelWidth, -1),
		wxSize(
			groupWidth - (m_borderWidth * 2) - m_internalBorderWidth - labelWidth,
			-1),
		wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
		NotEmptyValidator(wxT("serial line"), true));
	info.serialLineInput->SetToolTip(wxT("Serial line to connect."));

	int x = (m_borderWidth * 2) + m_internalBorderWidth + labelWidth;

	info.serialConfGroup = new wxStaticBox(
		this,
		wxID_ANY,
		wxT("Serial line configuration"),
		wxPoint(x, -1),
		wxSize(groupWidth - (m_borderWidth * 2) - labelWidth - m_internalBorderWidth, -1));

	const int localFieldsWidth = 61;
	x += m_borderWidth;

	info.serialBaudRateLabel = new wxStaticText(
		this,
		wxID_ANY,
		wxT("Speed (baud):"),
		wxPoint(x, -1));
	x += info.serialBaudRateLabel->GetSize().GetWidth() + m_internalBorderWidth;
	info.serialBaudRateInput = new wxTextCtrl(
		this,
		wxID_ANY,
		wxEmptyString,
		wxPoint(x , -1),
		wxSize(localFieldsWidth, -1),
		wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
		NumericValidator(wxT("speed (baud)"), true));

	x += localFieldsWidth + m_borderWidth;

	info.serialDataBitsLabel
		= new wxStaticText(this, wxID_ANY, wxT("Data bits:"), wxPoint(x, -1));
	x += info.serialDataBitsLabel->GetSize().GetWidth() + m_internalBorderWidth;
	info.serialDataBitsInput = new wxTextCtrl(
		this,
		wxID_ANY,
		wxEmptyString,
		wxPoint(x, -1),
		wxSize(localFieldsWidth, -1),
		wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
		NumericValidator(wxT("data bits"), true));

	x += localFieldsWidth + m_borderWidth;
	
	info.serialStopBitsLabel
		= new wxStaticText(this, -1, wxT("Stop bits:"), wxPoint(x, -1));
	x += info.serialStopBitsLabel->GetSize().GetWidth() + m_internalBorderWidth;
	info.serialStopBitsInput = new wxTextCtrl(
		this,
		wxID_ANY,
		wxEmptyString,
		wxPoint(x, -1),
		wxSize(localFieldsWidth, -1),
		wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
		NumericValidator(wxT("stop bits"), true));

	x = (m_borderWidth * 3) + m_internalBorderWidth + labelWidth;
	
	info.serialParityLabel
		= new wxStaticText(this, -1, wxT("Parity:"), wxPoint(x, -1));
	x += info.serialParityLabel->GetSize().GetWidth() + m_internalBorderWidth;
	info.serialParityInput
		= &RuleUtils::CreateSerialConfParity(this, wxID_ANY, wxPoint(x, -1));

	x += info.serialParityInput->GetSize().GetWidth() + m_borderWidth;
	info.serialFlowControlLabel
		= new wxStaticText(this, wxID_ANY, wxT("Flow control:"), wxPoint(x, -1));
	x += info.serialFlowControlLabel->GetSize().GetWidth() + m_internalBorderWidth;
	info.serialFlowControlInput
		= &RuleUtils::CreateSerialConfFlowControl(this, wxID_ANY, wxPoint(x, -1));

	{
		wxSize size = info.serialConfGroup->GetSize();
		size.SetHeight(
			(m_borderWidth * 5)
				+ info.serialFlowControlLabel->GetSize().GetHeight()
				+ info.serialDataBitsInput->GetSize().GetHeight());
		info.serialConfGroup->SetSize(size);
	}

}

void EndpointDlg::CreateControlEndpointPipe(
			EndpointInfoItem &info,
			int groupWidth,
			int labelWidth) {
	info.pipeLabel = new wxStaticText(
		this,
		wxID_ANY,
		wxT("Pipe name:"),
		wxPoint(m_borderWidth * 2, -1),
		wxSize(labelWidth, -1),
		wxALIGN_RIGHT);
	info.pipeInput = new wxTextCtrl(
		this,
		wxID_ANY,
		wxEmptyString,
		wxPoint((m_borderWidth * 2) + m_internalBorderWidth + labelWidth, -1),
		wxSize(
			groupWidth - (m_borderWidth * 2) - m_internalBorderWidth - labelWidth,
			-1),
		wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
		PipeValidator(true));
	info.pipeInput->SetToolTip(
		wxT("Each named pipe has a unique name that distinguishes it from other named pipes in the system's list of named objects."));
}

void EndpointDlg::CreateControlEndpointSsl(EndpointInfoItem &info, int, int labelWidth) {
	
	info.sslUseInput = new wxCheckBox(
		this,
		info.isReadOrCombined ? CONTROL_ID_SSL_USE : CONTROL_ID_SSL_USE_WRITE,
		!m_isFtpEndpoint
			?	wxT("Secure connection (SSL/TLS)")
			:	wxT("FTP over SSL/TLS (FTPS)"),
		wxPoint((m_borderWidth * 2) + m_internalBorderWidth + labelWidth, -1),
		wxDefaultSize,
		wxCHK_2STATE);

	info.sslSettings = new wxButton(
		this,
		info.isReadOrCombined ? CONTROL_ID_SSL_SETTINGS : CONTROL_ID_SSL_SETTINGS_WRITE,
		wxT("Settings..."),
		wxPoint(
			info.sslUseInput->GetPosition().x
				+ info.sslUseInput->GetSize().GetWidth() + m_borderWidth,
			-1),
		wxSize(-1, info.portInput->GetSize().GetHeight()));
	info.sslSettings->SetToolTip(wxT("Secure connection settings."));

}

void EndpointDlg::CreateControlEndpointProxy(EndpointInfoItem &info, int, int labelWidth) {
	
	info.proxyUseInput = new wxCheckBox(
		this,
		info.isReadOrCombined ? CONTROL_ID_PROXY_USE : CONTROL_ID_PROXY_USE_WRITE,
		wxT("Use HTTP proxy server"),
		wxPoint((m_borderWidth * 2) + m_internalBorderWidth + labelWidth, -1),
		wxDefaultSize,
		wxCHK_2STATE);
	info.proxyUseInput->SetToolTip(
		wxT("Make connection through HTTP proxy server.")
			wxT(" Endpoint, which use proxy server,")
			wxT(" could not be used for connections accepting."));

	info.proxySettings = new wxButton(
		this,
		info.isReadOrCombined ? CONTROL_ID_PROXY_SETTINGS : CONTROL_ID_PROXY_SETTINGS_WRITE,
		wxT("Settings..."),
		wxPoint(
			info.proxyUseInput->GetPosition().x
				+ info.proxyUseInput->GetSize().GetWidth() + m_borderWidth,
			-1),
		wxSize(-1, info.portInput->GetSize().GetHeight()));
	info.proxySettings->SetToolTip(wxT("Proxy server settings."));

	info.proxyLink = new wxHyperlinkCtrl(
		this,
		wxID_ANY,
		wxT("About HTTP tunneling"),
		wxT("http://") TUNNELEX_DOMAIN_W wxT("/product/http-tunneling?about"),
		wxPoint(
			info.proxySettings->GetPosition().x
				+ info.proxySettings->GetSize().GetWidth() + m_borderWidth,
			-1));

}

void EndpointDlg::CreateControlEndpointPathfinder(EndpointInfoItem &info, int, int labelWidth) {

	info.pathfinderUseInput = new wxCheckBox(
		this,
		info.isReadOrCombined ? CONTROL_ID_PATHFINDER_USE : CONTROL_ID_PATHFINDER_USE_WRITE,
		wxT("Try to find path with Pathfinder online service"),
		wxPoint((m_borderWidth * 2) + m_internalBorderWidth + labelWidth, -1),
		wxDefaultSize,
		wxCHK_2STATE);
	info.pathfinderUseInput->SetToolTip(
		wxT("Pathfinder - a ") TUNNELEX_NAME_W wxT(" online service,")
			wxT(" which helps to find a path for connection to a server on the Internet.")
			wxT(" This is useful in cases where the security policy")
			wxT(" on the local network does not allow to connect")
			wxT(" to some Internet service or network port."));

	info.pathfinderLink = new wxHyperlinkCtrl(
		this,
		wxID_ANY,
		wxT("About Pathfinder Online Service"),
		wxT("http://") TUNNELEX_DOMAIN_W wxT("/product/pathfinder?about"),
		wxPoint(
			info.pathfinderUseInput->GetPosition().x
				+ info.pathfinderUseInput->GetSize().GetWidth() + m_borderWidth,
			-1));

}

void EndpointDlg::CreateControlLog() {

	const wxSize clientSize(GetClientSize());
	wxSize boxSize((clientSize.x - (m_borderWidth * 2)), -1);

	wxPoint pos(m_borderWidth, -1);

	m_logGroup = new wxStaticBox(this, wxID_ANY, wxT("Logging"), pos, boxSize);
	pos.x = m_logGroup->GetPosition().x + m_borderWidth;

	const wxChar *const label = m_isInputEndpoint
		?	wxT("Log input data into a file")
		:	wxT("Log output data into a file");
	const wxChar *const toolTip = m_isInputEndpoint
		?	wxT("Turn it on, if you want to save received data into a file.")
		:	wxT("Turn it on, if you want to save sent data into a file.");

	m_logToggle = new wxCheckBox(
		this,
		CONTROL_ID_LOGTRAFFIC_TOGGLE,
		label,
		pos,
		wxSize(boxSize.x - m_borderWidth - pos.x, -1),
		wxCHK_2STATE);
	m_logToggle->SetToolTip(toolTip);
	m_logToggle->SetValue(m_logInfoSource.isOn);
	
	class Validator : public wxValidator {
	public:
		virtual bool Validate(wxWindow *parent) {
			bool result = true;
			wxCheckBox *toggleCtrl = dynamic_cast<wxCheckBox *>(
				parent->FindWindow(EndpointDlg::CONTROL_ID_LOGTRAFFIC_TOGGLE));
			assert(toggleCtrl);
			if (toggleCtrl->GetValue()) {
				wxTextCtrl *ctrl = dynamic_cast<wxTextCtrl*>(GetWindow());
				assert(ctrl);
				//! \todo: implemented only for Windows [2008/01/29 4:19]
				const std::wstring dir(ctrl->GetValue().c_str());
				result = boost::regex_match(
					dir,
					boost::wregex(L"[a-z]:(\\\\|/)[^\"*?<>|]*",
					boost::regex_constants::perl | boost::regex_constants::icase));
			}
			if (!result) {
				//! \todo: make two different messages for these errors [2008/01/29 4:24]
				wxLogWarning(wxT("Please, provide an absolute valid path for the log folder.\nEx.: \"C:\\Temp files\\") TUNNELEX_NAME_W wxT(" logs\"."));
			}
			return result;
		}
		virtual wxObject * Clone() const {
			return new Validator;
		}
		virtual bool TransferToWindow() {
			return true;
		}
	} validator;

	pos.x += m_borderWidth;
	m_logFolderLabel = new wxStaticText(
		this,
		CONTROL_ID_LOGTRAFFIC_LABEL,
		wxT("Log folder:"),
		pos,
		wxDefaultSize,
		wxALIGN_RIGHT);
	m_logFolderLabel->Enable(m_logInfoSource.isOn);
	
	m_logBrowseButton = new wxButton(
		this,
		CONTROL_ID_LOGTRAFFIC_FOLDER_SELECT,
		wxT("Browse..."),
		pos);
	m_logBrowseButton->Enable(m_logInfoSource.isOn);
	m_logBrowseButton->SetPosition(
		wxPoint(
			m_logGroup->GetPosition().x + boxSize.GetWidth()
				- m_borderWidth - m_logBrowseButton->GetSize().GetWidth(),
			-1));

	pos.x
		= m_logFolderLabel->GetPosition().x + m_internalBorderWidth
			+ m_logFolderLabel->GetSize().GetWidth();
	m_logFolderInput = new wxTextCtrl(
		this,
		CONTROL_ID_LOGTRAFFIC_FOLDER,
		m_logInfoSource.path,
		pos,
		wxSize(
			m_logBrowseButton->GetPosition().x - pos.x - m_internalBorderWidth,
			-1),
		wxTE_NOHIDESEL | wxTE_PROCESS_ENTER,
		validator);
	m_logFolderInput->Enable(m_logInfoSource.isOn);
	m_logBrowseButton->SetToolTip(wxT("Folder path for logged data packets."));
	m_logBrowseButton->Enable(m_logInfoSource.isOn);
	m_logBrowseButton->SetSize(
		m_logBrowseButton->GetSize().GetWidth(),
		m_logFolderInput->GetSize().GetHeight());

}

void EndpointDlg::ShowControlLog(int &y) {
	
	y += m_borderWidth;
	int x;

	m_logGroup->GetPosition(&x, 0);
	m_logGroup->SetPosition(wxPoint(x, y));

	y += m_borderWidth * 2;
	m_logToggle->GetPosition(&x, 0);
	m_logToggle->SetPosition(wxPoint(x, y));

	y += m_borderWidth + m_logToggle->GetSize().GetHeight();

	m_logFolderLabel->GetPosition(&x, 0);
	m_logFolderLabel->SetPosition(wxPoint(x, y));

	m_logBrowseButton->GetPosition(&x, 0);
	m_logBrowseButton->SetPosition(wxPoint(x, y));

	m_logFolderInput->GetPosition(&x, 0);
	m_logFolderInput->SetPosition(wxPoint(x, y));

	CenterControls(*m_logFolderLabel, *m_logFolderInput);

	m_logGroup->SetSize(
		wxSize(
			m_logGroup->GetSize().GetWidth(),
			m_logToggle->GetSize().GetHeight() + m_logFolderInput->GetSize().GetHeight()
				+ (m_borderWidth * 4)));

	y += m_logFolderInput->GetSize().GetHeight() + m_borderWidth;

}

void EndpointDlg::CreateControlDialogButtons() {

	const wxSize size(80, 25);
	const wxSize clientSize(GetClientSize());

	m_line = new wxStaticLine(
		this,
		wxID_ANY,
		wxPoint(m_borderWidth, -1),
		wxSize(clientSize.GetWidth() - (m_borderWidth * 2), -1),
		wxLI_HORIZONTAL);

	wxPoint pos(
		clientSize.x - (m_borderWidth * 2) - m_internalBorderWidth - (size.x * 3),
		-1);

	m_okButton = new wxButton(this, wxID_OK, wxT("OK"), pos, size);
	m_okButton->SetToolTip(wxT("Save changes and close dialog."));
	pos.x += m_internalBorderWidth + size.x;

	m_cancelButton = new wxButton(this, wxID_CANCEL, wxT("Cancel"), pos, size);
	m_cancelButton->SetToolTip(wxT("Discard changes and close dialog."));
	pos.x += m_borderWidth + size.x;

	m_helpButton = new wxButton(this, wxID_HELP, wxT("Help"), pos, size);
	
}

void EndpointDlg::ShowControlDialogButtons(bool isReadOnly, int &y) {

	y += m_borderWidth;
	int x = 0;

	m_line->GetPosition(&x, 0);
	m_line->SetPosition(wxPoint(x, y));
	y += m_line->GetSize().GetHeight() + m_borderWidth;

	m_okButton->GetPosition(&x, 0);
	m_okButton->SetPosition(wxPoint(x, y));
	m_okButton->Enable(!isReadOnly);

	m_cancelButton->GetPosition(&x, 0);
	m_cancelButton->SetPosition(wxPoint(x, y));

	m_helpButton->GetPosition(&x, 0);
	m_helpButton->SetPosition(wxPoint(x, y));

	y += m_helpButton->GetSize().GetHeight();
	
}

void EndpointDlg::ReadLogInfo() {
	LogInfo info;
	info.path = RuleUtils::GetEndpoindLogPath(m_endpoint);
	info.isOn = !info.path.IsEmpty();
	std::swap(info, m_logInfoSource);
}

template<typename Address>
void EndpointDlg::ReadEndpointSslInfo(
			const Address &address,
			EndpointInfoItem &info)
		const {
	info.EnableSsl(address.GetCertificate(),  address.GetRemoteCertificates());
}

void EndpointDlg::ReadEndpointInfo(
			const TunnelEx::EndpointAddress &address,
			EndpointInfoItem &info)
		const {
	
	CreateDefaultEndpointInfo(info);

	if (dynamic_cast<const TcpEndpointAddress *>(&address)) {
		ReadEndpointSslInfo(*dynamic_cast<const TcpEndpointAddress *>(&address), info);
	} else if (dynamic_cast<const UpnpEndpointAddress *>(&address)) {
		ReadEndpointSslInfo(*dynamic_cast<const UpnpEndpointAddress*>(&address), info);
	}

	if (m_isFtpEndpoint) {

		info.SetType(ENDPOINT_TYPE_NETWORK);
		info.networkProtoInput->SetStringSelection(wxT("TCP"));

		assert(dynamic_cast<const TcpEndpointAddress *>(&address) != 0);
		if (dynamic_cast<const TcpEndpointAddress *>(&address) != 0) {
			const TcpEndpointAddress &typedAddress
				= *boost::polymorphic_downcast<const TcpEndpointAddress *>(&address);
			if (typedAddress.GetHostName() != L"*") {
				info.hostInput->SetValue(typedAddress.GetHostName());
			}
			info.SetAdapter(typedAddress.GetAdapter(), m_serviceNetworkAdapters);
			if (typedAddress.GetPort()) {
				info.portInput->SetValue(boost::lexical_cast<std::wstring>(typedAddress.GetPort()));
			}
			info.EnableProxy(typedAddress.GetProxyList().size() > 0);
			if (typedAddress.GetProxyList().size() > 0) {
				foreach (const Proxy &proxy, typedAddress.GetProxyList()) {
					ProxyDlg::Info proxyInfo;
					proxyInfo.host = proxy.host;
					proxyInfo.port = boost::lexical_cast<std::wstring>(proxy.port);
					proxyInfo.isAuthInUse = !proxy.user.empty();
					proxyInfo.user = proxy.user;
					proxyInfo.password = proxy.password;
					info.proxyCascade.push_back(proxyInfo);
				}
			}
			info.EnablePathfinder(
				dynamic_cast<const PathfinderEndpointAddress *>(&address) != 0);
		}

	} else if (	dynamic_cast<const PathfinderEndpointAddress *>(&address)
				|| dynamic_cast<const TcpEndpointAddress *>(&address)
				|| dynamic_cast<const UdpEndpointAddress *>(&address)) {

		info.SetType(ENDPOINT_TYPE_NETWORK);
		info.networkProtoInput->SetStringSelection(
			dynamic_cast<const UdpEndpointAddress *>(&address)
				?	wxT("UDP")
				:	wxT("TCP"));
	
		const InetEndpointAddress &inetAddress
			= *boost::polymorphic_downcast<const InetEndpointAddress *>(&address);
		if (inetAddress.GetHostName() != L"*") {
			info.hostInput->SetValue(inetAddress.GetHostName());
		}
		info.SetAdapter(inetAddress.GetAdapter(), m_serviceNetworkAdapters);
		if (inetAddress.GetPort()) {
			info.portInput->SetValue(boost::lexical_cast<std::wstring>(inetAddress.GetPort()));
		}

		if (dynamic_cast<const TcpEndpointAddress *>(&inetAddress)) {
			const TcpEndpointAddress &tcpAddress
				= *boost::polymorphic_downcast<const TcpEndpointAddress *>(&inetAddress);
			info.EnableProxy(tcpAddress.GetProxyList().size() > 0);
			if (tcpAddress.GetProxyList().size() > 0) { 
				foreach (const Proxy &proxy, tcpAddress.GetProxyList()) {
					ProxyDlg::Info proxyInfo;
					proxyInfo.host = proxy.host;
					proxyInfo.port = boost::lexical_cast<std::wstring>(proxy.port);
					proxyInfo.isAuthInUse = !proxy.user.empty();
					proxyInfo.user = proxy.user;
					proxyInfo.password = proxy.password;
					info.proxyCascade.push_back(proxyInfo);
				}
			}
		}

		info.EnablePathfinder(
			dynamic_cast<const PathfinderEndpointAddress *>(&address) != 0);

	} else if (dynamic_cast<const SerialEndpointAddress *>(&address)) {

		info.SetType(ENDPOINT_TYPE_SERIAL);

		const SerialEndpointAddress &typedAddress
			= *boost::polymorphic_downcast<const SerialEndpointAddress *>(&address);
		info.serialLineInput->SetValue(typedAddress.GetLine());
		info.serialBaudRateInput->SetValue(boost::lexical_cast<std::wstring>(typedAddress.GetBaudRate()));
		info.serialDataBitsInput->SetValue(boost::lexical_cast<std::wstring>(typedAddress.GetDataBits()));
		info.serialStopBitsInput->SetValue(boost::lexical_cast<std::wstring>(typedAddress.GetStopBits()));
		typedef std::map<wxString, SerialEndpointAddress::Parity> ParityMap;
		ParityMap parityMap;
		RuleUtils::GetSerialParityValsMap(parityMap);
		foreach (const ParityMap::value_type &val, parityMap) {
			if (val.second == typedAddress.GetParity()) {
				info.serialParityInput->SetStringSelection(val.first);
				break;
			}
		}
		typedef std::map<wxString, SerialEndpointAddress::FlowControl> FcMap;
		FcMap fcMap;
		RuleUtils::GetSerialFlowControlValsMap(fcMap);
		foreach (const FcMap::value_type &val, fcMap) {
			if (val.second == typedAddress.GetFlowControl()) {
				info.serialFlowControlInput->SetStringSelection(val.first);
				break;
			}
		}

	} else if (dynamic_cast<const UpnpEndpointAddress *>(&address)) {

		const UpnpEndpointAddress &typedAddress
			= *boost::polymorphic_downcast<const UpnpEndpointAddress *>(&address);

		info.SetType(ENDPOINT_TYPE_NETWORK);
		info.SetUpnpAdapter();

		if (typedAddress.GetExternalPort()) {
			info.portInput->SetValue(boost::lexical_cast<std::wstring>(typedAddress.GetExternalPort()));
		}

		if (dynamic_cast<const UpnpTcpEndpointAddress *>(&address)) {
			info.networkProtoInput->SetStringSelection(wxT("TCP"));
		} else {
			assert(dynamic_cast<const UpnpUdpEndpointAddress *>(&address));
			info.networkProtoInput->SetStringSelection(wxT("UDP"));
		}

	} else {

		const std::wstring resourceIdentifier(address.GetResourceIdentifier().GetCStr());
		const boost::wregex exp(L"([^:/]+)://(.+)");
		boost::wsmatch what;
		if (	boost::regex_match(resourceIdentifier, what, exp)
				&& !wxString(what[1].str()).CompareTo(wxT("pipe"), wxString::ignoreCase)) {
			info.SetType(ENDPOINT_TYPE_PIPE);
			info.pipeInput->SetValue(what[2].str());
		} else {
			assert(false);
			info.hostInput->Clear();
			info.portInput->Clear();
			info.pipeInput->Clear();
		}

	}

}

void EndpointDlg::CreateDefaultEndpointInfo(EndpointInfoItem &info) const {

	info.SetType(ENDPOINT_TYPE_NETWORK);
	info.SetAccepting(info.isReadOrCombined);
	info.networkProtoInput->SetStringSelection(wxT("TCP"));
	if (m_isFtpEndpoint) {
		info.portInput->SetValue(wxT("21"));
	}
	info.adapterInput->SetSelection(0); // zero is index of "all"
	info.EnableSsl(false);
	info.EnableProxy(false);
	info.serialLineInput->SetValue(wxT("COM1"));
	info.serialBaudRateInput->SetValue(wxT("9600"));
	info.serialDataBitsInput->SetValue(wxT("8"));
	info.serialStopBitsInput->SetValue(wxT("1"));
	info.serialParityInput->SetStringSelection(wxT("none"));
	info.serialFlowControlInput->SetStringSelection(wxT("XON/XOFF"));

	if (!m_isFtpEndpoint) {
		wxString key = wxT("/Rule/Template/Endpoint/Type/");
		key += m_isInputEndpoint ? wxT("Input") : wxT("Destination");
		wxString typeName = wxGetApp().GetConfig().Read(key);
		if (	typeName == wxT("TCP")
				|| typeName == wxT("UDP")
				|| typeName == wxT("UPnP")
				|| typeName == wxT("FTP")) {
			info.SetType(ENDPOINT_TYPE_NETWORK);
			if (typeName == wxT("FTP")) {
				typeName = wxT("TCP");
			}
			info.networkProtoInput->SetStringSelection(typeName);
		} else if (typeName == wxT("pipe")) {
			info.SetType(ENDPOINT_TYPE_PIPE);
		} else if (typeName == wxT("serial")) {
			info.SetType(ENDPOINT_TYPE_SERIAL);
		} else {
			assert(typeName.IsEmpty());
		}
	}

	if (m_isInputEndpoint && !m_isFtpEndpoint) {
		const wxString key = wxT("/Rule/Template/Endpoint/Acceptor");
		const wxString acceptor = wxGetApp().GetConfig().Read(key);
		if (acceptor == wxT("reader")) {
			info.SetAccepting(info.isReadOrCombined);
		} else if (acceptor == wxT("writer")) {
			info.SetAccepting(!info.isReadOrCombined);
		} else if (acceptor == wxT("none")) {
			info.SetAccepting(false);
		} else {
			assert(acceptor.IsEmpty());
		}
	}
	
}

void EndpointDlg::ReadEndpointInfo() {
	if (m_isNewEndpoint) {
		CreateDefaultEndpointInfo(m_endpointsInfo[0]);
		SetSplitEndpoint(false);
		CreateDefaultEndpointInfo(m_endpointsInfo[1]);
	} else if (m_endpoint.IsCombined() || m_isFtpEndpoint) {
		assert(m_endpoint.IsCombined());
		if (m_endpoint.IsCombined()) {
			ReadEndpointInfo(*m_endpoint.GetCombinedAddress(), m_endpointsInfo[0]);
			assert(
				!m_isFtpEndpoint
				|| !m_isInputEndpoint
				|| m_endpoint.IsCombinedAcceptor());
			m_endpointsInfo[0].SetAccepting(
				m_endpoint.IsCombinedAcceptor() || (m_isFtpEndpoint && m_isInputEndpoint));
		} else {
			CreateDefaultEndpointInfo(m_endpointsInfo[0]);
		}
		CreateDefaultEndpointInfo(m_endpointsInfo[1]);
		SetSplitEndpoint(false);
	} else {
		ReadEndpointInfo(*m_endpoint.GetReadAddress(), m_endpointsInfo[0]);
		m_endpointsInfo[0].SetAccepting(
			m_endpoint.GetReadWriteAcceptor() == Endpoint::ACCEPTOR_READER);
		SetSplitEndpoint(true);
		ReadEndpointInfo(*m_endpoint.GetWriteAddress(), m_endpointsInfo[1]);
		m_endpointsInfo[1].SetAccepting(
			m_endpoint.GetReadWriteAcceptor() == Endpoint::ACCEPTOR_WRITER);
	}
}

void EndpointDlg::OnProxySettings(wxCommandEvent &) {
	ShowProxySettingsDialog(true);
}

void EndpointDlg::OnProxySettingsWrite(wxCommandEvent &) {
	ShowProxySettingsDialog(false);
}

bool EndpointDlg::ShowProxySettingsDialog(bool readEndpointAction) {
	return ShowProxySettingsDialog(m_endpointsInfo[!readEndpointAction ? 1 : 0]);
}

bool EndpointDlg::ShowProxySettingsDialog(EndpointInfoItem &info) {
	return RuleUtils::ShowProxySettingsDialog(
		m_service,
		this,
		m_licenses->proxy,
		m_licenses->proxyCascade,
		info.proxyCascade);
}

bool EndpointDlg::ShowSslSettingsDialog(bool readEndpointAction) {
	return ShowSslSettingsDialog(m_endpointsInfo[!readEndpointAction ? 1 : 0]);
}

bool EndpointDlg::ShowSslSettingsDialog(EndpointInfoItem &info) {
	return RuleUtils::ShowSslSettingsDialog(
		m_service,
		this,
		info.IsAccepting(),
		m_licenses->ssl,
		info.certificate,
		info.remoteCertificates);
}

void EndpointDlg::OnProxyUseToggle(wxCommandEvent &event) {
	CheckProxyUseControls(
		true,
		boost::polymorphic_downcast<wxCheckBox *>(event.GetEventObject())->GetValue());
}

void EndpointDlg::OnProxyUseWriteToggle(wxCommandEvent &event) {
	CheckProxyUseControls(
		false,
		boost::polymorphic_downcast<wxCheckBox *>(event.GetEventObject())->GetValue());
}

void EndpointDlg::CheckSslUseControls(
			bool readEndpointAction,
			bool isSet) {
	EndpointInfoItem &info = m_endpointsInfo[!readEndpointAction ? 1 : 0];
	if (isSet && !m_licenses->ssl.IsFeatureAvailable(true)) {
		LicenseRestrictionDlg(m_service, this, m_licenses->ssl, true).ShowModal();
	}
	if (	isSet
			&& !m_licenses->ssl.IsFeatureValueAvailable(true)
			&& !wxGetApp().IsUnlimitedModeActive()) {
		isSet = false;
	}
	if (	isSet
			&& !m_licenses->ssl.IsFeatureValueAvailable(true)
			&& !wxGetApp().IsUnlimitedModeActive()) {
		return;
	}
	if (isSet && info.certificate.IsEmpty() == 0) {
		isSet = ShowSslSettingsDialog(info);
	}
	info.EnableSsl(isSet);
}

void EndpointDlg::CheckProxyUseControls(
			bool readEndpointAction,
			bool isSet) {
	EndpointInfoItem &info = m_endpointsInfo[!readEndpointAction ? 1 : 0];
	if (isSet && !m_licenses->proxy.IsFeatureAvailable(true)) {
		LicenseRestrictionDlg(m_service, this, m_licenses->proxy, true).ShowModal();
	}
	if (	isSet
			&& !m_licenses->proxy.IsFeatureValueAvailable(true)
			&& !wxGetApp().IsUnlimitedModeActive()) {
		isSet = false;
	}
	if (	isSet
			&& !m_licenses->proxy.IsFeatureValueAvailable(true)
			&& !wxGetApp().IsUnlimitedModeActive()) {
		return;
	}
	if (isSet && !info.pathfinderUseInput->GetValue()) {
		RuleUtils::ShowWarnAboutFtpOverHttp(
			this,
			wxT("Proxy"),
			m_isFtpEndpoint);
	}
	if (isSet && info.proxyCascade.size() == 0) {
		isSet = ShowProxySettingsDialog(info);
	}
	info.EnableProxy(isSet);
}

void EndpointDlg::OnSslUseToggle(wxCommandEvent &event) {
	CheckSslUseControls(
		true,
		boost::polymorphic_downcast<wxCheckBox *>(event.GetEventObject())->GetValue());
}

void EndpointDlg::OnSslUseWriteToggle(wxCommandEvent &event) {
	CheckSslUseControls(
		false,
		boost::polymorphic_downcast<wxCheckBox *>(event.GetEventObject())->GetValue());
}

void EndpointDlg::OnSslSettings(wxCommandEvent &) {
	ShowSslSettingsDialog(true);
}

void EndpointDlg::OnSslSettingsWrite(wxCommandEvent &) {
	ShowSslSettingsDialog(false);
}

void EndpointDlg::OnPathfinderUseToggle(wxCommandEvent &event) {
	CheckPathfinderUseControls(
		true,
		boost::polymorphic_downcast<wxCheckBox *>(event.GetEventObject())->GetValue());
}

void EndpointDlg::OnIsPathfinderUseWriteToggle(wxCommandEvent &event) {
	CheckPathfinderUseControls(
		false,
		boost::polymorphic_downcast<wxCheckBox *>(event.GetEventObject())->GetValue());
}

void EndpointDlg::CheckPathfinderUseControls(
			bool readEndpointAction,
			bool isSet) {
	EndpointInfoItem &info = m_endpointsInfo[!readEndpointAction ? 1 : 0];
	if (isSet && !m_licenses->proxy.IsFeatureAvailable(true)) {
		LicenseRestrictionDlg(m_service, this, m_licenses->proxy, true).ShowModal();
		if (!wxGetApp().IsUnlimitedModeActive()) {
			info.EnablePathfinder(false);
			return;
		}
	}
	if (isSet && !info.proxyUseInput->GetValue()) {
		RuleUtils::ShowWarnAboutFtpOverHttp(
			this,
			wxT("Pathfinder"),
			m_isFtpEndpoint);
	}
	info.EnablePathfinder(isSet);
}

void EndpointDlg::OnAcceptingToggle(wxCommandEvent &) {
	CheckAcceptingControls(true);
}

void EndpointDlg::OnAcceptingWriteToggle(wxCommandEvent &) {
	CheckAcceptingControls(false);
}

void EndpointDlg::CheckAcceptingControls(bool readEndpointAction) {
	const size_t idx = !readEndpointAction ? 1 : 0;
	EndpointInfoItem &info = m_endpointsInfo[idx];
	const wxChoice &ctrl
		=	info.GetType() == ENDPOINT_TYPE_NETWORK
			&& info.networkProtoInput->GetStringSelection() == wxT("UDP")
		?	*info.isAcceptingUdp
		:	*info.isAcceptingInput;
	wxChoice &oppCtrl = &ctrl == info.isAcceptingInput
		?	*info.isAcceptingUdp
		:	*info.isAcceptingInput;
	oppCtrl.SetSelection(ctrl.GetCurrentSelection());
	if (info.IsAccepting()) {
		const size_t oppIdx = !readEndpointAction ? 0 : 1;
		m_endpointsInfo[oppIdx].SetAccepting(false);
		m_endpointsInfo[idx].ResetProxy();
	}
	ShowControls();
}

void EndpointDlg::ShowSplitRwLicenseRestriction() const {
	if (!IsSplitEndpoint() || m_licenses->rwSplit.IsFeatureAvailable(true)) {
		return;
	}
	LicenseRestrictionDlg(
			const_cast<EndpointDlg *>(this)->m_service,
			const_cast<EndpointDlg *>(this),
			m_licenses->rwSplit,
			false)
		.ShowModal();
}

void EndpointDlg::OnNetworkAdapterChange(wxCommandEvent &event) {
	wxChoice &ctrl = *boost::polymorphic_downcast<wxChoice *>(event.GetEventObject());
	ctrl.SetToolTip(ctrl.GetStringSelection());
	if (!RuleUtils::IsUpnpAdapterSelected(ctrl)) {
		std::list<texs__NetworkAdapterInfo>::const_iterator adapter
			= m_serviceNetworkAdapters.begin();
		advance(adapter, ctrl.GetCurrentSelection());
		CheckInactiveAdapterWarning(*adapter);
	}
	ShowControls();
}

bool EndpointDlg::CheckInactiveAdapterWarning(
			const texs__NetworkAdapterInfo &adapterInfo)
		const {
	if (!adapterInfo.ipAddress.empty()) {
		return false;
	}
	wxLogWarning(
		wxT("Selected inactive network adapter (IP address not assigned).")
		wxT(" If adapter still be inactive at rule enabling - error will occur for this endpoint."));
	return true;
}

bool EndpointDlg::CheckInactiveAdapterWarning(const Endpoint &endpoint) const {
	if (endpoint.IsCombined()) {
		if (endpoint.CheckCombinedAddressType<InetEndpointAddress>()) {
			if (CheckInactiveAdapterWarning(
					endpoint.GetCombinedTypedAddress<InetEndpointAddress>())) {
				return true;
			}
		}
	} else {
		if (endpoint.CheckReadAddressType<InetEndpointAddress>()) {
			if (CheckInactiveAdapterWarning(
					endpoint.GetReadTypedAddress<InetEndpointAddress>())) {
				return true;
			}
		}
		if (endpoint.CheckWriteAddressType<InetEndpointAddress>()) {
			if (CheckInactiveAdapterWarning(
					endpoint.GetWriteTypedAddress<InetEndpointAddress>())) {
				return true;
			}
		}
	}
	return false;
}

bool EndpointDlg::CheckInactiveAdapterWarning(
			const InetEndpointAddress &addr)
		const {
	const std::string id = wxString(addr.GetAdapter()).ToAscii();
	if (id.empty()) {
		return false;
	}
	foreach (const texs__NetworkAdapterInfo &info, m_serviceNetworkAdapters) {
		if (info.id == id) {
			if (CheckInactiveAdapterWarning(info)) {
				return true;
			}
		}
	}
	return false;
}

bool EndpointDlg::IsSplitEndpoint() const {
	return !m_isFtpEndpoint && m_readWriteInput->GetCurrentSelection() != 0;
}

void EndpointDlg::SetSplitEndpoint(bool flag) {
	m_readWriteInput->SetSelection(flag ? 1 : 0);
}
