/**************************************************************************
 *   Created: 2008/02/17 5:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: TexServiceImplementation.cpp 1127 2011-02-22 17:23:32Z palchukovsky $
 **************************************************************************/

#include "Prec.h"
#include "TexServiceImplementation.hpp"
#include "Licensing.hpp"
#include "ServiceControl/Configuration.hpp"
#include "Legacy/LegacySupporter.hpp"
#include "Modules/Upnp/Client.hpp"
#include "ServiceFilesSecurity.hpp"
#include "Core/Server.hpp"
#include "Core/SslCertificatesStorage.hpp"
#include "Core/Rule.hpp"
#include "Core/Log.hpp"
#include "Core/Exceptions.hpp"

namespace fs = boost::filesystem;
namespace ps = boost::posix_time;
using namespace TunnelEx;
using namespace TunnelEx::Helpers;
using namespace TunnelEx::Helpers::Crypto;

//////////////////////////////////////////////////////////////////////////

class TexServiceImplementation::Implementation : private boost::noncopyable {

public:

	Implementation()
			: m_lastRuleSetModificationTime(0),
			m_lastLicenseKeyModificationTime(0) {
		//...//
	}

	~Implementation() throw() {
		//...//
	}

	void  LoadRules() {
		std::wifstream rulesFile(m_rulesFilePath.c_str(), std::ios::binary | std::ios::in);
		if (!rulesFile) {
			Log::GetInstance().AppendDebug(
				"Could not find rule set file \"%1%\".",
				ConvertString<String>(m_rulesFilePath.c_str()).GetCStr());
			SetRuleSet(std::auto_ptr<RuleSet>(new RuleSet));
			return;
		}
		std::wostringstream rulesXml;
		rulesXml << rulesFile.rdbuf();
		if (!rulesXml.str().empty()) {
			try {
				SetRuleSet(
					std::auto_ptr<RuleSet>(new RuleSet(rulesXml.str().c_str())));
			} catch (const TunnelEx::XmlDoesNotMatchException &) {
				// rules xml file has wrong version
				std::auto_ptr<RuleSet> ruleSet(new RuleSet);
				LegacySupporter().MigrateCurrentRuleSet(*ruleSet);
				SetRuleSet(ruleSet);
			}
		} else {
			SetRuleSet(std::auto_ptr<RuleSet>(new RuleSet));
		}
	}

	void SaveRules() const {
		try {
			const fs::wpath rulesFilePath(m_rulesFilePath);
			fs::create_directories(rulesFilePath.branch_path());
			{
				std::wofstream rulesFile(
					rulesFilePath.string().c_str(),
					std::ios::binary | std::ios::out | std::ios::trunc);
				if (rulesFile) {
					WString rulesXml;
					m_ruleSet->GetXml(rulesXml);
					rulesFile << rulesXml.GetCStr();
				} else {
					Format message("Could not open rule set file \"%1%\".");
					message % ConvertString<String>(m_rulesFilePath.c_str()).GetCStr();
					Log::GetInstance().AppendFatalError(message.str().c_str());
				}
			}
			if (ServiceConfiguration::GetConfigurationFileDir() == rulesFilePath.branch_path()) {
				ServiceFilesSecurity::Set(rulesFilePath.branch_path());
			}
		} catch (const boost::filesystem::filesystem_error &ex) {
			Format message("Could not create directory for rule set file: \"%1%\".");
			message % ex.what();
			Log::GetInstance().AppendSystemError(message.str().c_str());
		}
	}

	ServiceConfiguration & GetConfiguration() {
		if (!m_conf.get()) {
			m_conf = LoadConfiguration();
		}
		return *m_conf;
	}

	SslCertificatesStorage & GetSslCertificatesStorage() {
		if (!m_sslCertificatesStorage.get()) {
			std::vector<unsigned char> key;
			GetSslStorageKey(key);
			m_sslCertificatesStorage.reset(
				new SslCertificatesStorage(
					GetConfiguration().GetCertificatesStoragePath().c_str(),
					&key[0],
					key.size()));
		}
		return *m_sslCertificatesStorage;
	}

	std::auto_ptr<ServiceConfiguration> LoadConfiguration() {
		std::auto_ptr<ServiceConfiguration> result;
		try {
			result.reset(new ServiceConfiguration);
		} catch (const ServiceConfiguration::ConfigurationNotFoundException &) {
			result = ServiceConfiguration::GetDefault();
		} catch (const ServiceConfiguration::ConfigurationHasInvalidFormatException &) {
			result.reset(
				new ServiceConfiguration(
					LegacySupporter().MigrateCurrentServiceConfiguration()));
		} catch (const ServiceConfiguration::ConfigurationException &) {
			//...//
		}
		if (result->IsChanged()) {
			const bool saveResult = result->Save();
			BOOST_ASSERT(saveResult);
			if (saveResult && !ServiceConfiguration::IsTestMode()) {
				ServiceFilesSecurity::Set(ServiceConfiguration::GetConfigurationFileDir());
			}
		}
		return result;
	}

	bool TruncateLog(
				const ServiceConfiguration &configuration,
				uintmax_t &previousLogSize)
			const {
		const std::wstring logPath = configuration.GetLogPath();
		previousLogSize = 0;
		try {
			if (fs::exists(logPath)) {
				previousLogSize = fs::file_size(logPath);
				if (previousLogSize > configuration.GetMaxLogSize()) {
					std::ofstream f(logPath.c_str(), std::ios::trunc);
					return true;
				}
			}
		} catch (const fs::filesystem_error&) {
			//...//
		}
		return false;
	}

	template<class RuleSet>
	bool UpdateRulesState(RuleSet &ruleSet) const {
		const size_t ruleSetSize = ruleSet.GetSize();
		bool hasChanges = false;
		for (size_t i = 0; i < ruleSetSize; ++i) {
			const bool isEnabled
				= Server::GetInstance().IsRuleEnabled(ruleSet[i].GetUuid());
			if (isEnabled != ruleSet[i].IsEnabled()) {
				BOOST_ASSERT(!isEnabled);
				ruleSet[i].Enable(isEnabled);
				hasChanges = true;
			}
		}
		return hasChanges;
	}

	//! Normally this operation required only be the active rules license restriction.
	void UpdateRulesState() {
		std::auto_ptr<RuleSet> rulesPtr(new RuleSet(*m_ruleSet));
		bool hasChanges = false;
		if (UpdateRulesState(rulesPtr->GetServices())) {
			hasChanges = true;
		}
		if (UpdateRulesState(rulesPtr->GetTunnels())) {
			hasChanges = true;
		}
		if (hasChanges) {
			SetRuleSet(rulesPtr);
			SaveRules();
		}
	}

	void SetRuleSet(std::auto_ptr<RuleSet> newRuleSet) throw() {
		m_ruleSet = newRuleSet;
		UpdateLastRuleSetModificationTime();
	}

	void UpdateLastRuleSetModificationTime() {
		m_lastRuleSetModificationTime = time(0);
	}

	void UpdateLastLicenseKeyModificationTime() {
		m_lastLicenseKeyModificationTime
			= Licensing::ServiceKeyRequest::LocalStorage::GetDbFileModificationTime();
	}

	template<class RuleSet>
	bool EnableRule(
				RuleSet &ruleSet,
				const std::wstring &uuid,
				bool wasEnabled,
				bool &changed)
			const {
		const size_t size = ruleSet.GetSize();
		for (size_t i = 0; i < size; ++i) {
			typename RuleSet::ItemType &rule = ruleSet[i];
			if (rule.GetUuid() == uuid.c_str()) {
				if (rule.IsEnabled() != wasEnabled) {
					rule.Enable(wasEnabled);
					if (	Server::GetInstance().IsStarted()
							&& !Server::GetInstance().UpdateRule(rule)) {
						Format message(
							"The rule \"%1%\" has been updated, but some errors has been occurred.");
						message % ConvertString<String>(rule.GetUuid()).GetCStr();
						Log::GetInstance().AppendWarn(message.str());
					}
					changed = true;
				} else {
					changed = false;
				}
				return true;
			}
		}
		changed = false;
		return false;
	}

	template<class RuleSet>
	bool DeleteRule(RuleSet &ruleSet, const std::wstring &uuid) const {
		const size_t size = ruleSet.GetSize();
		for (size_t i = 0; i < size; ++i) {
			if (ruleSet[i].GetUuid() == uuid.c_str()) {
				ruleSet.Remove(i);
				return true;
			}
		}
		return false;
	}

	template<class RuleSet>
	void UpdateRules(const RuleSet &ruleSetWithNew, RuleSet &ruleSet) {
		const size_t size = ruleSetWithNew.GetSize();
		for (unsigned int i = 0; i < size; ++i) {
			const typename RuleSet::ItemType &newRule = ruleSetWithNew[i];
			const WString &u = newRule.GetUuid();
			const size_t currentRulesNumb = ruleSet.GetSize();
			bool wasFound = false;
			for	(size_t j = 0; j < currentRulesNumb; ++j) {
				if (ruleSet[j].GetUuid() == u) {
					ruleSet[j] = newRule;
					wasFound = true;
					break;
				}
			}
			if (!wasFound) {
				ruleSet.Append(newRule);
			}
			if (	Server::GetInstance().IsStarted()
					&& !Server::GetInstance().UpdateRule(newRule)) {
				Format message(
					"The rule \"%1%\" has been updated, but some errors has been occurred.");
				message % ConvertString<String>(u).GetCStr();
				Log::GetInstance().AppendWarn(message.str());
			}
		}
	}

	void GetSslStorageKey(std::vector<unsigned char> &result) {
		/*
			<)g<D}GhTFXOByZneW8zJA`53u(Ddk6P&Mh!,G8oRra{g]UrGgi%bRl{=P$u,'^@Iln4QkW;vK/
			length: 75
		 */
		std::vector<unsigned char> key;
		key.resize(27); key[26] = '('; key.resize(51); key[50] = 'i';
		key[0] = '<'; key[22] = '`'; key[18] = '8'; key[32] = '&';
		key[33] = 'M'; key[17] = 'W'; key[8] = 'T'; key[4] = 'D';
		key[38] = '8'; key[34] = 'h'; key.resize(55); key[54] = 'l';
		key.resize(57); key[56] = '='; key[2] = 'g'; key[15] = 'n';
		key.resize(58); key[57] = 'P'; key.resize(64); key[63] = '@';
		key[7] = 'h'; key[14] = 'Z'; key.resize(72); key[71] = ';';
		key[55] = '{'; key[39] = 'o'; key[40] = 'R'; key.resize(75);
		key[74] = '/'; key[16] = 'e'; key[36] = ','; key[12] = 'B';
		key[24] = '3'; key[23] = '5'; key[11] = 'O'; key[30] = '6';
		key[6] = 'G'; key[20] = 'J'; key[1] = ')'; key[60] = ',';
		key[25] = 'u'; key[62] = '^'; key[44] = 'g'; key[29] = 'k';
		key[51] = '%'; key[5] = '}'; key[47] = 'r'; key[49] = 'g';
		key[67] = '4'; key[21] = 'A'; key[65] = 'l'; key[41] = 'r';
		key[52] = 'b'; key[68] = 'Q'; key[43] = '{'; key[19] = 'z';
		key[27] = 'D'; key[10] = 'X'; key[46] = 'U'; key[31] = 'P';
		key[45] = ']'; key[69] = 'k'; key[3] = '<'; key[70] = 'W';
		key[35] = '!'; key[72] = 'v'; key[58] = '$'; key[13] = 'y';
		key[59] = 'u'; key[61] = '\''; key[48] = 'G'; key[64] = 'I';
		key[42] = 'a'; key[9] = 'F'; key[53] = 'R'; key[73] = 'K';
		key[66] = 'n'; key[37] = 'G'; key[28] = 'd'; 
		key.swap(result);
	}

public:

	std::auto_ptr<RuleSet> m_ruleSet;
	time_t m_lastRuleSetModificationTime;
	time_t m_lastLicenseKeyModificationTime;
	std::wstring m_rulesFilePath;
	std::wstring m_logFilePath;
	boost::mutex m_mutex;
	std::auto_ptr<ServiceConfiguration> m_conf;
	std::auto_ptr<SslCertificatesStorage> m_sslCertificatesStorage;

};

//////////////////////////////////////////////////////////////////////////

TexServiceImplementation::TexServiceImplementation()
		: m_pimpl(new Implementation) {

	ServiceConfiguration &conf = m_pimpl->GetConfiguration();

	uintmax_t previousLogSize;
	const bool logWasTruncated = m_pimpl->TruncateLog(conf, previousLogSize);
	Log::GetInstance().AttachFile(conf.GetLogPath());
#	if !defined(_DEBUG) && !defined(TEST)
		if (conf.GetLogLevel() == TunnelEx::LOG_LEVEL_DEBUG) {
			conf.SetLogLevel(TunnelEx::LOG_LEVEL_INFO);
			conf.Save();
		}
#	endif
	Log::GetInstance().SetMinimumRegistrationLevel(conf.GetLogLevel());
	Log::GetInstance().AppendInfo(
		ConvertString<String>(L"Logging stated: " TUNNELEX_NAME_W L" " TUNNELEX_VERSION_FULL_W TUNNELEX_BUILD_IDENTITY_ADD_W).GetCStr());
	if (logWasTruncated) {
		Log::GetInstance().AppendInfo(
			(Format("Log was truncated, previous size - %1% bytes.") % previousLogSize).str());
	}

	m_pimpl->m_rulesFilePath = conf.GetRulesPath();
	m_pimpl->m_logFilePath = conf.GetLogPath();

	m_pimpl->LoadRules();

	if (conf.IsServerStarted()) {
		Start();
	}

}

TexServiceImplementation::~TexServiceImplementation() {
	LogTracking("TexServiceImplementation", "~TexServiceImplementation", __FILE__, __LINE__);
}

bool TexServiceImplementation::Start() {
	LogTracking("TexServiceImplementation", "StartServer", __FILE__, __LINE__);
	if (!Server::GetInstance().IsStarted()) {
		boost::mutex::scoped_lock lock(m_pimpl->m_mutex);
		if (!Server::GetInstance().IsStarted()) {
			try {
				Server::GetInstance().Start(
					*m_pimpl->m_ruleSet,
					m_pimpl->GetSslCertificatesStorage());
			} catch (const ::TunnelEx::LocalException &ex) {
				Format message("Could not start the server: \"%1%\".");
				message % ConvertString<String>(ex.GetWhat()).GetCStr();
				Log::GetInstance().AppendFatalError(message.str());
			}
		}
	}
	const bool isStarted = Server::GetInstance().IsStarted();
	if (isStarted) {
		m_pimpl->UpdateRulesState();
	}
	return isStarted;
}

bool TexServiceImplementation::Stop() {
	LogTracking("TexServiceImplementation", "StopServer", __FILE__, __LINE__);
	if (Server::GetInstance().IsStarted()) {
		boost::mutex::scoped_lock lock(m_pimpl->m_mutex);
		if (Server::GetInstance().IsStarted()) {
			try {
				Server::GetInstance().Stop();
			} catch (const ::TunnelEx::LocalException &ex) {
				Format message("Could not stop the server: \"%1%\".");
				message % ConvertString<String>(ex.GetWhat()).GetCStr();
				Log::GetInstance().AppendFatalError(message.str());
			}
		}
	}
	return !Server::GetInstance().IsStarted();
}

bool TexServiceImplementation::IsStarted() const {
	return Server::GetInstance().IsStarted();
}

void TexServiceImplementation::GetRuleSet(std::string &result) const {
	WString buffer;
	try {
		boost::mutex::scoped_lock lock(m_pimpl->m_mutex);
		m_pimpl->m_ruleSet->GetXml(buffer);
	} catch (const TunnelEx::LocalException &ex) {
		Format message("Could not serialize rules list into XML: \"%1%\".");
		message % ConvertString<String>(ex.GetWhat()).GetCStr();
		Log::GetInstance().AppendFatalError(message.str());
	}
	result = ConvertString<String>(buffer).GetCStr();
}

//! @todo: make exception-safe
void TexServiceImplementation::UpdateRules(const std::wstring &xml) {
	try {
		RuleSet ruleSet(xml.c_str());
		boost::mutex::scoped_lock lock(m_pimpl->m_mutex);
		m_pimpl->UpdateRules(ruleSet.GetServices(), m_pimpl->m_ruleSet->GetServices());
		m_pimpl->UpdateRules(ruleSet.GetTunnels(), m_pimpl->m_ruleSet->GetTunnels());
		m_pimpl->SaveRules();
		m_pimpl->UpdateLastRuleSetModificationTime();
	} catch (const ::TunnelEx::LocalException &ex) {
		Format message("Could not update rules: %1%.");
		message % ConvertString<String>(ex.GetWhat()).GetCStr();
		Log::GetInstance().AppendError(message.str());
		m_pimpl->UpdateRulesState();
		return;
	}
}

void TexServiceImplementation::EnableRules(
			const std::list<std::wstring> &uuids,
			bool isEnabled) {
	try {
		bool wasChanged = false;
		const std::list<std::wstring>::const_iterator end = uuids.end();
		boost::mutex::scoped_lock lock(m_pimpl->m_mutex);
		foreach (const std::wstring &u, uuids) {
			bool wasChangedNow = false;
			m_pimpl->EnableRule(m_pimpl->m_ruleSet->GetServices(), u, isEnabled, wasChangedNow)
				|| m_pimpl->EnableRule(m_pimpl->m_ruleSet->GetTunnels(), u, isEnabled, wasChangedNow);
			if (wasChangedNow) {
				wasChanged = true;
			}
		}
		if (wasChanged) {
			m_pimpl->SaveRules();
			m_pimpl->UpdateLastRuleSetModificationTime();
		}
	} catch (const ::TunnelEx::LocalException &ex) {
		Format message("Could not enable rules: %1%.");
		message % ConvertString<String>(ex.GetWhat()).GetCStr();
		Log::GetInstance().AppendError(message.str());
		m_pimpl->UpdateRulesState();
		return;
	}
}

//! @todo: make exception-safe
void TexServiceImplementation::DeleteRules(const std::list<std::wstring> &uuids) {
	bool wasFound = false;
	boost::mutex::scoped_lock lock(m_pimpl->m_mutex);
	foreach (const std::wstring &u, uuids) {
		if (Server::GetInstance().IsStarted()) {
			Server::GetInstance().DeleteRule(u.c_str());
		}
		if (	m_pimpl->DeleteRule(m_pimpl->m_ruleSet->GetServices(), u)
				|| m_pimpl->DeleteRule(m_pimpl->m_ruleSet->GetTunnels(), u)) {
			wasFound = true;
		}
	}
	if (wasFound) {
		m_pimpl->SaveRules();
		m_pimpl->UpdateLastRuleSetModificationTime();
	}
}

void TexServiceImplementation::SaveRunningState() const {
	LogTracking("TexServiceImplementation", "SaveServerState", __FILE__, __LINE__);
	ServiceConfiguration &conf = m_pimpl->GetConfiguration();
	try {
		conf.SetServerStarted(Server::GetInstance().IsStarted());
		conf.Save();
	} catch (const ServiceConfiguration::ConfigurationException &) {
		Log::GetInstance().AppendFatalError(
			"Could not save server state into service configuration file.");
	}
}

LogLevel TexServiceImplementation::GetLogLevel() const {
	return m_pimpl->GetConfiguration().GetLogLevel();
}

void TexServiceImplementation::SetLogLevel(LogLevel level) {
	ServiceConfiguration &conf = m_pimpl->GetConfiguration();
	try {
		conf.SetLogLevel(level);
		Log::GetInstance().SetMinimumRegistrationLevel(conf.GetLogLevel());
		conf.Save();
	} catch (const ServiceConfiguration::ConfigurationException &) {
		Log::GetInstance().AppendFatalError(
			"Could not save server state into service configuration file.");
	}
}

void TexServiceImplementation::GetNetworkAdapters(
			std::list<texs__NetworkAdapterInfo> &result)
		const {

	std::list<texs__NetworkAdapterInfo> adapters;

	{
		texs__NetworkAdapterInfo all;
		all.id = "all";
		all.name = "All available locally";
		all.ipAddress = "*";
		adapters.push_back(all);
	}

	{
		texs__NetworkAdapterInfo loopback;
		loopback.id = "loopback";
		loopback.name = "Loopback";
		loopback.ipAddress = "127.0.0.1";
		adapters.push_back(loopback);
	}
	
	std::vector<unsigned char> adaptersInfo(sizeof(IP_ADAPTER_INFO));
	ULONG adaptersInfoBufferLen = ULONG(adaptersInfo.size());
	if (	GetAdaptersInfo(
				reinterpret_cast<PIP_ADAPTER_INFO>(&adaptersInfo[0]),
				&adaptersInfoBufferLen)
			== ERROR_BUFFER_OVERFLOW) {
		adaptersInfo.resize(adaptersInfoBufferLen);
	}

	if (	GetAdaptersInfo(
				reinterpret_cast<PIP_ADAPTER_INFO>(&adaptersInfo[0]),
				&adaptersInfoBufferLen)
			== NO_ERROR) {
		PIP_ADAPTER_INFO adapter = reinterpret_cast<PIP_ADAPTER_INFO>(&adaptersInfo[0]);
		while (adapter) {
			std::string ipAddress = adapter->IpAddressList.IpAddress.String;
			const bool isIpValid = !ipAddress.empty() && ipAddress != "0.0.0.0";
			switch (adapter->Type) {
				default:
					if (!isIpValid) {
						break;
					}
				case MIB_IF_TYPE_ETHERNET:
				case MIB_IF_TYPE_PPP:
				case IF_TYPE_IEEE80211:
				case IF_TYPE_IEEE80212:
				case IF_TYPE_FASTETHER:
				case IF_TYPE_FASTETHER_FX:
					{
						texs__NetworkAdapterInfo info;
						info.id = adapter->AdapterName;
						info.name = adapter->Description;
						if (isIpValid) {
							info.ipAddress.swap(ipAddress);
						}
						adapters.push_back(info);
					}
					break;
			}
			adapter = adapter->Next;
		}
	}

	adapters.swap(result);

}

void TexServiceImplementation::GetLastLogRecords(
			unsigned int recordsNumber,
			std::list<texs__LogRecord> &destination)
		const {
	std::ifstream log(m_pimpl->m_logFilePath.c_str(), std::ios::in | std::ios::binary);
	log.seekg(-1, std::ios::end);
	unsigned int passedLines = 0;
	bool isContentStarted = false;
	for (std::streamsize i = 1; ; log.seekg(-(++i), std::ios::end)) {
		if (!log) {
			log.clear();
			log.seekg(-(i - 1), std::ios::end);
			break;
		}
		char c;
		log.get(c);
		if (c == '\n') {
			if (isContentStarted && ++passedLines >= recordsNumber) {
				break;
			}
		} else if (!isContentStarted) {
			isContentStarted = true;
		}
	}
	const unsigned short bufferStep
#		ifdef _DEBUG
			= 10;
#		else
			= 512;
#		endif // _DEBUG
	std::vector<char> buffer(bufferStep, 0);
	size_t pos = 0;
	boost::regex recordExp(
		"(\\d{4,4}\\-[a-z]{3,3}\\-\\d{2,2}\\s\\d{2,2}:\\d{2,2}:\\d{2,2}.\\d{6,6})\\s+([a-z]+(\\s[a-z]+)?):\\s([^\\r\\n]*)[\\r\\n\\t\\s]*",
		boost::regex::perl | boost::regex::icase);
	while (!log.getline(&buffer[pos], std::streamsize(buffer.size() - pos)).eof()) {
		if (log.fail()) {
			pos = buffer.size() - 1;
			buffer.resize(buffer.size() + bufferStep);
			log.clear(log.rdstate() & ~std::ios::failbit);
		} else {
			pos = 0;
			boost::cmatch what;
			if (boost::regex_match(&buffer[0], what, recordExp)) {
				texs__LogRecord record;
				record.time = what[1];
				record.level = Log::GetInstance().ResolveLevel(what[2].str().c_str());
				record.message = what[4];
				destination.push_back(record);
			}
		}
	}
}

bool TexServiceImplementation::Migrate() {
	LegacySupporter().MigrateAllAndSave();
	m_pimpl->LoadRules();
	return true;
}

void TexServiceImplementation::CheckState(texs__ServiceState &result) const {
	//! @todo: implement cache here, for situation with many clients.
	result.errorTime = Log::GetInstance().GetLastErrorTime();
	result.warnTime = Log::GetInstance().GetLastWarnTime();
	result.logSize = Log::GetInstance().GetSize();
	result.ruleSetTime = m_pimpl->m_lastRuleSetModificationTime;
	result.licKeyTime = m_pimpl->m_lastLicenseKeyModificationTime;
	result.isStarted = Server::GetInstance().IsStarted();
}

void TexServiceImplementation::GenerateLicenseKeyRequest(
			const std::string &license,
			std::string &request,
			std::string &privateKey)
		const {
	Licensing::ServiceKeyRequest::RequestGeneration::Generate(
		license,
		request,
		privateKey,
		boost::any());
}

std::string TexServiceImplementation::GetTrialLicense() const {
	return Licensing::ServiceKeyRequest::LocalStorage::GetTrialLicense();
}

std::string TexServiceImplementation::GetLicenseKey() const {
	return Licensing::ServiceKeyRequest::LocalStorage::GetLicenseKey();
}

std::string TexServiceImplementation::GetLicenseKeyLocalAsymmetricPrivateKey() const {
	return Licensing::ServiceKeyRequest::LocalStorage::GetLocalAsymmetricPrivateKey();
}

void TexServiceImplementation::SetLicenseKey(
			const std::string &licenseKey,
			const std::string &privateKey) {
	Licensing::ServiceKeyRequest::LocalStorage::StoreLicenseKey(licenseKey, privateKey);
	m_pimpl->UpdateLastLicenseKeyModificationTime();
}

void TexServiceImplementation::GetProperties(std::vector<unsigned char> &result) {
	
	using namespace Licensing;
	typedef boost::int8_t Result;
	typedef boost::int32_t ValSize;
	typedef boost::int32_t Id;

	std::vector<unsigned char> notEncryptedBuffer;
	notEncryptedBuffer.resize(sizeof(Result));

	WorkstationPropertyValues props;
	if (ExeLicense::WorkstationPropertiesLocal::Get(props, boost::any())) {
		
		reinterpret_cast<Result &>(*&notEncryptedBuffer[0]) = Result(true);

		foreach (const WorkstationPropertyValues::value_type &prop, props) {
			size_t i = notEncryptedBuffer.size();
			notEncryptedBuffer.resize(
				notEncryptedBuffer.size() + sizeof(Id) + sizeof(ValSize) + prop.second.size());
			reinterpret_cast<Id &>(*&notEncryptedBuffer[i]) = Id(prop.first);
			i += sizeof(Id);
			reinterpret_cast<ValSize &>(*&notEncryptedBuffer[i]) = ValSize(prop.second.size());
			i += sizeof(ValSize);
			memcpy(&notEncryptedBuffer[i], prop.second.c_str(), prop.second.size());
			BOOST_ASSERT(i + prop.second.size() == notEncryptedBuffer.size());
		}

	} else {
		reinterpret_cast<Result &>(*&notEncryptedBuffer[0]) = Result(false);
	}

	std::vector<unsigned char> encryptedBuffer(notEncryptedBuffer.size());
	std::vector<unsigned char>::iterator i = encryptedBuffer.begin();
	std::vector<unsigned char> key;
	LocalComunicationPolicy::GetEncryptingKey(key);
	size_t token = 0;
	foreach (char ch, notEncryptedBuffer) {
		ch ^= key[token++ % key.size()];
		*i = ch;
		++i;
	}

	encryptedBuffer.swap(result);

}

bool TexServiceImplementation::GetUpnpStatus(
			std::string &externalIp,
			std::string &localIp)
		const {
	try {
		Mods::Upnp::Client upnpc;
		std::string externalIpTmp = upnpc.GetExternalIpAddress();
		std::string localIpTmp = upnpc.GetLocalIpAddress();
		externalIpTmp.swap(externalIp);
		localIpTmp.swap(localIp);
		return true;
	} catch (const ::TunnelEx::Mods::Upnp::Client::DeviceNotExistException &) {
		//...//
	} catch (const ::TunnelEx::LocalException &ex) {
		Log::GetInstance()
			.AppendError(ConvertString<String>(ex.GetWhat()).GetCStr());
	}
	return false;
}

void TexServiceImplementation::GetSslCertificate(
			const std::wstring &id,
			texs__SslCertificateInfo &result)
		const {
	try {
		const UniquePtr<const X509Shared> certificate 
			= m_pimpl->GetSslCertificatesStorage().GetCertificate(id.c_str());
		texs__SslCertificateInfo certificateInfo;
		certificateInfo.id = id;
		certificateInfo.keySize = certificate->GetPublicKey().GetSize();
		certificateInfo.isPrivate
			= m_pimpl->GetSslCertificatesStorage().IsPrivateCertificate(id.c_str());
		certificateInfo.serial = certificate->GetSerialNumber();
		certificateInfo.validAfterTimeUtc = certificate->GetValidAfterTimeUtc();
		certificateInfo.validBeforeTimeUtc = certificate->GetValidBeforeTimeUtc();
		certificateInfo.subjectCommonName = certificate->GetSubjectCommonName();
		certificateInfo.issuerCommonName = certificate->GetIssuerCommonName();
		certificateInfo.subjectOrganization = certificate->GetSubjectOrganization();
		certificateInfo.subjectOrganizationUnit
			= certificate->GetSubjectOrganizationUnit();
		certificateInfo.subjectCity = certificate->GetSubjectCity();
		certificateInfo.subjectStateOrProvince
			= certificate->GetSubjectStateOrProvince();
		certificateInfo.subjectCountry = certificate->GetSubjectCountry();
		certificate->Print(certificateInfo.fullInfo);
		certificateInfo.certificate = certificate->Export();
		result = certificateInfo;
	} catch (const TunnelEx::LocalException &ex) {
		Format message("Could not get SSL certificate %1%: %2%.");
		message % ConvertString<String>(id.c_str()).GetCStr();
		message % ConvertString<String>(ex.GetWhat()).GetCStr();
		Log::GetInstance().AppendError(message.str());
		result = texs__SslCertificateInfo();
	} catch (const TunnelEx::Helpers::Crypto::Exception &ex) {
		Format message("Could not get SSL certificate info: %1%.");
		message % ex.what();
		Log::GetInstance().AppendError(message.str());
	}

}

void TexServiceImplementation::GetSslCertificates(
			std::list<texs__SslCertificateShortInfo> &result)
		const {

	try {
	
		for ( ; ; ) {

			std::list<texs__SslCertificateShortInfo> certificates;
			const UniquePtr<const SslCertificateIdCollection> ids
				= m_pimpl->GetSslCertificatesStorage().GetInstalledIds();
		
			try {
				
				for (size_t i = 0; i < ids->GetSize(); ++i) {
					const WString &id = (*ids)[i];
					try {
						const UniquePtr<const X509Shared> certificate 
							= m_pimpl->GetSslCertificatesStorage().GetCertificate(id);
						texs__SslCertificateShortInfo certificateInfo;
						certificateInfo.id = id.GetCStr();
						certificateInfo.isPrivate = m_pimpl
							->GetSslCertificatesStorage()
							.IsPrivateCertificate(id);
						certificateInfo.validBeforeTimeUtc
							= certificate->GetValidBeforeTimeUtc();
						certificateInfo.subjectCommonName
							= certificate->GetSubjectCommonName();
						certificateInfo.issuerCommonName
							= certificate->GetIssuerCommonName();
						certificates.push_back(certificateInfo);
					} catch (const TunnelEx::Helpers::Crypto::Exception &ex) {
						Format message("Could not get info for SSL certificate %1%: %2%.");
						message % ConvertString<String>(id).GetCStr();
						message % ex.what();
						Log::GetInstance().AppendError(message.str());
					}
				}

			} catch (const NotFoundException &) {
				Log::GetInstance().AppendDebug("SSL certificates list has been changed, reloading...");
				continue;
			}

			certificates.swap(result);
			return;

		}

	} catch (const TunnelEx::LocalException &ex) {
		Format message("Could not get SSL certificates list: %1%.");
		message % ConvertString<String>(ex.GetWhat()).GetCStr();
		Log::GetInstance().AppendError(message.str());
		std::list<texs__SslCertificateShortInfo>().swap(result);
	}

}

void TexServiceImplementation::DeleteSslCertificates(const std::list<std::wstring> &ids) {
	SslCertificateIdCollection idsForStorage(ids.size());
	foreach (const std::wstring &id, ids) {
		idsForStorage.Append(id.c_str());
	}
	try {
		m_pimpl->GetSslCertificatesStorage().Delete(idsForStorage);
	} catch (const TunnelEx::LocalException &ex) {
		Format message("Could not delete SSL certificates: %1%.");
		message % ConvertString<String>(ex.GetWhat()).GetCStr();
		Log::GetInstance().AppendError(message.str());
	}
}

void TexServiceImplementation::ImportSslCertificateX509(
			const std::vector<unsigned char> &certificate,
			const std::string &privateKeyStr,
			std::wstring &error) {
	
	Log::GetInstance().AppendDebug("Importing SSL certificate...");

	std::auto_ptr<PrivateKey> privateKey;
	if (!privateKeyStr.empty()) {
		Log::GetInstance().AppendDebug("Importing SSL certificate private key...");
		try {
			privateKey.reset(new PrivateKey(privateKeyStr));
			Log::GetInstance().AppendDebug("SSL certificate private key successfully parsed...");
		} catch (const TunnelEx::Helpers::Crypto::Exception &ex) {
			Log::GetInstance().AppendDebug(
				"Failed to parse SSL certificate private key: %1%.",
				ex.what());
			error = L"Could not parse the private key. Please check the file format.";
			return;
		}
	}

	std::auto_ptr<X509Shared> x509;
	Log::GetInstance().AppendDebug("Importing X.509 SSL certificate...");
	try {
		if (privateKey.get()) {
			x509.reset(new X509Private(&certificate[0], certificate.size(), *privateKey));
		} else {
			x509.reset(new X509Shared(&certificate[0], certificate.size()));
		}
		Log::GetInstance().AppendDebug("X.509 SSL certificate successfully parsed...");
	} catch (const TunnelEx::Helpers::Crypto::Exception &ex) {
		Log::GetInstance().AppendDebug(
			"Failed to parse X.509: %1%.",
			ex.what());
		error = L"Could not parse the X.509 SSL certificate. Please check the file format.";
		return;
	}

	BOOST_ASSERT(x509.get() != 0 || privateKey.get() == 0);

	try {
		Log::GetInstance().AppendDebug("Installing public SSL certificate...");
		if (privateKey.get()) {
			m_pimpl->GetSslCertificatesStorage().Insert(
				*boost::polymorphic_downcast<X509Private *>(x509.get()));
		} else {
			m_pimpl->GetSslCertificatesStorage().Insert(*x509);
		}
	} catch (const TunnelEx::Helpers::Crypto::Exception &ex) {
		Format message("Could not install SSL certificate: %1%.");
		message % ex.what();
		Log::GetInstance().AppendDebug(message.str());
		error = ConvertString<WString>(message.str().c_str()).GetCStr();
	} catch (const TunnelEx::LocalException &ex) {
		Format message("Could not import and install SSL certificate: %1%.");
		message % ConvertString<String>(ex.GetWhat()).GetCStr();
		Log::GetInstance().AppendError(message.str());
	}

}

void TexServiceImplementation::ImportSslCertificatePkcs12(
			const std::vector<unsigned char> &certificate,
			const std::string &password,
			std::wstring &error) {
	
	Log::GetInstance().AppendDebug("Importing PKCS12...");

	std::auto_ptr<Pkcs12> pkcs12;
	try {
		pkcs12.reset(new Pkcs12(&certificate[0], certificate.size()));
	} catch (const TunnelEx::Helpers::Crypto::Exception &ex) {
		Log::GetInstance().AppendDebug("Failed to parse PKCS12: %1%.", ex.what());
		error = L"Could not parse SSL certificate. Please check the file format.";
		return;
	}

	BOOST_ASSERT(pkcs12.get() != 0);
	
	std::auto_ptr<const X509Private> x509;
	try {
		Log::GetInstance().AppendDebug("Checking SSL certificate password...");
		x509 = pkcs12->GetCertificate(password);
	} catch (const TunnelEx::Helpers::Crypto::Exception &ex) {
		Log::GetInstance().AppendDebug(
			"Failed to check SSL certificate (%1%)...",
			ex.what());
		error
			= L"Could not parse SSL certificate."
				L" Please check the file format and password.";
		return;
	}

	BOOST_ASSERT(x509.get() != 0);
		
	Log::GetInstance().AppendDebug("Installing private SSL certificate...");
	try {
		m_pimpl->GetSslCertificatesStorage().Insert(*x509);
	} catch (const TunnelEx::Helpers::Crypto::Exception &ex) {
		Format message("Could not install SSL certificate: %1%.");
		message % ex.what();
		Log::GetInstance().AppendDebug(message.str());
		error = ConvertString<WString>(message.str().c_str()).GetCStr();
	} catch (const TunnelEx::LocalException &ex) {
		Format message("Could not import and install SSL certificate: %1%.");
		message % ConvertString<String>(ex.GetWhat()).GetCStr();
		Log::GetInstance().AppendError(message.str());
	}
	
}
