/**************************************************************************
 *   Created: 2008/02/16 21:02
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: ServiceAdapter.hpp 1097 2010-12-14 18:04:02Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__TexService_h__0802162102
#define INCLUDED_FILE__TUNNELEX__TexService_h__0802162102

#include "Rule.hpp"

class wxString;

class ServiceAdapter : private boost::noncopyable {

public:

	class Event : public wxNotifyEvent {

	public:

		enum Id {
			ID_NEW_ERROR,
			ID_NEW_WARNING,
			ID_NEW_LOG_RECORD,
			ID_RULE_SET_MODIFIED,
			ID_SERVICE_RUNNING_STATE_CHANGED,
			ID_CONNECTED,
			ID_DISCONNECTED,
			ID_CONNECTION_FAILED,
			ID_LICENSE_KEY_MODIFIED
		};

		typedef void (wxEvtHandler::*Handler)(ServiceAdapter::Event &);

	public:

		explicit Event(wxEventType, Id);
		virtual ~Event();

	public:
		
		virtual wxEvent * Clone() const;

	};

public:

	class ServiceError : public TunnelEx::LocalException {
	public:
		class ServiceError(const wchar_t *error)
				: LocalException(error) {
			//...//
		}
	};

public:
	
	explicit ServiceAdapter(const wxString &endpoint);
	
	explicit ServiceAdapter(
			wxEvtHandler &statusEventListener,
			const wxString &endpoint,
			time_t lastKnownErrorTime,
			time_t lastKnownWarnTime);
	
	~ServiceAdapter() throw();

public:

	void Reconnect(
			wxEvtHandler &statusEventListener,
			const wxString &endpoint,
			time_t lastKnownErrorTime,
			time_t lastKnownWarnTime);

public:

	bool Start();
	bool Stop();
	bool IsStarted() const;
	void GetRuleSet(TunnelEx::RuleSet &ruleSet) const;
	void UpdateRules(const TunnelEx::RuleSet &);
	void EnableRules(const RulesUuids &);
	void DisableRules(const RulesUuids &);
	void DeleteRules(const RulesUuids &);
	void GetLogRecords(
			unsigned int recordsNumber,
			std::list<texs__LogRecord> &records)
		const;
	texs__LogLevel GetLogLevel() const;
	void SetLogLevel(texs__LogLevel);
	bool Migrate();

	void GetNetworkAdapters(
			bool resetCache,
			std::list<texs__NetworkAdapterInfo> &result)
		const;

	bool IsConnected() const;

	const wxString & GetEndpoint() const;

	time_t GetLastKnownErrorTime() const;
	time_t GetLastKnownWarnTime() const;
	
	time_t GetLastLicenseKeyModificatiomTime() const;
	
	unsigned long long GetLogSize() const;

	void GenerateLicenseKeyRequest(
			const std::string &license,
			std::string &request,
			std::string &privateKey);
	std::string GetTrialLicense() const;
	std::string GetLicenseKey() const;
	std::string GetLicenseKeyLocalAsymmetricPrivateKey() const;
	void SetLicenseKey(const std::string &key, const std::string &privateKey);
	bool GetProperties(TunnelEx::Licensing::WorkstationPropertyValues &);

	bool GetUpnpStatus(wxString &externalIp, wxString &localIp) const;
	boost::optional<wxString> GetCachedUpnpDeviceExternalIp() const;

	void GetSslCertificates(std::list<texs__SslCertificateShortInfo> &) const;
	void GetSslCertificate(
				const std::wstring &id,
				texs__SslCertificateInfo &)
			const;
	void DeleteSslCertificates(const std::list<std::wstring> &ids);
	void ImportSslCertificateX509(
				const std::vector<unsigned char> &certificate,
				const std::string &password);
	void ImportSslCertificatePkcs12(
				const std::vector<unsigned char> &certificate,
				const std::string &privateKey);
	// see TEX-642
	// std::wstring ExportSslCertificate(const std::wstring &id) const;

private:

	class Implementation;
	std::auto_ptr<Implementation> m_pimpl;

};

DECLARE_EVENT_TYPE(EVT_SERVICE_ADAPTER_ACTION, -1);

#define EVT_SERVICE_ADAPTER(id, fn) \
	DECLARE_EVENT_TABLE_ENTRY( \
		EVT_SERVICE_ADAPTER_ACTION, \
		id, \
		-1, \
		(wxObjectEventFunction)(wxEventFunction) wxStaticCastEvent(ServiceAdapter::Event::Handler, &fn), \
		(wxObject *)NULL),

#endif // INCLUDED_FILE__TUNNELEX__TexService_h__0802162102
