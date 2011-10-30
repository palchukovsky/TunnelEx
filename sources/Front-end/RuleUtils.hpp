/**************************************************************************
 *   Created: 2010/09/18 18:02
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__EndpointDlgUtils_hpp__1009181802
#define INCLUDED_FILE__TUNNELEX__EndpointDlgUtils_hpp__1009181802

#include "ProxyDlg.hpp"
#include "LicensePolicies.hpp"
#include "Modules/Inet/InetEndpointAddress.hpp"

class ServiceWindow;

struct RuleUtils {

	class ListenerFinder : private boost::noncopyable {
	public:
		ListenerFinder(const TunnelEx::WString &listenerName);
		bool Find(const TunnelEx::RuleEndpoint::Listeners &) const;
		void Add(TunnelEx::RuleEndpoint::Listeners &) const;
		void Remove(TunnelEx::RuleEndpoint::Listeners &) const;
		bool Find(const TunnelEx::RuleEndpointCollection &) const;
		void Add(TunnelEx::RuleEndpointCollection &) const;
		void Remove(TunnelEx::RuleEndpointCollection &) const;
		bool Find(const TunnelEx::TunnelRule &) const;
	private:
		const TunnelEx::WString m_listenerName;
	};

	static wxChoice & CreateAdapterSelector(
				const std::list<texs__NetworkAdapterInfo> &serviceNetworkAdapters,
				wxWindow *const parent = 0,
				const wxWindowID id = wxID_ANY,
				const bool isFtpEndpoint = false,
				const wxPoint &position = wxDefaultPosition,
				wxSize size = wxDefaultSize);
	static void SelectAdapter(
				wxChoice &ctrl,
				const wxString &newAdapter,
				const std::list<texs__NetworkAdapterInfo> &adapters);
	static bool IsUpnpAdapterSelected(const wxChoice &ctrl) {
		return ctrl.GetCurrentSelection() + 1 == int(ctrl.GetCount());
	}
	static void SelectUpnpAdapter(wxChoice &ctrl) {
		assert(ctrl.GetCount() > 0);
		if (ctrl.GetCount() < 1) {
			return;
		}
		// UPnP last in the list
		ctrl.SetSelection(ctrl.GetCount() - 1);
	}

	static wxSize GetPortFieldSize() {
		return wxSize(60, -1);
	}

	static wxChoice & CreateSerialConfParity(
				wxWindow *const parent,
				const wxWindowID id = wxID_ANY,
				const wxPoint &position = wxDefaultPosition,
				const wxSize &size = wxDefaultSize);

	static wxChoice & CreateSerialConfFlowControl(
				wxWindow *const parent,
				const wxWindowID id = wxID_ANY,
				const wxPoint &position = wxDefaultPosition,
				const wxSize &size = wxDefaultSize);

	static void ShowWarnAboutFtpOverHttp(
				wxWindow *const parent,
				const wxString &varName,
				bool isFtpEndpoint);

	static bool ShowProxySettingsDialog(
			ServiceWindow &,
			wxWindow *const parent,
			const TunnelEx::Licensing::ProxyLicense &proxyLicense,
			const TunnelEx::Licensing::ProxyCascadeLicense &proxyCascadeLicense,
			ProxyCascadeDlg::Cascade &proxyCascade);

	static bool ShowSslSettingsDialog(
			ServiceWindow &,
			wxWindow *const parent,
			bool isServer,
			const TunnelEx::Licensing::SslLicense &license,
			TunnelEx::SslCertificateId &certificate,
			TunnelEx::SslCertificateIdCollection &remoteCertificates);

	template<class T>
	static void GetSerialFlowControlValsMap(T &result) {
		T tmp;
		tmp.insert(std::make_pair(L"none", SerialEndpointAddress::FC_NONE));
		tmp.insert(std::make_pair(L"XON/XOFF", SerialEndpointAddress::FC_XON_XOFF));
		tmp.insert(std::make_pair(L"RTS/CTS", SerialEndpointAddress::FC_RTS_CTS));
		tmp.insert(std::make_pair(L"DSR/DTR", SerialEndpointAddress::FC_DSR_DTR));
		tmp.swap(result);
	}

	template<class T>
	static void GetSerialParityValsMap(T &result) {
		T tmp;
		tmp.insert(std::make_pair(L"none", SerialEndpointAddress::P_NONE));
		tmp.insert(std::make_pair(L"odd", SerialEndpointAddress::P_ODD));
		tmp.insert(std::make_pair(L"even", SerialEndpointAddress::P_EVEN));
		tmp.insert(std::make_pair(L"mark", SerialEndpointAddress::P_MARK));
		tmp.insert(std::make_pair(L"space", SerialEndpointAddress::P_SPACE));
		tmp.swap(result);
	}

	static bool IsFtpTunnelIsOnInRule(const TunnelEx::TunnelRule &);
	static void SaveFtpInRule(const bool isOn, TunnelEx::TunnelRule &);

	static bool IsSortDestinationsByPingIsOnInRule(const TunnelEx::TunnelRule &);

	static wxString GetEndpoindLogPath(const TunnelEx::RuleEndpoint &);

	static void CheckInactiveAdapterWarning(
			const TunnelEx::TunnelRule &,
			const ServiceAdapter &);
	static bool CheckInactiveAdapterWarning(const texs__NetworkAdapterInfo &);
	static bool CheckInactiveAdapterWarning(
			const TunnelEx::Mods::Inet::InetEndpointAddress &,
			const ServiceAdapter &);
	static bool CheckInactiveAdapterWarning(
			const TunnelEx::Endpoint &,
			const ServiceAdapter &);
	static bool CheckInactiveAdapterWarning(
			const TunnelEx::RuleEndpointCollection &,
			const ServiceAdapter &);

	static TunnelEx::WString CreateSerialResourceIdentifier(
			const wxString &serialLine,
			const wxString &baudRate,
			const wxString &dataBits,
			const wxString &stopBit,
			const wxString &parityStr,
			const wxString &flowControl);

	static unsigned short ConvertPort(const wxString &);

	static bool SlitAddressFromClipboard(
			wxTextCtrl &host,
			wxTextCtrl &port,
			bool isPortChanged);

};

#endif // INCLUDED_FILE__TUNNELEX__EndpointDlgUtils_hpp__1009181802
