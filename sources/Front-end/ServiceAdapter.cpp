 /**************************************************************************
 *   Created: 2008/02/16 21:06
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "ServiceAdapter.hpp"
#include "Core/Exceptions.hpp"
#include "LicensePolicies.hpp"

using namespace TunnelEx;

//////////////////////////////////////////////////////////////////////////

class ServiceAdapter::Implementation : public boost::noncopyable {

private:

	class ConnectionError : public TunnelEx::LocalException {
	public:
		class ConnectionError(const wchar_t *error)
				: LocalException(error) {
			//...//
		}
	};

	class StateCheckingThread : public wxThread {

	public:

		explicit StateCheckingThread(ServiceAdapter::Implementation &serviceAdapter)
				: wxThread(wxTHREAD_JOINABLE),
				m_serviceAdapter(serviceAdapter) {
			//...//
		}

	public:

		virtual ExitCode Entry() {
			const DWORD eventsCount = 2;
			HANDLE events[] = {
				m_serviceAdapter.m_stateCheckingStopEvent.get(),
				m_serviceAdapter.m_stateCheckingUpdateEvent.get()
			};
			unsigned int errorsCount = 0;
			//! @todo: hardcoded timeout value
			for (const DWORD iterationWait = 2500; ; ) {
				const DWORD event = WaitForMultipleObjects(
					eventsCount,
					&events[0],
					FALSE,
					iterationWait);
				switch (event) {
					case WAIT_OBJECT_0:
						return 0;
					case WAIT_OBJECT_0 + 1:
						{
							wxMutexLocker lock(m_serviceAdapter.m_serviceMutex);
							ResetEvent(events[1]);
						}
					case WAIT_TIMEOUT:
						{
							texs__ServiceState state;
							int errorCode;
							{
								wxMutexLocker serviceLock(
									m_serviceAdapter.m_serviceMutex);
								errorCode
									= m_serviceAdapter.m_service.texs__CheckState(state);
							}
							BOOST_ASSERT(
								errorCode == SOAP_EOF
								|| errorCode == SOAP_OK
								|| errorCode == SOAP_TCP_ERROR
								|| errorCode == SOAP_UDP_ERROR);
							if (errorCode == SOAP_OK) {
								errorsCount = 0;
								if (!m_serviceAdapter.m_isConnected) {
									m_serviceAdapter.m_isConnected = true;
									m_serviceAdapter.m_lastRulesModifyTime = 0;
									m_serviceAdapter.GenerateEvent(
										ServiceAdapter::Event::ID_CONNECTED);
								}
								m_serviceAdapter.CheckState(state);
							} else if (m_serviceAdapter.m_isConnected) {
								if (++errorsCount > 2) {
									m_serviceAdapter.m_isConnected = false;
									m_serviceAdapter.GenerateEvent(
										ServiceAdapter::Event::ID_DISCONNECTED);
								} else {
									m_serviceAdapter.InitiateStateUpdate(
										wxMutexLocker(m_serviceAdapter.m_serviceMutex));
								}
							} else {
								m_serviceAdapter.GenerateEvent(
									ServiceAdapter::Event::ID_CONNECTION_FAILED);
							}
						}
						break;
					default:
						BOOST_ASSERT(false);
						continue;
				}
			}
			return 0;
		}

	private:

		ServiceAdapter::Implementation &m_serviceAdapter;

	};

	template<typename Result>
	class RequestThread : public wxThread {
	public:
		explicit RequestThread(
					const boost::function<Result(void)> requestFunc,
					const Result &defaultResult)
				: wxThread(wxTHREAD_JOINABLE),
				m_requestFunc(requestFunc),
				m_defaultResult(defaultResult) {
			//...//
		}
	public:
		const typename Result & GetRequestResult() const {
			if (m_exeption) {
				wxLogError(m_exeption->GetWhat());
				return m_defaultResult;
			}
			return m_requestResult;
		}
		virtual ExitCode Entry() {
			try {
				m_requestResult = m_requestFunc();
			} catch (const ::TunnelEx::LocalException &ex) {
				m_exeption = ex.Clone();
			}
			return 0;
		}
	private:
		const boost::function<Result(void)> m_requestFunc;
		Result m_requestResult;
		const Result &m_defaultResult;
		UniquePtr<LocalException> m_exeption;
	};

	template<>
	class RequestThread<void> : public wxThread {
	public:
		explicit RequestThread(
					const boost::function<void(void)> requestFunc)
				: wxThread(wxTHREAD_JOINABLE),
				m_requestFunc(requestFunc) {
			//...//
		}
	public:
		void GetRequestResult() const {
			if (m_exeption) {
				wxLogError(m_exeption->GetWhat());
			}
		}
		virtual ExitCode Entry() {
			try {
				m_requestFunc();
			} catch (const ::TunnelEx::LocalException &ex) {
				m_exeption = ex.Clone();
			}
			return 0;
		}
	private:
		const boost::function<void(void)> m_requestFunc;
		UniquePtr<LocalException> m_exeption;
	};

public:

	explicit Implementation(const wxString &endpoint)
			: m_statusEventListener(0),
			m_serviceMutex(wxMUTEX_RECURSIVE),
			m_endpoint(ConvertString<String>(endpoint.c_str())),
			m_originalEndpoint(endpoint),
			m_isStarted(false),
			m_stateCheckingThread(*this),
			m_isConnected(false),
			m_logSize(0) {
		InitSoapConnector();
	}

	explicit Implementation(
				wxEvtHandler &statusEventListener,
				const wxString &endpoint,
				time_t lastKnownErrorTime,
				time_t lastKnownWarnTime)
			: m_statusEventListener(&statusEventListener),
			m_serviceMutex(wxMUTEX_RECURSIVE),
			m_endpoint(ConvertString<String>(endpoint.c_str())),
			m_originalEndpoint(endpoint),
			m_stateCheckingStopEvent(CreateEvent(NULL, FALSE, FALSE, NULL), &CloseHandle),
			m_stateCheckingUpdateEvent(CreateEvent(NULL, FALSE, FALSE, NULL), &CloseHandle),
			m_lastErrorTime(lastKnownErrorTime),
			m_lastWarningTime(lastKnownWarnTime),
			m_lastRulesModifyTime(0),
			m_lastLicenseKeyModificatiomTime(0),
			m_isStarted(false),
			m_stateCheckingThread(*this),
			m_isConnected(false) {
		InitSoapConnector();
		m_stateCheckingThread.Create();
		m_stateCheckingThread.Run();
		InitiateStateUpdate(wxMutexLocker(m_serviceMutex));
	}

	~Implementation() {
		if (m_stateCheckingThread.IsRunning()) {
			BOOST_ASSERT(m_stateCheckingStopEvent.get());
			SetEvent(m_stateCheckingStopEvent.get());
			m_stateCheckingThread.Wait();
		}
	}

private:

	template<typename Request>
	void DoRequestWithProgressBar(Request &request, const wxString &progressBarMessage) {
		request.Create();
		request.Run();
		for (size_t i = 0; i < 9 && request.IsRunning(); ++i, wxMilliSleep(25)); 
		if (request.IsRunning()) {
			wxProgressDialog progress(
				wxEmptyString,
				!progressBarMessage.IsEmpty()
					?	progressBarMessage
					:	wxT("Requesting current state information, please wait..."),
				100,
				NULL,
				wxPD_APP_MODAL | wxPD_SMOOTH);
			for ( ; request.IsRunning(); progress.Pulse(), wxMilliSleep(25));
		}
		request.Wait();
	}

	template<typename T>
	typename const T & GetDefaultRequestResult() {
		static T val;
		return val;
	}
	
	template<>
	const bool & GetDefaultRequestResult() {
		static bool val = false;
		return val;
	}

	template<>
	const texs__LogLevel & GetDefaultRequestResult() {
		static texs__LogLevel val = LOG_LEVEL_INFO;
		return val;
	}

	template<>
	const std::list<texs__NetworkAdapterInfo> * const & GetDefaultRequestResult() {
		static std::list<texs__NetworkAdapterInfo> emptyVal;
		static std::list<texs__NetworkAdapterInfo> *ptr = &emptyVal;
		return ptr;
	}

public:

	template<typename Result>
	typename Result RequestWithProgressBar(
			const boost::function<Result(void)> requestFunc,
			const wxString &progressBarMessage) {
		RequestThread<Result> request(requestFunc, GetDefaultRequestResult<Result>());
		DoRequestWithProgressBar(request, progressBarMessage);
		return request.GetRequestResult();
	}

	template<>
	void RequestWithProgressBar<void>(
				const boost::function<void(void)> requestFunc,
				const wxString &progressBarMessage) {
		RequestThread<void> request(requestFunc);
		DoRequestWithProgressBar(request, progressBarMessage);
		request.GetRequestResult();
	}

	bool Start() {
		for (int attempts = 0; ; ) {
			bool result;
			wxMutexLocker serviceLock(m_serviceMutex);
			const int resultCode = m_service.texs__Start(result);
			++attempts;
			if (resultCode != SOAP_EOF || attempts >= 2) {
				if (resultCode != SOAP_OK) {
					HandleSoapError(resultCode);
				}
				InitiateStateUpdate(serviceLock);
				return result;
			}
		}
	}

	bool Stop() {
		for (int attempts = 0; ; ) {
			bool result;
			wxMutexLocker serviceLock(m_serviceMutex);
			const int resultCode = m_service.texs__Stop(result);
			++attempts;
			if (resultCode != SOAP_EOF || attempts >= 2) {
				if (resultCode != SOAP_OK) {
					HandleSoapError(resultCode);
				}
				InitiateStateUpdate(serviceLock);
				return result;
			}
		}
	}

	bool IsStarted() const {
		return m_isStarted;
	}

	void GetRuleSet(RuleSet &result) const {
		for (int attempts = 0; ; ) {
			std::string rulesXml;
			wxMutexLocker serviceLock(m_serviceMutex);
			const int resultCode = m_service.texs__GetRuleSet(rulesXml);
			++attempts;
			if (resultCode != SOAP_EOF || attempts >= 2) {
				if (resultCode != SOAP_OK) {
					HandleSoapError(resultCode);
				}
				try {
					RuleSet(ConvertString<WString>(rulesXml.c_str())).Swap(result);
					return;
				} catch (const ::TunnelEx::XmlDoesNotMatchException& ex) {
					throw ServiceError(ex.GetWhat());
				} catch (const ::TunnelEx::InvalidXmlException& ex) {
					throw ServiceError(ex.GetWhat());
				}
			}
		}
	}

	void UpdateRules(const RuleSet &rules) {
		if (!rules.GetSize()) {
			return;
		}
		for (int attempts = 0; ; ) {
			WString xml;
			wxMutexLocker serviceLock(m_serviceMutex);
			rules.GetXml(xml);
			const int resultCode
				= m_service.texs__UpdateRules(xml.GetCStr(), NULL);
			++attempts;
			if (resultCode != SOAP_EOF || attempts >= 2) {
				if (resultCode != SOAP_OK) {
					HandleSoapError(resultCode);
				}
				InitiateStateUpdate(serviceLock);
				return;
			}
		}
	}

	void DeleteRules(const RulesUuids &uuids) {
		std::list<std::wstring> transportContainer(uuids.begin(), uuids.end());
		for (int attempts = 0; ; ) {
			wxMutexLocker serviceLock(m_serviceMutex);
			const int resultCode
				= m_service.texs__DeleteRules(transportContainer, NULL);
			++attempts;
			if (resultCode != SOAP_EOF || attempts >= 2) {
				if (resultCode != SOAP_OK) {
					HandleSoapError(resultCode);
				}
				InitiateStateUpdate(serviceLock);
				return;
			}
		}
	}

	void EnableRules(const RulesUuids &uuids) {
		std::list<std::wstring> transportContainer(uuids.begin(), uuids.end());
		for (int attempts = 0; ; ) {
			wxMutexLocker serviceLock(m_serviceMutex);
			const int resultCode
				= m_service.texs__EnableRules(transportContainer, NULL);
			++attempts;
			if (resultCode != SOAP_EOF || attempts >= 2) {
				if (resultCode != SOAP_OK) {
					HandleSoapError(resultCode);
				}
				InitiateStateUpdate(serviceLock);
				return;
			}
		}
	}

	void DisableRules(const RulesUuids &uuids) {
		std::list<std::wstring> transportContainer(uuids.begin(), uuids.end());
		for (int attempts = 0; ; ) {
			wxMutexLocker serviceLock(m_serviceMutex);
			const int resultCode
				= m_service.texs__DisableRules(transportContainer, NULL);
			++attempts;
			if (resultCode != SOAP_EOF || attempts >= 2) {
				if (resultCode != SOAP_OK) {
					HandleSoapError(resultCode);
				}
				InitiateStateUpdate(serviceLock);
				return;
			}
		}
	}

	void GetLogRecords(
				unsigned int recordsNumber,
				std::list<texs__LogRecord> &records)
			const {
		for (int attempts = 0; ; ) {
			wxMutexLocker serviceLock(m_serviceMutex);
			const int resultCode
				= m_service.texs__GetLogRecords(recordsNumber, records);
			++attempts;
			if (resultCode != SOAP_EOF || attempts >= 2) {
				if (resultCode != SOAP_OK) {
					HandleSoapError(resultCode);
				}
				return;
			}
		}
	}

	void SetLogLevel(texs__LogLevel logLevel) {
		for (int attempts = 0; ; ) {
			wxMutexLocker serviceLock(m_serviceMutex);
			const int resultCode = m_service.texs__SetLogLevel(logLevel, NULL);
			++attempts;
			if (resultCode != SOAP_EOF || attempts >= 2) {
				if (resultCode != SOAP_OK) {
					HandleSoapError(resultCode);
				}
				return;
			}
		}
	}

	texs__LogLevel GetLogLevel() {
		for (int attempts = 0; ; ) {
			wxMutexLocker serviceLock(m_serviceMutex);
			texs__LogLevel result;
			const int resultCode = m_service.texs__GetLogLevel(result);
			++attempts;
			if (resultCode != SOAP_EOF || attempts >= 2) {
				if (resultCode != SOAP_OK) {
					HandleSoapError(resultCode);
				}
				return result;
			}
		}
	}

	void GetNetworkAdapters(
				bool resetCache,
				std::list<texs__NetworkAdapterInfo> &result)
			const {
		std::list<texs__NetworkAdapterInfo> adapters;
		for (int attempts = 0; ; ) {
			wxMutexLocker serviceLock(m_serviceMutex);
			if (m_networkAdaptersCache.size() && !resetCache) {
				std::list<texs__NetworkAdapterInfo>(m_networkAdaptersCache).swap(result);
				return;
			}
			const int resultCode
				= m_service.texs__GetNetworkAdapters(adapters);
			++attempts;
			if (resultCode != SOAP_EOF || attempts >= 2) {
				if (resultCode != SOAP_OK) {
					HandleSoapError(resultCode);
				} else {
					adapters.swap(
						const_cast<Implementation *>(this)->m_networkAdaptersCache);
				}
				std::list<texs__NetworkAdapterInfo>(m_networkAdaptersCache).swap(result);
				return;
			}
		}
	}

	void GetSslCertificates(std::list<texs__SslCertificateShortInfo> &result) const {
		for (int attempts = 0; ; ) {
			wxMutexLocker serviceLock(m_serviceMutex);
			const int resultCode = m_service.texs__GetSslCertificates(result);
			++attempts;
			if (resultCode != SOAP_EOF || attempts >= 2) {
				if (resultCode != SOAP_OK) {
					HandleSoapError(resultCode);
				}
				return;
			}
		}
	}

	void GetSslCertificate(
				const std::wstring &id,
				texs__SslCertificateInfo &result)
			const {
		for (int attempts = 0; ; ) {
			wxMutexLocker serviceLock(m_serviceMutex);
			const int resultCode = m_service.texs__GetSslCertificate(id, result);
			++attempts;
			if (resultCode != SOAP_EOF || attempts >= 2) {
				if (resultCode != SOAP_OK) {
					HandleSoapError(resultCode);
				}
				return;
			}
		}
	}

	void ImportSslCertificatex509(
				const std::vector<unsigned char> &certificate,
				const std::string &password) {
		for (int attempts = 0; ; ) {
			std::wstring error;
			wxMutexLocker serviceLock(m_serviceMutex);
			const int resultCode
				= m_service.texs__ImportSslCertificateX509(certificate, password, error);
			++attempts;
			if (resultCode != SOAP_EOF || attempts >= 2) {
				if (resultCode != SOAP_OK) {
					HandleSoapError(resultCode);
				} else if (!error.empty()) {
					wxLogError(error.c_str());
				}
				return;
			}
		}
	}

	void ImportSslCertificatePkcs12(
				const std::vector<unsigned char> &certificate,
				const std::string &privateKey) {
		for (int attempts = 0; ; ) {
			std::wstring error;
			wxMutexLocker serviceLock(m_serviceMutex);
			const int resultCode
				= m_service.texs__ImportSslCertificatePkcs12(certificate, privateKey, error);
			++attempts;
			if (resultCode != SOAP_EOF || attempts >= 2) {
				if (resultCode != SOAP_OK) {
					HandleSoapError(resultCode);
				} else if (!error.empty()) {
					wxLogError(error.c_str());
				}
				return;
			}
		}
	}

	// see TEX-642
	/* std::string ExportSslCertificate(const std::wstring &id) const {
		for (int attempts = 0; ; ) {
			std::string result;
			wxMutexLocker serviceLock(m_serviceMutex);
			const int resultCode
				= m_service.texs__ExportSslCertificate(id, result);
			++attempts;
			if (resultCode != SOAP_EOF || attempts >= 2) {
				if (resultCode != SOAP_OK) {
					HandleSoapError(resultCode);
				}
				return result;
			}
		}
	} */

	void DeleteSslCertificates(const std::list<std::wstring> &ids) {
		for (int attempts = 0; ; ) {
			std::wstring error;
			wxMutexLocker serviceLock(m_serviceMutex);
			const int resultCode
				= m_service.texs__DeleteSslCertificates(ids, NULL);
			++attempts;
			if (resultCode != SOAP_EOF || attempts >= 2) {
				if (resultCode != SOAP_OK) {
					HandleSoapError(resultCode);
				}
				return;
			}
		}
	}

	bool Migrate() {
		for (int attempts = 0; ; ) {
			bool result = false;
			wxMutexLocker serviceLock(m_serviceMutex);
			const int resultCode = m_service.texs__Migrate(result);
			++attempts;
			if (resultCode != SOAP_EOF || attempts >= 2) {
				if (resultCode != SOAP_OK) {
					HandleSoapError(resultCode);
				}
				return result;
			}
		}
	}

	void GenerateLicenseKeyRequest(
				const std::string &license,
				std::string &request,
				std::string &privateKey) {
		for (int attempts = 0; ; ) {
			texs__LicenseKeyRequest result;
			wxMutexLocker serviceLock(m_serviceMutex);
			const int resultCode = m_service.texs__GenerateLicenseKeyRequest(
				license,
				result);
			++attempts;
			if (resultCode != SOAP_EOF || attempts >= 2) {
				if (resultCode != SOAP_OK) {
					HandleSoapError(resultCode);
				}
				request = result.request;
				privateKey = result.key;
				return;
			}
		}
	}

	std::string GetTrialLicense() const {
		for (int attempts = 0; ; ) {
			std::string result;
			wxMutexLocker serviceLock(m_serviceMutex);
			const int resultCode = m_service.texs__GetTrialLicense(result);
			++attempts;
			if (resultCode != SOAP_EOF || attempts >= 2) {
				if (resultCode != SOAP_OK) {
					HandleSoapError(resultCode);
				}
				return result;
			}
		}
	}

	std::string GetLicenseKey() const {
		for (int attempts = 0; ; ) {
			std::string result;
			wxMutexLocker serviceLock(m_serviceMutex);
			const int resultCode = m_service.texs__GetLicenseKey(result);
			++attempts;
			if (resultCode != SOAP_EOF || attempts >= 2) {
				if (resultCode != SOAP_OK) {
					HandleSoapError(resultCode);
				}
				return result;
			}
		}
	}

	std::string GetLicenseKeyLocalAsymmetricPrivateKey() const {
		for (int attempts = 0; ; ) {
			texs__LicenseKeyInfo info;
			wxMutexLocker serviceLock(m_serviceMutex);
			const int resultCode = m_service.texs__GetLicenseKeyInfo(info);
			++attempts;
			if (resultCode != SOAP_EOF || attempts >= 2) {
				if (resultCode != SOAP_OK) {
					HandleSoapError(resultCode);
				}
				return info.key;
			}
		}
	}

	void SetLicenseKey(const std::string &key, const std::string &privateKey) {
		for (int attempts = 0; ; ) {
			texs__SetLicenseKeyResult result;
			wxMutexLocker serviceLock(m_serviceMutex);
			const int resultCode
				= m_service.texs__SetLicenseKey(key, privateKey, &result);
			++attempts;
			if (resultCode != SOAP_EOF || attempts >= 2) {
				if (resultCode != SOAP_OK) {
					HandleSoapError(resultCode);
				}
				return;
			}
		}
	}

	bool GetProperties(Licensing::WorkstationPropertyValues &result) {
		
		using namespace Licensing;
		
		for (int attempts = 0; ; ) {
		
			wxMutexLocker serviceLock(m_serviceMutex);

			if (m_properties.get()) {
				result = *m_properties;
				return true;
			}

			std::vector<unsigned char> encryptedBuffer;
			const int resultCode = m_service.texs__GetProperties(encryptedBuffer);
			++attempts;

			if (resultCode != SOAP_EOF || attempts >= 2) {

				if (resultCode != SOAP_OK) {
					HandleSoapError(resultCode);
					return false;
				}

				std::vector<char> decryptedBuffer(encryptedBuffer.size());
				std::vector<unsigned char> key;
				LocalComunicationPolicy::GetEncryptingKey(key);
				size_t token = 0;
				std::vector<char>::iterator pos = decryptedBuffer.begin();
				foreach (char ch, encryptedBuffer) {
					ch ^= key[token++ % key.size()];
					*pos = ch;
					++pos;
				}

				typedef int8_t Result;
				if (	decryptedBuffer.size() < sizeof(Result)
						|| !*reinterpret_cast<Result *>(&decryptedBuffer[0])) {
					return false;
				}

				typedef int32_t Id;
				typedef int32_t ValSize;

				std::auto_ptr<WorkstationPropertyValues> properties(
					new WorkstationPropertyValues);
				for (size_t i = sizeof(Result); ; ) {
					const size_t bufferSize = decryptedBuffer.size() - i;
					BOOST_ASSERT(bufferSize == 0 || bufferSize > sizeof(Id) + sizeof(ValSize));
					if (bufferSize <= sizeof(Id) + sizeof(ValSize)) {
						break;
					}
					const Id id = *reinterpret_cast<Id *>(&decryptedBuffer[i]);
					const ValSize valSize = *reinterpret_cast<ValSize *>(&decryptedBuffer[i + sizeof(Id)]);
					const size_t valBeginOffset = i + sizeof(Id) + sizeof(ValSize);
					const size_t valEndOffset = i + sizeof(Id) + sizeof(ValSize) + valSize;
					properties->insert(
						make_pair(
							WorkstationProperty(id),
							std::string(
								&decryptedBuffer[valBeginOffset],
								&decryptedBuffer[0] + valEndOffset)));
					i += sizeof(Id) + sizeof(ValSize) + valSize;
				}

				BOOST_ASSERT(encryptedBuffer.size() == 0 || properties->size() > 0);
				
				m_properties = properties;
				result = *m_properties;
				return true;

			}

		}

	}

	bool GetUpnpStatus(wxString &externalIp, wxString &localIp) const {
		for (int attempts = 0; ; ) {
			texs__UpnpStatus result;
			wxMutexLocker serviceLock(m_serviceMutex);
			const_cast<Implementation *>(this)->m_cachedUpnpDeviceExternalIp = std::wstring();
			const int resultCode = m_service.texs__GetUpnpStatus(result);
			++attempts;
			if (resultCode != SOAP_EOF || attempts >= 2) {
				if (resultCode != SOAP_OK) {
					HandleSoapError(resultCode);
				}
				wxString externalIpTmp = wxString::FromAscii(result.externalIp.c_str());
				wxString externalIpCache = externalIpTmp;
				wxString localIpTmp = wxString::FromAscii(result.localIp.c_str());
				const_cast<Implementation *>(this)
					->m_cachedUpnpDeviceExternalIp = externalIpCache;
				externalIpTmp.swap(externalIp);
				localIpTmp.swap(localIp);
				return result.isDeviceExist;
			}
		}
	}

	boost::optional<wxString> GetCachedUpnpDeviceExternalIp() const {
		wxMutexLocker serviceLock(m_serviceMutex);
		return m_cachedUpnpDeviceExternalIp;
	}

	bool IsConnected() const {
		return m_isConnected;
	}

	const wxString & GetEndpoint() const {
		return m_originalEndpoint;
	}

	time_t GetLastKnownErrorTime() const {
		return m_lastErrorTime;
	}

	time_t GetLastKnownWarnTime() const {
		return m_lastWarningTime;
	}

	time_t GetLastLicenseKeyModificatiomTime() const {
		return m_lastLicenseKeyModificatiomTime;
	}

	unsigned long long GetLogSize() const {
		return m_logSize;
	}

private:

	void InitiateStateUpdate(const wxMutexLocker &) {
		if (m_stateCheckingThread.IsRunning()) {
			BOOST_ASSERT(m_stateCheckingUpdateEvent.get());
			SetEvent(m_stateCheckingUpdateEvent.get());
		}
	}

	void InitSoapConnector() {
		soap_set_omode(m_service.soap, SOAP_IO_KEEPALIVE);
		soap_set_imode(m_service.soap, SOAP_IO_KEEPALIVE);
		m_service.soap->max_keep_alive = 1000;
		m_service.endpoint = m_endpoint.GetCStr();
	}

	void HandleSoapError(int errorCode) const {
		std::wstring errorMessage;
		if (errorCode == SOAP_TCP_ERROR || errorCode == SOAP_UDP_ERROR) {
			WFormat format(
				L"Could not connect to " TUNNELEX_NAME_W L" service (%1%).\n"
				L"Re-installing the application may fix this problem.\n"
				L"Local firewall can be the reason of it. Check your\n"
				L"firewall manual to give access for " TUNNELEX_NAME_W L".\n"
				L"Also you can check that " TUNNELEX_NAME_W L" service is started:\n"
				L"open service list (\"Start\" > \"Control panel\" >\n"
				L"\"Administrative Tools\" > \"Services\"),\n"
				L"find " TUNNELEX_NAME_W L" service and start it if stopped.");
			format % errorCode;
			errorMessage = format.str();
		} else {
			std::ostringstream oss;
			soap_stream_fault(m_service.soap, oss);
			std::string soapError(
				boost::regex_replace(
					oss.str(),
					boost::regex("[\n\t\r]"),
					" ",
					boost::match_default | boost::format_all));
			boost::trim_if(soapError, boost::is_any_of("\n\t\r "));
			WFormat format(L"Server communication error: \"%1%\" (code: %2%).");
			format % ConvertString<WString>(soapError.c_str()).GetCStr();
			format % errorCode;
			errorMessage = format.str();
		}
		throw ConnectionError(errorMessage.c_str());
	}

	template<class CheckpointTypeLocal, class CheckpointTypeRemote>
	void CheckCheckpointState(
				CheckpointTypeLocal &localCheckpoint,
				const CheckpointTypeRemote &remoteCheckpoint,
				ServiceAdapter::Event::Id eventId) {
		if (localCheckpoint != remoteCheckpoint) {
			localCheckpoint = remoteCheckpoint;
			GenerateEvent(eventId);
		}
	}

	template<class CheckpointTypeRemote>
	void CheckCheckpointState(
				time_t &localCheckpoint,
				const CheckpointTypeRemote &remoteCheckpoint,
				ServiceAdapter::Event::Id eventId) {
		if (localCheckpoint < remoteCheckpoint) {
			localCheckpoint = remoteCheckpoint;
			GenerateEvent(eventId);
		} else if (localCheckpoint != remoteCheckpoint && remoteCheckpoint == 0) {
			localCheckpoint = remoteCheckpoint;
		}
	}

	void CheckState(const texs__ServiceState &state) {
		CheckCheckpointState(
			m_isStarted,
			state.isStarted,
			ServiceAdapter::Event::ID_SERVICE_RUNNING_STATE_CHANGED);
		CheckCheckpointState(
			m_lastRulesModifyTime,
			state.ruleSetTime,
			ServiceAdapter::Event::ID_RULE_SET_MODIFIED);
		CheckCheckpointState(
			m_lastLicenseKeyModificatiomTime,
			state.licKeyTime,
			ServiceAdapter::Event::ID_LICENSE_KEY_MODIFIED);
		CheckCheckpointState(
			m_logSize,
			state.logSize,
			ServiceAdapter::Event::ID_NEW_LOG_RECORD);
		CheckCheckpointState(
			m_lastErrorTime,
			state.errorTime,
			ServiceAdapter::Event::ID_NEW_ERROR);
		CheckCheckpointState(
			m_lastWarningTime,
			state.warnTime,
			ServiceAdapter::Event::ID_NEW_WARNING);
	}

	void GenerateEvent(ServiceAdapter::Event::Id eventId) const {
		ServiceAdapter::Event adapterEvent(EVT_SERVICE_ADAPTER_ACTION, eventId);
		BOOST_ASSERT(m_statusEventListener);
		wxPostEvent(m_statusEventListener, adapterEvent);
	}

private:

	wxEvtHandler *const m_statusEventListener;

	mutable TunnelExService m_service;
	mutable wxMutex m_serviceMutex;

	const String m_endpoint;
	const wxString m_originalEndpoint;

	//! @todo: replace with unique_ptr
	boost::shared_ptr<void> m_stateCheckingStopEvent;
	boost::shared_ptr<void> m_stateCheckingUpdateEvent;

	time_t m_lastErrorTime;
	time_t m_lastWarningTime;
	time_t m_lastRulesModifyTime;
	time_t m_lastLicenseKeyModificatiomTime;
	bool m_isStarted;
	unsigned long long m_logSize;
	
	StateCheckingThread m_stateCheckingThread;

	bool m_isConnected;

	std::list<texs__NetworkAdapterInfo> m_networkAdaptersCache;
	boost::optional<wxString> m_cachedUpnpDeviceExternalIp;

	std::auto_ptr<Licensing::WorkstationPropertyValues> m_properties;

};

//////////////////////////////////////////////////////////////////////////

DEFINE_EVENT_TYPE(EVT_SERVICE_ADAPTER_ACTION);

ServiceAdapter::Event::Event(wxEventType type, Id id)
		: wxNotifyEvent(type, id) {
	//...//
}

ServiceAdapter::Event::~Event() {
	//...//
}

wxEvent * ServiceAdapter::Event::Clone() const {
	return new Event(*this);
}

//////////////////////////////////////////////////////////////////////////

ServiceAdapter::ServiceAdapter(const wxString &endpoint)
		: m_pimpl(new Implementation(endpoint)) {
	//...//
}

ServiceAdapter::ServiceAdapter(
			wxEvtHandler &statusEventListener,
			const wxString &endpoint,
			time_t lastKnownErrorTime,
			time_t lastKnownWarnTime)
		: m_pimpl(
			new Implementation(
				statusEventListener,
				endpoint,
				lastKnownErrorTime,
				lastKnownWarnTime)) {
	//...//
}

ServiceAdapter::~ServiceAdapter() {
	//...//
}

void ServiceAdapter::Reconnect(
			wxEvtHandler &statusEventListener,
			const wxString &endpoint,
			time_t lastKnownErrorTime,
			time_t lastKnownWarnTime) {
	m_pimpl.reset(
		new Implementation(
			statusEventListener,
			endpoint,
			lastKnownErrorTime,
			lastKnownWarnTime));
}

bool ServiceAdapter::Start() {
	return m_pimpl->RequestWithProgressBar<bool>(
		boost::bind(&Implementation::Start, m_pimpl.get()),
		wxT("Starting ") TUNNELEX_NAME_W wxT(", please wait..."));
}

bool ServiceAdapter::Stop() {
	return m_pimpl->RequestWithProgressBar<bool>(
		boost::bind(&Implementation::Stop, m_pimpl.get()),
		wxT("Stopping ") TUNNELEX_NAME_W wxT(", please wait..."));
}

bool ServiceAdapter::IsStarted() const {
	return m_pimpl->IsStarted();
}

void ServiceAdapter::UpdateRules(const RuleSet &rules) {
	m_pimpl->RequestWithProgressBar<void>(
		boost::bind(&Implementation::UpdateRules, m_pimpl.get(), boost::cref(rules)),
		wxT("Applying rule set changes, please wait..."));
}

void ServiceAdapter::GetRuleSet(RuleSet &ruleSet) const {
	m_pimpl->RequestWithProgressBar<void>(
		boost::bind(&Implementation::GetRuleSet, m_pimpl.get(), boost::ref(ruleSet)),
		wxT("Refreshing rule std::set, please wait..."));
}

void ServiceAdapter::DeleteRules(const RulesUuids &uuids) {
	m_pimpl->RequestWithProgressBar<void>(
		boost::bind(&Implementation::DeleteRules, m_pimpl.get(), boost::cref(uuids)),
		wxT("Deleting selected rules, please wait..."));
}

const wxString & ServiceAdapter::GetEndpoint() const {
	return m_pimpl->GetEndpoint();
}

void ServiceAdapter::GetLogRecords(
			unsigned int recordsNumber,
			std::list<texs__LogRecord> &records)
		const {
	m_pimpl->RequestWithProgressBar<void>(
		boost::bind(&Implementation::GetLogRecords, m_pimpl.get(), recordsNumber, boost::ref(records)),
		wxT("Requesting service log, please wait..."));
}

void ServiceAdapter::SetLogLevel(texs__LogLevel logLevel) {
	m_pimpl->RequestWithProgressBar<void>(
		boost::bind(&Implementation::SetLogLevel, m_pimpl.get(), logLevel),
		wxEmptyString);
}

texs__LogLevel ServiceAdapter::GetLogLevel() const {
	return m_pimpl->RequestWithProgressBar<texs__LogLevel>(
		boost::bind(&Implementation::GetLogLevel, m_pimpl.get()),
		wxEmptyString);
}

void ServiceAdapter::GetNetworkAdapters(
			bool resetCache,
			std::list<texs__NetworkAdapterInfo> &result)
		const {
	m_pimpl->RequestWithProgressBar<void>(
		boost::bind(&Implementation::GetNetworkAdapters, m_pimpl.get(), resetCache, boost::ref(result)),
		wxEmptyString);
}

bool ServiceAdapter::Migrate() {
	try {
		return m_pimpl->Migrate();
	} catch (const LocalException &ex) {
		wxLogError(ex.GetWhat());
		return false;
	}
}

time_t ServiceAdapter::GetLastKnownErrorTime() const {
	try {
		return m_pimpl->GetLastKnownErrorTime();
	} catch (const LocalException &ex) {
		wxLogError(ex.GetWhat());
		return 0;
	}
}

time_t ServiceAdapter::GetLastKnownWarnTime() const {
	try {
		return m_pimpl->GetLastKnownWarnTime();
	} catch (const LocalException &ex) {
		wxLogError(ex.GetWhat());
		return 0;
	}
}

time_t ServiceAdapter::GetLastLicenseKeyModificatiomTime() const {
	try {
		return m_pimpl->GetLastLicenseKeyModificatiomTime();
	} catch (const LocalException &ex) {
		wxLogError(ex.GetWhat());
		return 0;
	}
}

bool ServiceAdapter::IsConnected() const {
	try {
		return m_pimpl->IsConnected();
	} catch (const LocalException &ex) {
		wxLogError(ex.GetWhat());
		return false;
	}
}

unsigned long long ServiceAdapter::GetLogSize() const {
	try {
		return m_pimpl->GetLogSize();
	} catch (const LocalException &ex) {
		wxLogError(ex.GetWhat());
		return 0;
	}
}

void ServiceAdapter::EnableRules(const RulesUuids &uuids) {
	m_pimpl->RequestWithProgressBar<void>(
		boost::bind(&Implementation::EnableRules, m_pimpl.get(), boost::cref(uuids)),
		wxT("Enabling selected rules, please wait..."));
}

void ServiceAdapter::DisableRules(const RulesUuids &uuids) {
	m_pimpl->RequestWithProgressBar<void>(
		boost::bind(&Implementation::DisableRules, m_pimpl.get(), boost::cref(uuids)),
		wxT("Disabling selected rules, please wait..."));
}

void ServiceAdapter::GenerateLicenseKeyRequest(
			const std::string &license,
			std::string &request,
			std::string &privateKey) {
	m_pimpl->RequestWithProgressBar<void>(
		boost::bind(
			&Implementation::GenerateLicenseKeyRequest,
			m_pimpl.get(),
			boost::cref(license),
			boost::ref(request),
			boost::ref(privateKey)),
		wxEmptyString);
}

std::string ServiceAdapter::GetTrialLicense() const {
	return m_pimpl->RequestWithProgressBar<std::string>(
		boost::bind(&Implementation::GetTrialLicense, m_pimpl.get()),
		wxEmptyString);
}

std::string ServiceAdapter::GetLicenseKey() const {
	return m_pimpl->RequestWithProgressBar<std::string>(
		boost::bind(&Implementation::GetLicenseKey, m_pimpl.get()),
		wxEmptyString);
}

void ServiceAdapter::SetLicenseKey(
			const std::string &key,
			const std::string &privateKey) {
	m_pimpl->RequestWithProgressBar<void>(
		boost::bind(
			&Implementation::SetLicenseKey,
			m_pimpl.get(),
			boost::cref(key),
			boost::cref(privateKey)),
		wxEmptyString);
}

bool ServiceAdapter::GetProperties(Licensing::WorkstationPropertyValues &result) {
	try {
		return m_pimpl->GetProperties(result);
	} catch (const LocalException &ex) {
		wxLogError(ex.GetWhat());
		return false;
	}
}

std::string ServiceAdapter::GetLicenseKeyLocalAsymmetricPrivateKey() const {
	return m_pimpl->RequestWithProgressBar<std::string>(
		boost::bind(&Implementation::GetLicenseKeyLocalAsymmetricPrivateKey, m_pimpl.get()),
		wxEmptyString);
}

bool ServiceAdapter::GetUpnpStatus(wxString &externalIp, wxString &localIp) const {
	return m_pimpl->RequestWithProgressBar<bool>(
		boost::bind(&Implementation::GetUpnpStatus, m_pimpl.get(), boost::ref(externalIp), boost::ref(localIp)),
		wxT("Getting information about UPnP device, please wait..."));
}

boost::optional<wxString> ServiceAdapter::GetCachedUpnpDeviceExternalIp() const {
	try {
		return m_pimpl->GetCachedUpnpDeviceExternalIp();
	} catch (const LocalException &ex) {
		wxLogError(ex.GetWhat());
		return boost::optional<wxString>();
	}
}

void ServiceAdapter::GetSslCertificates(std::list<texs__SslCertificateShortInfo> &result) const {
	m_pimpl->RequestWithProgressBar<void>(
		boost::bind(&Implementation::GetSslCertificates, m_pimpl.get(), boost::ref(result)),
		wxT("Getting installed SSL certificates list, please wait..."));
}

void ServiceAdapter::GetSslCertificate(
			const std::wstring &id,
			texs__SslCertificateInfo &result)
		const {
	m_pimpl->RequestWithProgressBar<void>(
		boost::bind(&Implementation::GetSslCertificate, m_pimpl.get(), boost::cref(id), boost::ref(result)),
		wxT("Getting SSL certificate, please wait..."));
}

void ServiceAdapter::DeleteSslCertificates(const std::list<std::wstring> &ids) {
	m_pimpl->RequestWithProgressBar<void>(
		boost::bind(&Implementation::DeleteSslCertificates, m_pimpl.get(), boost::cref(ids)),
		wxT("Deleting certificates, please wait..."));
}

void ServiceAdapter::ImportSslCertificateX509(
			const std::vector<unsigned char> &certificate,
			const std::string &password) {
	m_pimpl->RequestWithProgressBar<void>(
		boost::bind(&Implementation::ImportSslCertificatex509, m_pimpl.get(), boost::cref(certificate), boost::cref(password)),
		wxT("Importing and installing SSL certificate, please wait..."));
}

void ServiceAdapter::ImportSslCertificatePkcs12(
			const std::vector<unsigned char> &certificate,
			const std::string &privateKey) {
	m_pimpl->RequestWithProgressBar<void>(
		boost::bind(&Implementation::ImportSslCertificatePkcs12, m_pimpl.get(), boost::cref(certificate), boost::cref(privateKey)),
		wxT("Importing and installing SSL certificate, please wait..."));
}

// see TEX-642
/* std::wstring ServiceAdapter::ExportSslCertificate(const std::wstring &id) const {
	return m_pimpl->RequestWithProgressBar<std::string>(
		boost::bind(&Implementation::ExportSslCertificate, m_pimpl.get(), boost::cref(id)),
		wxT("Exporting certificates, please wait..."));
} */
