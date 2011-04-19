/**************************************************************************
 *   Created: 2008/01/27 5:40
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__EndpointDlg_h__0801270540
#define INCLUDED_FILE__TUNNELEX__EndpointDlg_h__0801270540

#include "ProxyDlg.hpp"
#include "ServiceAdapter.hpp"
#include "Modules/Inet/InetEndpointAddress.hpp"

class ServiceWindow;

class EndpointDlg : public wxDialog {

	template<class T, std::size_t N>
	friend class boost::array;

protected:

	enum ControlId {
		CONTROL_ID_LOGTRAFFIC_TOGGLE,
		CONTROL_ID_LOGTRAFFIC_LABEL,
		CONTROL_ID_LOGTRAFFIC_FOLDER,
		CONTROL_ID_LOGTRAFFIC_FOLDER_SELECT,
		CONTROL_ID_READ_WRITE_TYPE,
		CONTROL_ID_TYPE,
		CONTROL_ID_TYPE_WRITE,
		CONTROL_ID_NETWORK_PROTOCOL,
		CONTROL_ID_NETWORK_PROTOCOL_WRITE,
		CONTROL_ID_NETWORK_ADAPTER,
		CONTROL_ID_NETWORK_ADAPTER_WRITE,
		CONTROL_ID_PORT,
		CONTROL_ID_PORT_WRITE,
		CONTROL_ID_ACCEPTING,
		CONTROL_ID_ACCEPTING_WRITE,
		CONTROL_ID_ACCEPTING_UDP,
		CONTROL_ID_ACCEPTING_UDP_WRITE,
		CONTROL_ID_PROXY_USE,
		CONTROL_ID_PROXY_USE_WRITE,
		CONTROL_ID_PROXY_SETTINGS,
		CONTROL_ID_PROXY_SETTINGS_WRITE,
		CONTROL_ID_PATHFINDER_USE,
		CONTROL_ID_PATHFINDER_USE_WRITE,
		CONTROL_ID_SSL_USE,
		CONTROL_ID_SSL_USE_WRITE,
		CONTROL_ID_SSL_SETTINGS,
		CONTROL_ID_SSL_SETTINGS_WRITE
	};

	enum EndpointType {
		ENDPOINT_TYPE_NETWORK = 0,
		ENDPOINT_TYPE_PIPE,
		ENDPOINT_TYPE_SERIAL
	};

	struct LogInfo {
		bool isOn;
		wxString path;
	};

	struct EndpointInfoItem {

		bool isReadOrCombined;

		wxStaticBox *group;

		wxChoice *networkProtoInput;
		wxStaticText *networkProtoLabel;

		wxStaticText *adapterLabel;
		wxChoice *adapterInput;
		void SetAdapter(
				const wxString &newAdapter,
				const std::list<texs__NetworkAdapterInfo> &);
		void SetUpnpAdapter();

		wxStaticText *hostLabel;
		wxTextCtrl *hostInput;

		wxStaticText *externalIpLabel;
		wxTextCtrl *externalIpInput;

		wxStaticText *portLabel;
		wxTextCtrl *portInput;
		std::wstring portValid;

		wxStaticText *pipeLabel;
		wxTextCtrl *pipeInput;

		wxStaticText *typeLabel;
		wxChoice *typeInput;
		EndpointType GetType() const;
		void SetType(EndpointType type);

		wxStaticText *isAcceptingLabel;
		wxChoice *isAcceptingInput;
		wxChoice *isAcceptingUdp;
		bool IsAccepting() const;
		void SetAccepting(bool flag);

		wxCheckBox *proxyUseInput;
		void EnableProxy(bool isEnable);
		void ResetProxy();

		wxButton *proxySettings;
		wxHyperlinkCtrl *proxyLink;
		ProxyCascadeDlg::Cascade proxyCascade;

		wxCheckBox *pathfinderUseInput;
		wxHyperlinkCtrl *pathfinderLink;
		void EnablePathfinder(bool isEnable);

		wxStaticText *serialLineLabel;
		wxTextCtrl *serialLineInput;

		wxStaticBox *serialConfGroup;

		wxStaticText *serialBaudRateLabel;
		wxTextCtrl *serialBaudRateInput;
		
		wxStaticText *serialDataBitsLabel;
		wxTextCtrl *serialDataBitsInput;
		
		wxStaticText *serialStopBitsLabel;
		wxTextCtrl *serialStopBitsInput;
		
		wxStaticText *serialParityLabel;
		wxChoice *serialParityInput;

		wxStaticText *serialFlowControlLabel;
		wxChoice *serialFlowControlInput;

		wxCheckBox *sslUseInput;
		wxButton *sslSettings;
		TunnelEx::SslCertificateId certificate;
		TunnelEx::SslCertificateIdCollection remoteCertificates;
		void EnableSsl(bool isEnable);
		void EnableSsl(
				const TunnelEx::SslCertificateId &certificate,
				const TunnelEx::SslCertificateIdCollection &remoteCertificates);
		void ResetSsl();
		
	};

	typedef boost::array<EndpointInfoItem, 2> EndpointsInfo;

public:

	EndpointDlg(
			ServiceWindow &,
			wxWindow *parent,
			bool isInputEndpoint,
			bool isFtp);

	EndpointDlg(
			ServiceWindow &,
			wxWindow *parent,
			TunnelEx::RuleEndpoint &,
			bool isInputEndpoint,
			bool isFtp);
	~EndpointDlg();

	DECLARE_NO_COPY_CLASS(EndpointDlg);
	DECLARE_EVENT_TABLE();

public:

	const TunnelEx::RuleEndpoint & GetEndpoint() const {
		return m_endpoint;
	}

public:

	//! Returns true if currently edit input endpoint, false otherwise.
	bool IsInputEndpoint() const;

	void OnOk(wxCommandEvent &);
	void OnCancel(wxCommandEvent &);
	void OnHelp(wxCommandEvent &);
	void OnLogToggle(wxCommandEvent &);
	void OnCombinedOrReadPortChanged(wxCommandEvent &);
	void OnWritePortChanged(wxCommandEvent &);
	void OnEndpointTypeChange(wxCommandEvent &);
	void OnEndpointReadWriteTypeChange(wxCommandEvent &);
	void OnLogFolderBrowse(wxCommandEvent &);
	void OnAcceptingToggle(wxCommandEvent &);
	void OnAcceptingWriteToggle(wxCommandEvent &);
	void OnSslUseToggle(wxCommandEvent &);
	void OnSslUseWriteToggle(wxCommandEvent &);
	void OnSslSettings(wxCommandEvent &);
	void OnSslSettingsWrite(wxCommandEvent &);
	void OnProxyUseToggle(wxCommandEvent &);
	void OnProxyUseWriteToggle(wxCommandEvent &);
	void OnProxySettings(wxCommandEvent &);
	void OnProxySettingsWrite(wxCommandEvent &);
	void OnPathfinderUseToggle(wxCommandEvent &);
	void OnIsPathfinderUseWriteToggle(wxCommandEvent &);
	void OnNetworkAdapterChange(wxCommandEvent &);

private:

	void Init();

	void CreateControls();
	void UpdateControls();
	
	void CreateControlReadWriteTypeEndpoint();
	
	void CreateControlEndpoint(EndpointInfoItem &);
	void CreateControlEndpointType(EndpointInfoItem &, int, int);
	void CreateControlEndpointNetworkProto(EndpointInfoItem &, int, int);
	void CreateControlEndpointAction(EndpointInfoItem &, int, int);
	void CreateControlEndpointNetworkAddress(EndpointInfoItem &, int, int);
	void CreateControlEndpointPipe(EndpointInfoItem &, int, int);
	void CreateControlEndpointSsl(EndpointInfoItem &, int, int);
	void CreateControlEndpointProxy(EndpointInfoItem &, int, int);
	void CreateControlEndpointPathfinder(EndpointInfoItem &, int, int);
	void CreateControlEndpointSerial(EndpointInfoItem &, int, int);
	void CreateControlLog();
	void CreateControlDialogButtons();

	void ShowControls();
	void ShowControlReadWriteTypeEndpoint(int &);
	void ShowControlEndpoint(EndpointInfoItem &, int &);
	void ShowControlEndpointType(EndpointInfoItem &, int &);
	void ShowControlEndpointNetworkProto(EndpointInfoItem &, int &);
	void ShowControlEndpointAction(EndpointInfoItem &, int &);
	void ShowControlEndpointNetworkAddress(EndpointInfoItem &, int &);
	void ShowControlEndpointSerial(EndpointInfoItem &, int &);
	void ShowControlEndpointPipe(EndpointInfoItem &, int &);
	void ShowControlEndpointSsl(EndpointInfoItem &, int &);
	void ShowControlEndpointProxy(EndpointInfoItem &, int &);
	void ShowControlEndpointPathfinder(EndpointInfoItem &, int &);
	void ShowControlLog(int &);
	void ShowControlDialogButtons(bool isReadOnly, int &);
	
	void CheckControlValues(EndpointInfoItem &);

	void ReadEndpointInfo();
	void ReadEndpointInfo(
			const TunnelEx::EndpointAddress &,
			EndpointInfoItem &)
		const;
	template<typename Address>
	void ReadEndpointSslInfo(const Address &, EndpointInfoItem &) const;
	void CreateDefaultEndpointInfo(EndpointInfoItem &) const;
	void ReadLogInfo();

	bool SaveEndpointAddress(
			bool isOrigEndpointCombined,
			bool isNewEndpointCombined,
			const EndpointInfoItem &info,
			boost::function<TunnelEx::SharedPtr<TunnelEx::EndpointAddress>(void)> getAddressFunc,
			TunnelEx::WString &resourceIdentifier)
		const;

	void OnPortChanged(EndpointInfoItem &info) const;

	void SaveEnpointTemplate() const;

	void CheckAcceptingControls(bool readEndpointAction);
	void CheckSslUseControls(bool readEndpointAction, bool isSet);
	void CheckProxyUseControls(bool readEndpointAction, bool isSet);
	void CheckPathfinderUseControls(bool readEndpointAction, bool isSet);

	bool ShowSslSettingsDialog(bool readEndpointAction);
	bool ShowSslSettingsDialog(EndpointInfoItem &);

	bool ShowProxySettingsDialog(bool readEndpointAction);
	bool ShowProxySettingsDialog(EndpointInfoItem &);

	void ShowSplitRwLicenseRestriction() const;

	void RefreshServiceNetworkAdaptes();

	bool CheckInactiveAdapterWarning(const texs__NetworkAdapterInfo &) const;
	bool CheckInactiveAdapterWarning(const TunnelEx::Endpoint &) const;
	bool CheckInactiveAdapterWarning(const TunnelEx::Mods::Inet::InetEndpointAddress &) const;

	static void CenterControls(wxControl &label, const wxControl &control);

	bool IsSplitEndpoint() const;
	void SetSplitEndpoint(bool);

private:

	TunnelEx::RuleEndpoint m_endpoint;
	EndpointsInfo m_endpointsInfo;
	bool m_hasChanges;
	bool m_isInputEndpoint;
	bool m_isFtpEndpoint;
	const bool m_isNewEndpoint;
	unsigned char m_borderWidth;
	unsigned char m_internalBorderWidth;
	bool m_isFirstTimeReadWriteTypeChanged;

	ServiceWindow &m_service;

	struct Licenses;
	std::auto_ptr<Licenses> m_licenses;

	std::list<texs__NetworkAdapterInfo> m_serviceNetworkAdapters;

	wxStaticText *m_readWriteLabel;
	wxChoice *m_readWriteInput;
	
	LogInfo m_logInfoSource;
	wxStaticBox *m_logGroup;
	wxCheckBox *m_logToggle;
	wxStaticText *m_logFolderLabel;
	wxTextCtrl *m_logFolderInput;
	wxButton *m_logBrowseButton;

	wxStaticLine *m_line;
	wxButton *m_okButton;
	wxButton *m_cancelButton;
	wxButton *m_helpButton;

	bool m_isUpnpDevChecked;

};

#endif // INCLUDED_FILE__TUNNELEX__EndpointDlg_h__0801270540
