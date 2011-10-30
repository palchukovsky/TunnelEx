/**************************************************************************
 *   Created: 2010/09/06 19:53
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__RuleShortDlg_hpp__1009061953
#define INCLUDED_FILE__TUNNELEX__RuleShortDlg_hpp__1009061953

#include "ServiceAdapter.hpp"
#include "RuleDlg.hpp"
#include "ProxyDlg.hpp"
#include "Modules/Inet/InetEndpointAddress.hpp"

class TunnelRuleShortDlg : public RuleDlg {

public:

	typedef RuleDlg Base;

private:

	typedef std::list<wxPanel *> Pannels;
	typedef std::list<wxHyperlinkCtrl *> Links;

protected:

	enum ControlId {
		CONTROL_ID_TYPE = CONTROL_ID_LAST,
		CONTROL_ID_NETWORK_ADAPTER,
		CONTROL_ID_PORT_INPUT,
		CONTROL_ID_HOST_DESTINATION,
		CONTROL_ID_PORT_DESTINATION,
		CONTROL_ID_PROXY_USE,
		CONTROL_ID_PROXY_SETTINGS,
		CONTROL_ID_PATHFINDER_USE,
		CONTROL_ID_ADVANCED_MODE,
		CONTROL_ID_PREV_STEP,
		CONTROL_ID_INPUT_SSL_USE,
		CONTROL_ID_INPUT_SSL_SETTINGS,
		CONTROL_ID_DESTINATION_SSL_USE,
		CONTROL_ID_DESTINATION_SSL_SETTINGS,
		CONTROL_ID_LAST
	};

public:

	explicit TunnelRuleShortDlg(ServiceWindow &service, wxWindow *const parent);
	explicit TunnelRuleShortDlg(
			ServiceWindow &service,
			wxWindow *const parent,
			const TunnelEx::TunnelRule &);
	virtual ~TunnelRuleShortDlg();

	DECLARE_NO_COPY_CLASS(TunnelRuleShortDlg);

	DECLARE_EVENT_TABLE();

public:

	static bool CheckRule(const TunnelEx::TunnelRule &);

public:

	virtual bool IsLicenseValid() const;

	const TunnelEx::TunnelRule & GetRule() const {
		return const_cast<TunnelRuleShortDlg *>(this)->GetRule();
	}

	virtual const wxChar * GetHelpPath() const;
	virtual bool Save(TunnelEx::Rule &rule) const;

	bool IsAdvancdeMode() const {
		return m_isAdvancedMode;
	}

	bool IsFtp() const {
		return m_typeFtp->GetValue();
	}

public:

	void OnTypeChange(wxCommandEvent &);
	void OnNetworkAdapterChange(wxCommandEvent &);
	void OnInputPortChanged(wxCommandEvent &);
	void OnDestinationHostPasted(wxClipboardTextEvent &);
	void OnDestinationPortChanged(wxCommandEvent &);
	void OnProxyUseToggle(wxCommandEvent &);
	void OnProxySettings(wxCommandEvent &);
	void OnPathfinderUseToggle(wxCommandEvent &);
	void OnAdvancedMode(wxCommandEvent &);
	void OnPrevStep(wxCommandEvent &);
	void OnOk(wxCommandEvent &);

	void OnUseInputSslToggle(wxCommandEvent &);
	void OnUseDestintationSslToggle(wxCommandEvent &);
	void OnInputSslSettings(wxCommandEvent &);
	void OnDestintationSslSettings(wxCommandEvent &);

protected:

	virtual void Init();
	void UpdateVisible();
	virtual void Cancel();

	void UpdateVisibleNewRule();
	void UpdateVisibleRuleEdit();

	TunnelEx::TunnelRule & GetRule() {
		return *boost::polymorphic_downcast<TunnelEx::TunnelRule *>(&Base::GetRule());
	}

	virtual void SaveTemplate() const;

	void OnSslSettings(
			bool,
			TunnelEx::SslCertificateId &,
			TunnelEx::SslCertificateIdCollection &)
		const;

private:

	virtual std::auto_ptr<wxSizer> CreateControlOptions();
	virtual std::auto_ptr<wxSizer> CreateControlContent();
	virtual std::auto_ptr<wxSizer> CreateButtons();
	virtual std::auto_ptr<wxSizer> CreateControlRuleInfo();

	unsigned char GetFirstStep() const {
		return 1;
	}

	unsigned char GetLastStep() const {
		return 2;
	}

	void OnPortChanged(wxTextCtrl &, wxString &) const;

	void CheckLicense();

	void ReadRule();

	void OnUseSslToggle(
			wxCheckBox &checkBox,
			wxButton &button,
			TunnelEx::SslCertificateId &certificate,
			TunnelEx::SslCertificateIdCollection &remoteCertificates,
			wxTextCtrl &port,
			bool isPortChanged)
		const;

private:

	wxPanel *m_typeBox;
	wxPanel *m_ruleEnableBox;
	wxPanel *m_tcpUdpFtpBox;
	wxPanel *m_pipeBox;
	wxPanel *m_serialBox;
	wxPanel *m_proxySubPanel;
	wxPanel *m_sslInputSubPanel;
	wxPanel *m_sslDestinationSubPanel;
	Pannels m_allPanels;
	Links m_proxyPanelLinks;

	wxRadioButton *m_typeTcp;
	wxRadioButton *m_typeUdp;
	wxRadioButton *m_typeFtp;
	wxRadioButton *m_typePipe;
	wxRadioButton *m_typeSerial;
	wxStaticText *m_typeDescription;
	wxHyperlinkCtrl *m_typeFtpLink;

	wxChoice *m_inputAdapter;
	wxTextCtrl *m_inputPort;
	wxString m_inputPortValid;

	wxCheckBox *m_inputSslUse;
	wxButton *m_inputSslSettings;

	wxTextCtrl *m_inputPipe;
	
	wxTextCtrl *m_inputSerialLine;
	wxTextCtrl *m_inputSerialBaudRate;
	wxTextCtrl *m_inputSerialDataBits;
	wxTextCtrl *m_inputSerialStopBits;
	wxChoice *m_inputSerialParity;
	wxChoice *m_inputSerialFlowControl;

	wxTextCtrl *m_destinationHost;
	wxStaticBox *m_destinationTcpUdpFtpBox;
	wxTextCtrl *m_destinationPort;
	wxString m_destinationPortValid;

	wxCheckBox *m_destinationSslUse;
	wxButton *m_destinationSslSettings;

	wxCheckBox *m_destinationProxyUse;
	ProxyCascadeDlg::Cascade m_proxyCascade;
	wxButton *m_destinationProxySettings;
	wxCheckBox *m_destinationPathfinderUse;

	wxTextCtrl *m_destinationPipe;
	
	wxTextCtrl *m_destinationSerialLine;
	wxTextCtrl *m_destinationSerialBaudRate;
	wxTextCtrl *m_destinationSerialDataBits;
	wxTextCtrl *m_destinationSerialStopBits;
	wxChoice *m_destinationSerialParity;
	wxChoice *m_destinationSerialFlowControl;

	std::list<texs__NetworkAdapterInfo> m_serviceNetworkAdapters;

	bool m_isAdvancedMode;

	unsigned char m_step;
	wxButton *m_prevStepButton;
	wxButton *m_okButton;

	struct Licenses;
	std::auto_ptr<Licenses> m_licenses;
	bool m_isLicenseValid;
	
	bool m_isUpnpDevChecked;

	TunnelEx::SslCertificateId m_inputCertificate;
	TunnelEx::SslCertificateIdCollection m_inputRemoteCertificates;

	TunnelEx::SslCertificateId m_destinationCertificate;
	TunnelEx::SslCertificateIdCollection m_destinationRemoteCertificates;

	bool m_isInputPortChanged;
	bool m_isDestinationPortChanged;

};

#endif // INCLUDED_FILE__TUNNELEX__RuleShortDlg_hpp__1009061953
