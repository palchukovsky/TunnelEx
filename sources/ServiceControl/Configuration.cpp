/**************************************************************************
 *   Created: 2007/12/27 22:48
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Configuration.cpp 1107 2010-12-20 12:24:07Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "Configuration.hpp"
#include "Legacy/LegacySupporter.hpp"

#include "Core/Log.hpp"
#include "Core/String.hpp"

namespace fs = boost::filesystem;
using namespace TunnelEx;
using namespace TunnelEx::Helpers;
using namespace TunnelEx::Helpers::Xml;

//////////////////////////////////////////////////////////////////////////

class ServiceConfiguration::Implementation {

public:

	explicit Implementation(boost::shared_ptr<const Document> doc)
			: m_isChanged(true) {
		Init(boost::shared_ptr<Document>(new Document(*doc)));
	}

	explicit Implementation(const std::wstring &configurationFilePath)
			: m_isChanged(false) {
		if (!fs::exists(configurationFilePath)) {
			throw ConfigurationNotFoundException();
		}
		boost::shared_ptr<Document> doc;
		try {
			doc = Document::LoadFromFile(configurationFilePath);
		} catch (const Document::ParseException &ex) {
			const char *const message
				= "Could not parse XML-file with service configuration, "
					"file has invalid format, empty or access is denied (\"%1%\").";
			Log::GetInstance().AppendError((Format(message) % ex.what()).str());
			throw ServiceConfiguration::ConfigurationHasInvalidFormatException();
		}
		Init(doc);
	}

	explicit Implementation(DllObjectPtr<ServiceConfiguration> rhs) 
			: m_doc(Document::CreateDuplicate(*rhs.Get().m_pimpl->m_doc)),
			m_isChanged(rhs.Get().m_pimpl->m_isChanged) {
		//...//
	}

	Implementation(const Implementation &rhs)
			: m_doc(new Document(*rhs.m_doc)),
			m_isChanged(rhs.m_isChanged) {
		//...//
	}

	~Implementation() {
		/*...*/
	}

private:

	const Implementation & operator=(const Implementation &);

protected:

	void Init(boost::shared_ptr<Document> doc) {
		try {
			Schema schema(GetSchemaFilePath());
			std::string validateErrors;
			if (!schema.Validate(*doc, &validateErrors)) {
				const char *const message
					= "Passed XML-string has invalid format "
						"and can not be loaded as service configuration (\"%1%\").";
				Log::GetInstance().AppendError((Format(message) % validateErrors).str());
				throw ServiceConfiguration::ConfigurationHasInvalidFormatException();
			}
		} catch (const Document::ParseException &ex) {
			const char *const message
				= "Could not load system XML-Schema file "
					"for service configuration: \"%1%\".";
			Log::GetInstance().AppendError((Format(message) % ex.what()).str());
			throw ServiceConfiguration::ConfigurationHasInvalidFormatException();
		}
		m_doc = doc;
	}

private:

	boost::shared_ptr<Document> m_doc;
	bool m_isChanged;

protected:

	boost::shared_ptr<Node> GetNode(Document &doc, const char *tag) {
		std::string query = "//Configuration[@Version = \"1.2\"]/";
		query += tag;
		NodeCollection queryResult;
		doc.GetXPath()->Query(query.c_str(), queryResult);
		return queryResult[0];
	}

	boost::shared_ptr<Node> GetNode(const char *tag) const {
		return const_cast<Implementation *>(this)->GetNode(*m_doc, tag);
	}

	boost::shared_ptr<Node> GetNode(const char *tag) {
		return GetNode(*m_doc, tag);
	}

	void ValidateDocAndThrow(Document &doc) {
		try {
			Schema schema(GetSchemaFilePath());
			if (!schema.Validate(doc)) {
				throw ServiceConfiguration::ConfigurationHasInvalidFormatException();
			}
		} catch (const Document::ParseException&) {
			throw ServiceConfiguration::ConfigurationException();
		}
	}

	void SetNodeContent(const char *tag, const std::wstring &content) {
		boost::shared_ptr<Document> newDoc = Document::CreateDuplicate(*m_doc);
		GetNode(*newDoc, tag)->SetContent(content);
		ValidateDocAndThrow(*newDoc);
		m_doc = newDoc;
		m_isChanged = true;
	}

	void SetNodeAttribute(const char *tag, const char *attribute, const std::wstring &content) {
		boost::shared_ptr<Document> newDoc = Document::CreateDuplicate(*m_doc);
		GetNode(*newDoc, tag)->SetAttribute(attribute, content);
		ValidateDocAndThrow(*newDoc);
		m_doc = newDoc;
		m_isChanged = true;
	}

	typedef std::map<std::wstring, LogLevel> LogLevelsNames;
	void Fill(LogLevelsNames &names) const {
		names.clear();
		names.insert(make_pair(std::wstring(L"track"), LOG_LEVEL_TRACK));
		names.insert(make_pair(std::wstring(L"debug"), LOG_LEVEL_DEBUG));
		names.insert(make_pair(std::wstring(L"information"), LOG_LEVEL_INFO));
		names.insert(make_pair(std::wstring(L"warning"), LOG_LEVEL_WARN));
		names.insert(make_pair(std::wstring(L"error"), LOG_LEVEL_ERROR));
	}

public:

	std::wstring GetLogPath() const {
		std::wstring buffer;
		return GetNode("Log")->GetContent(buffer);
	}
	
	void SetLogPath(const std::wstring& path) {
		SetNodeContent("Log", path);
	}

	LogLevel GetLogLevel() const {
		LogLevelsNames levels;
		Fill(levels);
		std::wstring buffer;
		return levels.find(GetNode("Log")->GetAttribute("Level", buffer))->second;
	}

	void SetLogLevel(LogLevel level) {
		if (level == LOG_LEVEL_FATAL_ERROR || level == LOG_LEVEL_SYSTEM_ERROR) {
			level = LOG_LEVEL_ERROR;
		}
		LogLevelsNames levels;
		Fill(levels);
		const LogLevelsNames::const_iterator end = levels.end();
		for (LogLevelsNames::const_iterator i = levels.begin(); i != end; ++i) {
			if (i->second == level) {
				SetNodeAttribute("Log", "Level", i->first);
				return;
			}
		}
		throw ServiceConfiguration::ConfigurationHasInvalidFormatException();
	}

	std::wstring GetRulesPath() const {
		std::wstring buffer;
		return GetNode("Rules")->GetContent(buffer);
	}

	void SetRulesPath(const std::wstring &path) {
		SetNodeContent("Rules", path);
	}

	std::wstring GetCertificatesStoragePath() const {
		std::wstring buffer;
		return GetNode("CertificatesStorage")->GetContent(buffer);
	}

	void SetCertificatesStoragePath(const std::wstring &path) {
		SetNodeContent("CertificatesStorage", path);
	}

	unsigned long GetMaxLogSize() const {
		std::wstring buffer;
		try {
			return boost::lexical_cast<unsigned long>(GetNode("Log")->GetAttribute("MaxSize", buffer));
		} catch (const boost::bad_lexical_cast &) {
			return std::numeric_limits<unsigned long>::max();
		}
	}

	void SetMaxLogSize(unsigned long size) {
		SetNodeAttribute("Log", "MaxSize", boost::lexical_cast<std::wstring>(size));
	}

	bool IsServerStarted() const {
		std::wstring buffer;
		return GetNode("ServerState")->GetContent(buffer) == L"started";
	}

	void SetServerStarted(bool val) {
		SetNodeContent("ServerState", val ? L"started" : L"stopped");
	}

	bool Save(const std::wstring &confFilePath) {
		fs::create_directories(fs::wpath(confFilePath).branch_path());
		return m_doc->Save(confFilePath);
	}

	bool IsChanged() const {
		return m_isChanged;
	}

	static std::string GetSchemaFilePath() {
		fs::path file = Helpers::GetModuleFilePathA().branch_path();
		file /= "ServiceConfiguration.xsd";
		return file.string();
	}

};

//////////////////////////////////////////////////////////////////////////

ServiceConfiguration::ServiceConfiguration(boost::shared_ptr<const Document> doc)
		: m_pimpl(new Implementation(doc)) {
	//...//
}

ServiceConfiguration::ServiceConfiguration(const wchar_t *confFilePath)
		: m_pimpl(
			new Implementation(
				confFilePath
					?	confFilePath
					:	GetConfigurationFilePath())) {
	//...//
}

ServiceConfiguration::ServiceConfiguration(DllObjectPtr<ServiceConfiguration> rhs) 
		: m_pimpl(new Implementation(rhs)) {
	//...//
}

ServiceConfiguration::ServiceConfiguration(const ServiceConfiguration &rhs)
		: m_pimpl(new Implementation(*rhs.m_pimpl)) {
	//...//
}

const ServiceConfiguration & ServiceConfiguration::operator =(const ServiceConfiguration &rhs) {
	ServiceConfiguration(rhs).Swap(*this);
	return *this;
}

void ServiceConfiguration::Swap(ServiceConfiguration &rhs) {
	std::auto_ptr<Implementation> tmpImpl(m_pimpl);
	m_pimpl = rhs.m_pimpl;
	rhs.m_pimpl = tmpImpl;
}

ServiceConfiguration::~ServiceConfiguration() {
	//...//
}

std::wstring ServiceConfiguration::GetLogPath() const {
	return m_pimpl->GetLogPath();
}

void ServiceConfiguration::SetLogPath(const std::wstring &path) {
	m_pimpl->SetLogPath(path);
}

LogLevel ServiceConfiguration::GetLogLevel() const {
	return m_pimpl->GetLogLevel();
}

void ServiceConfiguration::SetLogLevel(LogLevel level) {
	m_pimpl->SetLogLevel(level);
}
	
std::wstring ServiceConfiguration::GetRulesPath() const {
	return m_pimpl->GetRulesPath();
}

void ServiceConfiguration::SetRulesPath(const std::wstring &path) {
	m_pimpl->SetRulesPath(path);
}

std::wstring ServiceConfiguration::GetCertificatesStoragePath() const {
	return m_pimpl->GetCertificatesStoragePath();
}

void ServiceConfiguration::SetCertificatesStoragePath(const std::wstring &path) {
	m_pimpl->SetCertificatesStoragePath(path);
}

unsigned long ServiceConfiguration::GetMaxLogSize() const {
	return m_pimpl->GetMaxLogSize();
}

void ServiceConfiguration::SetMaxLogSize(unsigned long size) {
	m_pimpl->SetMaxLogSize(size);
}

bool ServiceConfiguration::Save(const wchar_t *confFilePath) {
	return m_pimpl->Save(confFilePath ? confFilePath : GetConfigurationFilePath());
}

bool ServiceConfiguration::IsChanged() const {
	return m_pimpl->IsChanged();
}

std::wstring ServiceConfiguration::GetConfigurationFileDir() {
	if (IsTestMode()) {
		return Helpers::GetModuleFilePath().branch_path().string() + L"/";
	}
	std::vector<wchar_t> buffer(MAX_PATH, 0);
	if (!SHGetSpecialFolderPathW(NULL, &buffer[0], CSIDL_COMMON_APPDATA, TRUE)) {
		Log::GetInstance().AppendError(
			(Format("Could not get special folder path (system error is \"%1%\").") % GetLastError())
				.str().c_str());
		return std::wstring();
	}
	std::wstring result = &buffer[0];
	result += L"\\" TUNNELEX_NAME_W L"\\";
	return result;
}

std::wstring ServiceConfiguration::GetConfigurationFilePath() {
	std::wstring result = GetConfigurationFileDir();
	result += GetConfigurationFile();
	return result;
}

bool ServiceConfiguration::IsServerStarted() const {
	return m_pimpl->IsServerStarted();
}

void ServiceConfiguration::SetServerStarted(bool val) {
	m_pimpl->SetServerStarted(val);
}

const wchar_t* ServiceConfiguration::GetConfigurationFile() {
	return L"ServiceConfiguration.xml";
}

boost::shared_ptr<Document> ServiceConfiguration::GetDefaultConfigurationDoc() {
	const fs::wpath configuradionDir(GetConfigurationFileDir());
	boost::shared_ptr<Document> doc = Document::CreateNew("Configuration");
	boost::shared_ptr<Node> root = doc->GetRoot();
	root->SetAttribute("Version", "1.2");
	fs::wpath rulesFile(configuradionDir);
	rulesFile /= L"RuleSet.xml";
	root->CreateNewChild("Rules")->SetContent(rulesFile.string());
	boost::shared_ptr<Node> log = root->CreateNewChild("Log");
	fs::wpath logFile(configuradionDir);
	logFile /= L"Service.log";
	log->SetContent(logFile.string());
	log->SetAttribute("Level", "information");
	log->SetAttribute("MaxSize", boost::lexical_cast<std::wstring>((1024 *  1024) * 1));
	root->CreateNewChild("ServerState")->SetContent("stopped");
	fs::wpath sslCertificatesDir(configuradionDir);
	sslCertificatesDir /= L"CertificatesStorage";
	root->CreateNewChild("CertificatesStorage")->SetContent(sslCertificatesDir.string());
	return doc;
}

std::auto_ptr<ServiceConfiguration> ServiceConfiguration::GetDefault() {
	return std::auto_ptr<ServiceConfiguration>(
		new ServiceConfiguration(GetDefaultConfigurationDoc()));
}


bool ServiceConfiguration::m_testModeToggle = false;
void ServiceConfiguration::SetTestModeToggle(bool val) {
	m_testModeToggle = val;
}
bool ServiceConfiguration::IsTestMode() {
	return m_testModeToggle;
}
