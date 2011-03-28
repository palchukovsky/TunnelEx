/**************************************************************************
 *   Created: 2008/02/17 3:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: TunnelExService.h 1097 2010-12-14 18:04:02Z palchukovsky $
 **************************************************************************/

#import "stllist.h"
#import "stlvector.h"

//gsoap texs service name: TunnelExService
//gsoap texs service style: rpc
//gsoap texs service namespace: http://schemas.tunnelex.net/service/2.3.wsdl
//gsoap texs schema namespace: http://schemas.tunnelex.net/service/2.3.xsd
//gsoap texs service location: No

//gsoap texs service method-action: Start "urn:#start"
int texs__Start(bool &startServerResult);

//gsoap texs service method-action: Stop "urn:#stop"
int texs__Stop(bool &stopServerResult);

//gsoap texs service method-action: IsStarted "urn:#isStarted"
int texs__IsStarted(bool &isServerStartedResult);

//gsoap texs service method-action: GetRuleSet "urn:#getRuleSet"
int texs__GetRuleSet(std::string &getRuleListResult);

class texs__ServiceState {
public:
	long long errorTime;
	long long warnTime;
	long long licKeyTime;
	unsigned long long logSize;
	long long ruleSetTime;
	bool isStarted;
};

//gsoap texs service method-action: texs__CheckState "urn:#checkState"
int texs__CheckState(texs__ServiceState &serviceState);

//gsoap texs service method-action: UpdateRules "urn:#updateRules"
int texs__UpdateRules(
		std::wstring sereleasedRulesToUpdate,
		struct texs__UpdateRulesResult {} *updateRulesResult);

//gsoap texs service method-action: EnableRules "urn:#enableRules"
int texs__EnableRules(
		std::list<std::wstring> rulesUuidsToEnable,
		struct texs__EnableRulesResult {} *enableRulesResult);

//gsoap texs service method-action: DisableRules "urn:#disableRules"
int texs__DisableRules(
		std::list<std::wstring> rulesUuidsToDisable,
		struct texs__DisableRulesResult {} *disableRulesResult);

//gsoap texs service method-action: DeleteRules "urn:#deleteRules"
int texs__DeleteRules(
		std::list<std::wstring> rulesUuidsToDelete,
		struct texs__DeleteRulesResult {} *deleteRulesResult);

class texs__LogRecord {
public:
	std::string time;
	unsigned int level;
	std::string message;
};

//gsoap texs service method-action: GetLogRecords "urn:#getLogRecords"
int texs__GetLogRecords(
		unsigned int recordsNumber,
		std::list<texs__LogRecord> &getLogRecordsResult);

enum texs__LogLevel {
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_INFO,
	LOG_LEVEL_WARN,
	LOG_LEVEL_ERROR
};

//gsoap texs service method-action: SetLogLevel "urn:#setLogLevel"
int texs__SetLogLevel(
		enum texs__LogLevel logLevel,
		struct texs__SetLogLevelResult {} *setLogLevelResult);

//gsoap texs service method-action: GetLogLevel "urn:#getLogLevel"
int texs__GetLogLevel(enum texs__LogLevel &getLogLevelResult);

//gsoap texs service method-action: Migrate "urn:#migrate"
int texs__Migrate(bool &migrateResult);

class texs__LicenseKeyRequest {
public:
	std::string request;
	std::string key;
};

//gsoap texs service method-action: GenerateLicenseKeyRequest "urn:#generateLicenseKeyRequest"
int texs__GenerateLicenseKeyRequest(
		std::string license,
		texs__LicenseKeyRequest &request);

//gsoap texs service method-action: GetTrialLicense "urn:#getTrialLicense"
int texs__GetTrialLicense(std::string &license);

//gsoap texs service method-action: GetLicenseKey "urn:#getLicenseKey"
int texs__GetLicenseKey(std::string &licenseKey);

class texs__LicenseKeyInfo {
public:
	std::string key;
};

//gsoap texs service method-action: GetLicenseKeyInfo "urn:#getLicenseKeyInfo"
int texs__GetLicenseKeyInfo(texs__LicenseKeyInfo &licenseKeyInfo);

//gsoap texs service method-action: SetLicenseKey "urn:#setLicenseKey"
int texs__SetLicenseKey(
		std::string licenseKey,
		std::string privateKey,
		struct texs__SetLicenseKeyResult {} *setLicenseKeyResult);

//gsoap texs service method-action: GetProperties "urn:#getProperties"
int texs__GetProperties(std::vector<unsigned char> &properties);

class texs__NetworkAdapterInfo {
public:
	std::string id;
	std::string name;
	std::string ipAddress;
};

//gsoap texs service method-action: GetNetworkAdapters "urn:#getNetworkAdapters"
int texs__GetNetworkAdapters(
		std::list<texs__NetworkAdapterInfo> &getNetworkAdaptersResult);

class texs__UpnpStatus {
public:
	bool isDeviceExist;
	std::string externalIp;
	std::string localIp;
};

//gsoap texs service method-action: GetNetworkAdapters "urn:#getNetworkAdapters"
int texs__GetUpnpStatus(texs__UpnpStatus &status);

class texs__SslCertificateShortInfo {
public:
	std::wstring id;
	bool isPrivate;
	std::string subjectCommonName;
	std::string issuerCommonName;
	time_t validBeforeTimeUtc;
};

class texs__SslCertificateInfo {
public:
	//! Local ID - out
	std::wstring id;
	//! Is a personal certificate flag - out
	bool isPrivate;
	//! Key size - inout
	int keySize;
	//! Serial number - out
	std::string serial;
	//! Valid time (after) - out
	time_t validAfterTimeUtc;
	//! Valid time (before) - out
	time_t validBeforeTimeUtc;
	//! Issuer common name - inout
	std::string subjectCommonName;
	//! Issuer organization - inout
	std::string subjectOrganization;
	//! Issuer organization unit - inout
	std::string subjectOrganizationUnit;
	//! Issuer city - inout
	std::string subjectCity;
	//! Issuer state or province - inout
	std::string subjectStateOrProvince;
	//! Issuer country - inout
	std::string subjectCountry;
	//! Subject common name - inout
	std::string issuerCommonName;
	//! Full human readable info - out
	std::string fullInfo;
	//! Certificate - out
	std::string certificate;
};

//gsoap texs service method-action: GetSslCertificates "urn:#getSslCertificates"
int texs__GetSslCertificates(
		std::list<texs__SslCertificateShortInfo> &getSslCertificatesResult);

//gsoap texs service method-action: GetSslCertificate "urn:#getSslCertificate"
int texs__GetSslCertificate(
		std::wstring id,
		texs__SslCertificateInfo &getSslCertificatesResult);

//gsoap texs service method-action: DeleteSslCertificate "urn:#deleteSslCertificate"
int texs__DeleteSslCertificates(
		std::list<std::wstring> idsToDelete,
		struct texs__DeleteSslCertificateResult {} *deleteSslCertificateResult);

//gsoap texs service method-action: ImportSslCertificateX509 "urn:#importSslCertificateX509"
int texs__ImportSslCertificateX509(
		std::vector<unsigned char> certificate,
		std::string privateKey,
		std::wstring &error);

//gsoap texs service method-action: ImportSslCertificatePkcs12 "urn:#importSslCertificatePkcs12"
int texs__ImportSslCertificatePkcs12(
		std::vector<unsigned char> certificate,
		std::string password,
		std::wstring &error);
