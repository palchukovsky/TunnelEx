/**************************************************************************
 *   Created: 2008/02/16 20:09
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "WinService.hpp"
#include "TexServiceImplementation.hpp"

int texs__Start(soap *, bool &result) {
	boost::shared_ptr<TexServiceImplementation> service(TexWinService::GetTexServiceInstance());
	result = service->Start();
	service->SaveRunningState();
	return SOAP_OK;
}

int texs__Stop(soap *, bool &result) {
	boost::shared_ptr<TexServiceImplementation> service(TexWinService::GetTexServiceInstance());
	result = service->Stop();
	service->SaveRunningState();
	return SOAP_OK;
}

int texs__IsStarted(soap *, bool &result) {
	result = TexWinService::GetTexServiceInstance()->IsStarted();
	return SOAP_OK;
}

int texs__GetRuleSet(soap *, std::string &result) {
	TexWinService::GetTexServiceInstance()->GetRuleSet(result);
	return SOAP_OK;
}

int texs__UpdateRules(soap *, std::wstring rulesXml, texs__UpdateRulesResult *) {
	TexWinService::GetTexServiceInstance()->UpdateRules(rulesXml);
	return SOAP_OK;
}

int texs__EnableRules(soap *, std::list<std::wstring> uuidsToEnable, texs__EnableRulesResult *) {
	TexWinService::GetTexServiceInstance()->EnableRules(uuidsToEnable, true);
	return SOAP_OK;
}

int texs__DisableRules(soap *, std::list<std::wstring> uuidsToDisable, texs__DisableRulesResult *) {
	TexWinService::GetTexServiceInstance()->EnableRules(uuidsToDisable, false);
	return SOAP_OK;
}

int texs__DeleteRules(soap *, std::list<std::wstring> uuidsToDelete, texs__DeleteRulesResult *) {
	TexWinService::GetTexServiceInstance()->DeleteRules(uuidsToDelete);
	return SOAP_OK;
}

int texs__GetLogRecords(
			soap *,
			unsigned int recordsNumber,
			std::list<texs__LogRecord> &result) {
	TexWinService::GetTexServiceInstance()->GetLastLogRecords(recordsNumber, result);
	return SOAP_OK;
}

int texs__SetLogLevel(
			soap *,
			enum texs__LogLevel texsLogLevel,
			struct texs__SetLogLevelResult *) {
	using namespace TunnelEx;
	LogLevel texLogLevel;
	switch (texsLogLevel) {
		case ::LOG_LEVEL_DEBUG:
			texLogLevel = TunnelEx::LOG_LEVEL_DEBUG;
			break;
		case ::LOG_LEVEL_INFO:
			texLogLevel = TunnelEx::LOG_LEVEL_INFO;
			break;
		case ::LOG_LEVEL_WARN:
			texLogLevel = TunnelEx::LOG_LEVEL_WARN;
			break;
		case ::LOG_LEVEL_ERROR:
			texLogLevel = TunnelEx::LOG_LEVEL_ERROR;
			break;
		default:
			assert(false);
			texLogLevel = TunnelEx::LOG_LEVEL_INFO;
	}
	TexWinService::GetTexServiceInstance()->SetLogLevel(texLogLevel);
	return SOAP_OK;
}

int texs__GetLogLevel(struct soap*, enum texs__LogLevel &getLogLevelResult) {
	switch (TexWinService::GetTexServiceInstance()->GetLogLevel()) {
		case TunnelEx::LOG_LEVEL_TRACK:
		case TunnelEx::LOG_LEVEL_DEBUG:
			getLogLevelResult = ::LOG_LEVEL_DEBUG;
			break;
		case TunnelEx::LOG_LEVEL_UNKNOWN:
		default:
			assert(false);
		case TunnelEx::LOG_LEVEL_INFO:
			getLogLevelResult = ::LOG_LEVEL_INFO;
			break;
		case TunnelEx::LOG_LEVEL_WARN:
			getLogLevelResult = ::LOG_LEVEL_WARN;
			break;
		case TunnelEx::LOG_LEVEL_ERROR:
		case TunnelEx::LOG_LEVEL_SYSTEM_ERROR:
		case TunnelEx::LOG_LEVEL_FATAL_ERROR:
			getLogLevelResult = ::LOG_LEVEL_ERROR;
			break;
	}
	return SOAP_OK;
}

int texs__Migrate(soap *, bool &result) {
	result = TexWinService::GetTexServiceInstance()->Migrate();
	return SOAP_OK;
}

int texs__CheckState(soap *, texs__ServiceState &result) {
	TexWinService::GetTexServiceInstance()->CheckState(result);
	return SOAP_OK;
}

int texs__GenerateLicenseKeyRequest(
			soap *,
			std::string license,
			texs__LicenseKeyRequest &request) {
	TexWinService::GetTexServiceInstance()->GenerateLicenseKeyRequest(
		license,
		request.request,
		request.key);
	return SOAP_OK;
}

int texs__GetLicenseKey(soap *, std::string &licenseKey) {
	licenseKey = TexWinService::GetTexServiceInstance()->GetLicenseKey();
	return SOAP_OK;
}

int texs__RegisterLicenseError(
			soap *,
			int client,
			std::string license,
			std::string time,
			std::string point,
			std::string error,
			texs__RegisterLicenseErrorResult *) {
	using namespace TunnelEx::Licensing;
	assert(client >= CLIENT_START && client < CLIENT_LAST_INDEX);
	TexWinService::GetTexServiceInstance()->RegisterLicenseError(
		Client(client),
		license,
		time,
		point,
		error);
	return SOAP_OK;
}

int texs__GetLicenseKeyInfo(soap *, texs__LicenseKeyInfo &result) {
	result.key = TexWinService::GetTexServiceInstance()
		->GetLicenseKeyLocalAsymmetricPrivateKey();
	return SOAP_OK;
}

int texs__GetTrialLicense(soap *, std::string &licenseKey) {
	licenseKey = TexWinService::GetTexServiceInstance()->GetTrialLicense();
	return SOAP_OK;
}

int texs__SetLicenseKey(
			soap *, 
			std::string licenseKey,
			std::string privateKey,
			texs__SetLicenseKeyResult *) {
	TexWinService::GetTexServiceInstance()->SetLicenseKey(licenseKey, privateKey);
	return SOAP_OK;
}

int texs__GetProperties(soap *, std::vector<unsigned char> &result) {
	TexWinService::GetTexServiceInstance()->GetProperties(result);
	return SOAP_OK;
}

int texs__GetNetworkAdapters(soap *, std::list<texs__NetworkAdapterInfo> &result) {
	TexWinService::GetTexServiceInstance()->GetNetworkAdapters(result);
	return SOAP_OK;
}

int texs__GetUpnpStatus(soap *, texs__UpnpStatus &result) {
	result.isDeviceExist = TexWinService::GetTexServiceInstance()->GetUpnpStatus(
		result.externalIp,
		result.localIp);
	return SOAP_OK;
}

int texs__GetSslCertificates(soap *, std::list<texs__SslCertificateShortInfo> &result) {
	TexWinService::GetTexServiceInstance()->GetSslCertificates(result);
	return SOAP_OK;
}

int texs__GetSslCertificate(soap *, std::wstring id, texs__SslCertificateInfo &result) {
	TexWinService::GetTexServiceInstance()->GetSslCertificate(id, result);
	return SOAP_OK;
}

int texs__DeleteSslCertificates(
			soap *,
			std::list<std::wstring> ids,
			texs__DeleteSslCertificateResult *)  {
	TexWinService::GetTexServiceInstance()->DeleteSslCertificates(ids);
	return SOAP_OK;
}

int texs__ImportSslCertificatePkcs12(
			soap *,
			std::vector<unsigned char> certificate,
			std::string password,
			std::wstring &error) {
	TexWinService::GetTexServiceInstance()->ImportSslCertificatePkcs12(
		certificate,
		password,
		error);
	return SOAP_OK;
}

int texs__ImportSslCertificateX509(
			soap *,
			std::vector<unsigned char> certificate,
			std::string privateKey,
			std::wstring &error) {
	TexWinService::GetTexServiceInstance()->ImportSslCertificateX509(
		certificate,
		privateKey,
		error);
	return SOAP_OK;
}
