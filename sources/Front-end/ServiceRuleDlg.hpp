/**************************************************************************
 *   Created: 2010/05/23 5:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Service_hpp__1005230520
#define INCLUDED_FILE__TUNNELEX__Service_hpp__1005230520

#include "RuleDlg.hpp"

class wxChoice;
class wxTextCtrl;
class wxCheckBox;

//////////////////////////////////////////////////////////////////////////

class ServiceRuleDlg : public RuleDlg {

protected:

	enum ControlId {
		CONTROL_ID_LAST = RuleDlg::CONTROL_ID_LAST
	};

public:

	explicit ServiceRuleDlg(
			const wxString &title,
			ServiceWindow &service,
			wxWindow *parent);
	explicit ServiceRuleDlg(
			const wxString &title,
			ServiceWindow &service,
			wxWindow *parent,
			const TunnelEx::Rule &);
	virtual ~ServiceRuleDlg();

	DECLARE_NO_COPY_CLASS(ServiceRuleDlg);

public:

	static std::auto_ptr<TunnelEx::ServiceRule> EditRule(
			ServiceWindow &,
			wxWindow &parent,
			const TunnelEx::ServiceRule &);

public:

	const TunnelEx::ServiceRule & GetRule() const {
		return const_cast<ServiceRuleDlg *>(this)->GetRule();
	}

protected:

	TunnelEx::ServiceRule & GetRule() {
		return *boost::polymorphic_downcast<TunnelEx::ServiceRule *>(&RuleDlg::GetRule());
	}

private:

	virtual bool Save(TunnelEx::Rule &) const;
	virtual bool SaveService(TunnelEx::ServiceRule::Service &) const = 0;

};

//////////////////////////////////////////////////////////////////////////

class UpnpServiceRuleDlg: public ServiceRuleDlg {

protected:

	enum ControlId {
		CONTROL_ID_EXTERNAL_PORT = ServiceRuleDlg::CONTROL_ID_LAST,
		CONTROL_ID_DESTINATION_HOST,
		CONTROL_ID_DESTINATION_PORT,
		CONTROL_ID_LAST
	};

public:

	explicit UpnpServiceRuleDlg(
			ServiceWindow &,
			wxWindow *parent);
	explicit UpnpServiceRuleDlg(
			ServiceWindow &,
			wxWindow *parent,
			const TunnelEx::Rule &);
	virtual ~UpnpServiceRuleDlg();

	DECLARE_NO_COPY_CLASS(UpnpServiceRuleDlg);
	DECLARE_EVENT_TABLE();

public:

	virtual bool IsLicenseValid(void) const;

	const wxString & GetExternalIpAddress() const {
		return m_externalIp;
	}

private:

	void Init();

	virtual std::auto_ptr<wxSizer> CreateControlContent();
	virtual std::auto_ptr<wxSizer> CreateControlAdditionalOptions();

	void CheckLicense();

	virtual const wxChar * GetHelpPath(void) const;

	virtual bool SaveService(TunnelEx::ServiceRule::Service &) const;

	void CheckPortValue(wxTextCtrl &, wxString &) const;

public:

	void OnExternalPortChanged(wxCommandEvent &);
	void OnDestinationPortChanged(wxCommandEvent &);
	void OnDestinationHostPasted(wxClipboardTextEvent &);

private:

	wxString m_sourceProto;
	wxString m_sourceExternalPort;
	wxString m_sourceDestinationHost;
	wxString m_sourceDestinationPort;
	bool m_sourceIsForceMode;

	wxChoice *m_proto;
	wxTextCtrl *m_externalPort;
	wxString m_externalValidPort;
	wxTextCtrl *m_destinationHost; 
	wxTextCtrl *m_destinationPort;
	wxString m_destinationValidPort;
	wxCheckBox *m_forceRecriation;

	bool m_isDevExist;
	wxString m_localIp;
	wxString m_externalIp;

	bool m_isLicenseValid;

};


//////////////////////////////////////////////////////////////////////////

#endif // INCLUDED_FILE__TUNNELEX__Service_hpp__1005230520
