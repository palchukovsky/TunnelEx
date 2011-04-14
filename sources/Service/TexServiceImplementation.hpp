/**************************************************************************
 *   Created: 2008/02/17 5:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: TexServiceImplementation.hpp 1097 2010-12-14 18:04:02Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__TexServiceImplementation_h__0802170531
#define INCLUDED_FILE__TUNNELEX__TexServiceImplementation_h__0802170531

#include "Core/Log.hpp"

class TexServiceImplementation : private boost::noncopyable {

	friend class TexWinService;

private:

	TexServiceImplementation();

public:

	~TexServiceImplementation() throw();

public:

	bool Start();
	bool Stop();
	bool IsStarted() const;
	void SaveRunningState() const;
	
	void GetRuleSet(std::string &) const;
	
	void UpdateRules(const std::wstring &);
	void EnableRules(const std::list<std::wstring> &, bool isEnabled);
	void DeleteRules(const std::list<std::wstring> &);

	void CheckState(texs__ServiceState &) const;

	void GetLastLogRecords(unsigned int, std::list<texs__LogRecord> &) const;

	bool Migrate();

	void GetNetworkAdapters(std::list<texs__NetworkAdapterInfo> &) const;

	void GetSslCertificates(std::list<texs__SslCertificateShortInfo> &) const;
	void GetSslCertificate(const std::wstring &id, texs__SslCertificateInfo &) const;
	void DeleteSslCertificates(const std::list<std::wstring> &ids);
	void ImportSslCertificatePkcs12(
			const std::vector<unsigned char> &certificate,
			const std::string &password,
			std::wstring &error);
	void ImportSslCertificateX509(
			const std::vector<unsigned char> &certificate,
			const std::string &privateKey,
			std::wstring &error);

	void GenerateLicenseKeyRequest(
			const std::string &license,
			std::string &request,
			std::string &privateKey)
		const;
	std::string GetTrialLicense() const;
	std::string GetLicenseKey() const;
	std::string GetLicenseKeyLocalAsymmetricPrivateKey() const;
	void SetLicenseKey(
			const std::string &licenseKey,
			const std::string &privateKey);
	void GetProperties(std::vector<unsigned char> &);

	TunnelEx::LogLevel GetLogLevel() const;
	void SetLogLevel(TunnelEx::LogLevel);

	bool GetUpnpStatus(std::string &externalIp, std::string &localIp) const;

private:

	class Implementation;
	std::auto_ptr<Implementation> m_pimpl;

};

#endif // INCLUDED_FILE__TUNNELEX__TexServiceImplementation_h__0802170531
