/**************************************************************************
 *   Created: 2010/09/21 20:25
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "RuleUtils.hpp"
#include "SslEndpointSettingsDlg.hpp"
#include "Application.hpp"
#include "LicenseRestrictionDlg.hpp"
#include "Modules/Serial/SerialEndpointAddress.hpp"

using namespace TunnelEx;
using namespace TunnelEx::Licensing;
using namespace TunnelEx::Mods::Inet;
using namespace TunnelEx::Mods::Serial;

//////////////////////////////////////////////////////////////////////////

RuleUtils::ListenerFinder::ListenerFinder(
			const WString &listenerName)
		: m_listenerName(listenerName) {
	//...//
}

bool RuleUtils::ListenerFinder::Find(const RuleEndpoint::Listeners &listeners) const {
	const size_t count = listeners.GetSize();
	WString buffer;
	for (size_t i = 0; i < count; ++i) {
		listeners[i].name.SubStr(buffer, 0, m_listenerName.GetLength());
		if (buffer == m_listenerName) {
			return true;
		}
	}
	return false;
}

void RuleUtils::ListenerFinder::Add(RuleEndpoint::Listeners &listeners) const {
	const size_t count = listeners.GetSize();
	for (size_t i = 0; i < count; ++i) {
		if (listeners[i].name == m_listenerName) {
			return;
		}
	}
	RuleEndpoint::ListenerInfo newListener;
	newListener.name = m_listenerName;
	listeners.Append(newListener);
}

void RuleUtils::ListenerFinder::Remove(RuleEndpoint::Listeners &listeners) const {
	for (size_t i = 0; i < listeners.GetSize(); ) {
		if (listeners[i].name == m_listenerName) {
			listeners.Remove(i);
		} else {
			++i;
		}
	}
}

bool RuleUtils::ListenerFinder::Find(const RuleEndpointCollection &endpoints) const {
	const size_t count = endpoints.GetSize();
	for (size_t i = 0; i < count; ++i) {
		if (Find(endpoints[i].GetPreListeners())) {
			return true;
		}
	}
	return false;
}

void RuleUtils::ListenerFinder::Add(RuleEndpointCollection &endpoints) const {
	const size_t count = endpoints.GetSize();
	for (size_t i = 0; i < count; ++i) {
		Add(endpoints[i].GetPreListeners());
	}
}

void RuleUtils::ListenerFinder::Remove(RuleEndpointCollection &endpoints) const {
	const size_t count = endpoints.GetSize();
	for (size_t i = 0; i < count; ++i) {
		Remove(endpoints[i].GetPreListeners());
	}
}

bool RuleUtils::ListenerFinder::Find(const TunnelRule &rule) const {
	return	Find(rule.GetInputs())
		||	Find(rule.GetDestinations());
}

//////////////////////////////////////////////////////////////////////////

void RuleUtils::SelectAdapter(
			wxChoice &ctrl,
			const wxString &newAdapter,
			const std::list<texs__NetworkAdapterInfo> &adapters) {
	if (!adapters.size() || newAdapter.IsEmpty()) {
		return;
	}
	size_t selectedIndex = 0;
	const std::string newAdapterA = newAdapter.ToAscii();
	foreach (const texs__NetworkAdapterInfo &info, adapters) {
		if (newAdapterA == info.id) {
			break;
		}
		++selectedIndex;
	}
	if (selectedIndex >= adapters.size()) {
		if (!newAdapter.IsEmpty()) {
			wxLogError(
				wxT("Could not find selected adapter.")
				wxT(" Current adapters has been set to \"Loopback\"."));
			// 1 - index of loopback
			selectedIndex = std::min(size_t(1), ctrl.GetCount());
		} else {
			selectedIndex = 0;
		}
	}
	ctrl.SetSelection(int(selectedIndex));
}

wxChoice & RuleUtils::CreateAdapterSelector(
			const std::list<texs__NetworkAdapterInfo> &serviceNetworkAdapters,
			wxWindow *const parent /*= 0*/,
			const wxWindowID id /*= wxID_ANY*/,
			const bool isFtpEndpoint /*false*/,
			const wxPoint &position /*= wxDefaultPosition*/,
			wxSize size /*= wxDefaultSize*/) {
	
	// see GetAdapterUpnpSelecteionIndex and SetUpnpAdapter before add items!

	wxArrayString adapters;
	foreach (const texs__NetworkAdapterInfo &adapterInfo, serviceNetworkAdapters) {
		wxString adapter;
		if (!adapterInfo.ipAddress.empty() && adapterInfo.ipAddress != "*") {
			adapter += wxString::FromAscii(adapterInfo.ipAddress.c_str());
			if (!adapterInfo.name.empty()) {
				adapter += wxT(": ");
			}
		}
		adapter += wxString::FromAscii(adapterInfo.name.c_str());
		adapters.Add(adapter);
	}
	
	assert(adapters.GetCount() >= 2);
	if (!isFtpEndpoint) {
		adapters.Add(wxT("UPnP Router Port Mapping"));
	}
	
	if (size == wxDefaultSize) {
		size = wxSize(200, -1);
	}

	return *new wxChoice(
		parent,
		id,
		position,
		size,
		adapters,
		wxTE_NOHIDESEL | wxTE_PROCESS_ENTER);

}

wxChoice & RuleUtils::CreateSerialConfParity(
			wxWindow *const parent,
			const wxWindowID id /*= wxID_ANY*/,
			const wxPoint &position /*= wxDefaultPosition*/,
			const wxSize &size /*= wxDefaultSize*/) {
	wxArrayString list;
	list.Add(wxT("none"));
	list.Add(wxT("odd"));
	list.Add(wxT("even"));
	list.Add(wxT("mark"));
	list.Add(wxT("space"));
	wxChoice &result = *new wxChoice(
		parent,
		id,
		position,
		size,
		list,
		wxTE_NOHIDESEL | wxTE_PROCESS_ENTER);
	result.SetSelection(0);
	return result;
}

wxChoice & RuleUtils::CreateSerialConfFlowControl(
			wxWindow *const parent,
			const wxWindowID id /*= wxID_ANY*/,
			const wxPoint &position /*= wxDefaultPosition*/,
			const wxSize &size /*= wxDefaultSize*/) {
	wxArrayString list;
	list.Add(wxT("none"));
	list.Add(wxT("XON/XOFF"));
	list.Add(wxT("RTS/CTS"));
	list.Add(wxT("DSR/DTR"));
	wxChoice &result = *new wxChoice(
		parent,
		id,
		position,
		size,
		list,
		wxTE_NOHIDESEL | wxTE_PROCESS_ENTER);
	result.SetSelection(0);
	return result;
}

void RuleUtils::ShowWarnAboutFtpOverHttp(
			wxWindow *const parent,
			const wxString &varName,
			bool isFtpEndpoint) {

	if (!isFtpEndpoint) {
		return;
	}

	const wxString configVarFullName = wxT("/Show/Endpoint/FtpOverHttp/") + varName;

	bool showThisDialog;
	wxGetApp().GetConfig().Read(configVarFullName, &showThisDialog, true);
	if (!showThisDialog) {
		return;
	}

	const int border = 10;
	const wxSize buttonSize(75, 25);

	const wxString message
		= wxT("You are trying to create FTP tunnel over HTTP tunnel.")
			wxT(" This feature allows using FTP only in passive mode for transfers.")
			wxT(" Please configure your FTP client to work in passive mode")
			wxT(" before use it.");

	wxDialog dlg(
		parent,
		wxID_ANY,
		wxT("FTP tunnel over HTTP tunnel"),
		wxDefaultPosition,
		wxDefaultSize,
		wxCAPTION);
	wxStaticText &text = *(new wxStaticText(&dlg, wxID_ANY, message));
	wxCheckBox &checkBox = *(new wxCheckBox(
		&dlg, wxID_ANY, wxT("Do not show this message in the future")));

	wxFont checkBoxFont(checkBox.GetFont());
	checkBoxFont.SetPointSize(7);
	checkBox.SetFont(checkBoxFont);
	text.Wrap(
		checkBox.GetSize().GetWidth()
			+ buttonSize.GetWidth() * 2
			+ border * 4);

	dlg.SetClientSize(
		wxSize(
			text.GetBestSize().GetWidth() + border * 2,
			text.GetBestSize().GetHeight()
				+ border * 4
				+ buttonSize.GetHeight()
				+ checkBox.GetSize().GetHeight()));

	text.SetPosition(wxPoint(border, border));
	text.SetSize(text.GetBestSize());
	checkBox.SetPosition(
		wxPoint(
			border,
			dlg.GetClientSize().GetHeight()
				- border
				- buttonSize.GetHeight()
				+ checkBox.GetSize().GetHeight() / 2));

	new wxButton(
		&dlg,
		wxID_CANCEL,
		wxT("OK"),
		wxPoint(
			dlg.GetClientSize().GetWidth() - border - buttonSize.GetWidth(),
			dlg.GetClientSize().GetHeight() - border - buttonSize.GetHeight()),
		buttonSize);

	dlg.ShowModal();
	wxGetApp().GetConfig().Write(configVarFullName, !checkBox.IsChecked());

}

bool RuleUtils::ShowProxySettingsDialog(
			ServiceWindow &service,
			wxWindow *const parent,
			const ProxyLicense &proxyLicense,
			const ProxyCascadeLicense &proxyCascadeLicense,
			ProxyCascadeDlg::Cascade &proxyCascade) {

	ProxyCascadeDlg::Cascade &proxyCascadeTmp(proxyCascade);

	if (proxyCascadeTmp.size() < 2) {

		if (!proxyLicense.IsFeatureAvailable(true)) {
			LicenseRestrictionDlg(service, parent, proxyLicense, false).ShowModal();
		}
	
		std::auto_ptr<ProxyDlg> dlg(proxyCascadeTmp.size() == 0
			?	new ProxyDlg(
					parent,
					false,
					!proxyLicense.IsFeatureAvailable(true)
						&& !wxGetApp().IsUnlimitedModeActive())
			:	new ProxyDlg(
					parent,
					*proxyCascadeTmp.begin(),
					false,
					!proxyLicense.IsFeatureAvailable(true)
						&& !wxGetApp().IsUnlimitedModeActive()));
		if (dlg->ShowModal() == wxID_OK) {
			if (proxyCascadeTmp.size() == 0) {
				proxyCascadeTmp.push_back(dlg->GetProxy());
			} else {
				*proxyCascadeTmp.begin() = dlg->GetProxy();
			}
		}

		if (dlg->IsInCascade()) {
			ProxyCascadeDlg dlg(service, parent, proxyCascadeTmp, proxyCascadeLicense);
			if (dlg.ShowModal() == wxID_OK) {
				ProxyCascadeDlg::Cascade(dlg.GetCascade()).swap(proxyCascadeTmp);
			}
		}

	} else {

		ProxyCascadeDlg dlg(service, parent, proxyCascadeTmp, proxyCascadeLicense);
		if (dlg.ShowModal() == wxID_OK) {
			ProxyCascadeDlg::Cascade(dlg.GetCascade()).swap(proxyCascadeTmp);
		}

	}

	const bool isAnyProxySet = proxyCascadeTmp.size() > 0;
	proxyCascadeTmp.swap(proxyCascade);
	return isAnyProxySet;

}

bool RuleUtils::ShowSslSettingsDialog(
			ServiceWindow &service,
			wxWindow *const parent,
			bool isServer,
			const Licensing::SslLicense &license,
			SslCertificateId &certificate,
			SslCertificateIdCollection &remoteCertificates) {

	bool readonly = false;
	if (!license.IsFeatureAvailable(true)) {
		LicenseRestrictionDlg(service, parent, license, false).ShowModal();
		readonly = !wxGetApp().IsUnlimitedModeActive();
	}

	SslEndpointSettingsDlg settings(
		isServer,
		certificate,
		remoteCertificates,
		parent,
		service,
		readonly);
	if (settings.ShowModal() == wxID_OK) {
		assert(
			!settings.GetCertificate().IsEmpty()
			|| settings.GetRemoteCertificates().GetSize() == 0);
		certificate = settings.GetCertificate();
		remoteCertificates = settings.GetRemoteCertificates();
	}

	return !certificate.IsEmpty();

}

bool RuleUtils::IsFtpTunnelIsOnInRule(const TunnelRule &rule) {
	return ListenerFinder(L"Tunnel/Ftp/").Find(rule);
}

bool RuleUtils::IsSortDestinationsByPingIsOnInRule(const TunnelRule &rule) {
	const TunnelRule::Filters &filters = rule.GetFilters();
	const size_t filtersSize = filters.GetSize();
	for (size_t i = 0; i < filtersSize; ++i) {
		if (filters[i] == L"DestinationsSorter/Ping") {
			return true;
		}
	}
	return false;
}

wxString RuleUtils::GetEndpoindLogPath(const RuleEndpoint &endpoint) {
	const RuleEndpoint::Listeners &listeners = endpoint.GetPreListeners();
	const size_t listenersNumb = listeners.GetSize();
	for (size_t i = 0; i < listenersNumb; ++i) {
		if (listeners[i].name == L"TrafficLogger/File") {
			return listeners[i].param.GetCStr();
		}
	}
	return wxEmptyString;
}

void RuleUtils::CheckInactiveAdapterWarning(
			const TunnelRule &rule,
			const ServiceAdapter &service) {
	!CheckInactiveAdapterWarning(rule.GetInputs(), service)
		&& CheckInactiveAdapterWarning(rule.GetDestinations(), service);
}

bool RuleUtils::CheckInactiveAdapterWarning(
			const texs__NetworkAdapterInfo &adapterInfo) {
	if (!adapterInfo.ipAddress.empty()) {
		return false;
	}
	wxLogWarning(
		wxT("Some endpoints using inactive network adapter (IP address not assigned).")
			wxT(" If adapter still be inactive at rule enabling - error will occur for this endpoint."));
	return true;
}

bool RuleUtils::CheckInactiveAdapterWarning(
			const InetEndpointAddress &addr,
			const ServiceAdapter &service) {
	const std::string id = wxString(addr.GetAdapter()).ToAscii();
	if (id.empty()) {
		return false;
	}
	std::list<texs__NetworkAdapterInfo> serviceNetworkAdapters;
	service.GetNetworkAdapters(false, serviceNetworkAdapters);
	foreach (const texs__NetworkAdapterInfo &info, serviceNetworkAdapters) {
		if (info.id == id) {
			if (CheckInactiveAdapterWarning(info)) {
				return true;
			}
		}
	}
	return false;
}

bool RuleUtils::CheckInactiveAdapterWarning(
			const Endpoint &endpoint,
			const ServiceAdapter &service) {
	if (endpoint.IsCombined()) {
		if (endpoint.CheckCombinedAddressType<InetEndpointAddress>()) {
			return CheckInactiveAdapterWarning(
				endpoint.GetCombinedTypedAddress<InetEndpointAddress>(),
				service);
		}
	} else {
		if (endpoint.CheckReadAddressType<InetEndpointAddress>()) {
			if (	CheckInactiveAdapterWarning(
						endpoint.GetReadTypedAddress<InetEndpointAddress>(),
						service)) {
				return true;
			}
		}
		if (endpoint.CheckWriteAddressType<InetEndpointAddress>()) {
			if (	CheckInactiveAdapterWarning(
						endpoint.GetWriteTypedAddress<InetEndpointAddress>(),
						service)) {
				return true;
			}
		}
	}
	return false;
}

bool RuleUtils::CheckInactiveAdapterWarning(
			const RuleEndpointCollection &endpoints,
			const ServiceAdapter &service) {
	const size_t size = endpoints.GetSize();
	for (size_t i = 0; i < size; ++i) {
		if (CheckInactiveAdapterWarning(endpoints[i], service)) {
			return true;
		}
	}
	return false;
}

void RuleUtils::SaveFtpInRule(const bool isOn, TunnelRule &rule) {
	assert(rule.GetInputs().GetSize() > 0);
	assert(rule.GetDestinations().GetSize() > 0);
	RuleUtils::ListenerFinder ftpActiveFinder(L"Tunnel/Ftp/Active");
	RuleUtils::ListenerFinder ftpPassiveFinder(L"Tunnel/Ftp/Passive");
	ftpPassiveFinder.Remove(rule.GetInputs());
	ftpActiveFinder.Remove(rule.GetDestinations());
	if (isOn) {
		ftpPassiveFinder.Add(rule.GetInputs());
		ftpActiveFinder.Add(rule.GetDestinations());
	}
}

WString RuleUtils::CreateSerialResourceIdentifier(
			const wxString &serialLine,
			const wxString &baudRateStr,
			const wxString &dataBitsStr,
			const wxString &stopBitsStr,
			const wxString &parityStr,
			const wxString &flowControlStr) {
	
	int baudRate;
	std::wistringstream(baudRateStr.c_str()) >> baudRate;
		
	unsigned short dataBits;
	std::wistringstream(dataBitsStr.c_str()) >> dataBits;

	unsigned short stopBits;
	std::wistringstream(stopBitsStr.c_str()) >> stopBits;

	typedef std::map<wxString, SerialEndpointAddress::Parity> ParityMap;
	ParityMap parityMap;
	GetSerialParityValsMap(parityMap);
	assert(parityMap.find(parityStr) != parityMap.end());

	typedef std::map<wxString, SerialEndpointAddress::FlowControl> FcMap;
	FcMap fcMap;
	GetSerialFlowControlValsMap(fcMap);
	assert(fcMap.find(flowControlStr) != fcMap.end());
		
	return SerialEndpointAddress::CreateResourceIdentifier(
		serialLine.c_str(),
		baudRate,
		dataBits,
		stopBits,
		parityMap[parityStr],
		fcMap[flowControlStr]);

}

unsigned short RuleUtils::ConvertPort(const wxString &port) {
	try {
		std::wstring portStr = port;
		boost::trim(portStr);
		return boost::lexical_cast<unsigned short>(portStr);
	} catch (const boost::bad_lexical_cast &) {
		const unsigned int portNum = std::numeric_limits<unsigned short>::max();
		WFormat message(L"Network port value truncated from \"%1%\" to \"%2%\".");
		message % port.c_str() % portNum;
		wxLogWarning(message.str().c_str());
		return portNum;
	}
}

bool RuleUtils::SlitAddressFromClipboard(wxTextCtrl &host, wxTextCtrl &port, bool isPortChanged) {
	
	if (!port.GetValue().IsEmpty() && isPortChanged) {
		return false;
	}
	
	if (!wxTheClipboard->Open()) {
		assert(false);
		return false;
	}

	bool isSet = false;
	try {
		if (wxTheClipboard->IsSupported(wxDF_TEXT)) {
			wxTextDataObject clipboardData;
			wxTheClipboard->GetData(clipboardData);
			std::wstring value = clipboardData.GetText();
			boost::trim(value);
			boost::wsmatch what;
			if (boost::regex_match(value, what, boost::wregex(L"^([^:]+):(\\d+)$"))) {
				host.SetValue(what[1].str());
				port.SetValue(what[2].str());
				isSet = true;
			}
		}
	} catch (...) {
		wxTheClipboard->Close();
		throw;
	}
	wxTheClipboard->Close();

	return isSet;

}
