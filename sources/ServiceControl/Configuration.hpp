/**************************************************************************
 *   Created: 2007/12/27 22:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Configuration.hpp 1107 2010-12-20 12:24:07Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Configuration_h__0712272245
#define INCLUDED_FILE__TUNNELEX__Configuration_h__0712272245

#include "Dll.hpp"
#include "Core/Log.hpp"

namespace TunnelEx {
	namespace Helpers {
		namespace Xml {
			class Document;
		}
	}
}

class ServiceConfiguration {

public:

	struct ConfigurationException : std::exception {
		ConfigurationException()
				: exception("Service configuration exception.") {
			//...//
		}
	};
	struct ConfigurationNotFoundException : public ConfigurationException {
		//...//
	};
	struct ConfigurationHasInvalidFormatException : public ConfigurationException {
		//...//
	};

public:

	/** @throw ConfigurationNotFoundException
	  * @throw ConfigurationHasInvalidFormatException
	  */
	explicit ServiceConfiguration(const wchar_t *confFilePath = NULL);

	/** @throw ConfigurationNotFoundException
	  * @throw ConfigurationHasInvalidFormatException
	  */
	explicit ServiceConfiguration(
			boost::shared_ptr<const TunnelEx::Helpers::Xml::Document>);

	explicit ServiceConfiguration(
			TunnelEx::Helpers::DllObjectPtr<ServiceConfiguration>);
	
	ServiceConfiguration(const ServiceConfiguration &);
	
	~ServiceConfiguration() throw();
	
	const ServiceConfiguration & operator =(const ServiceConfiguration &);
	
	void Swap(ServiceConfiguration &) throw();

public:

	std::wstring GetLogPath() const;
	/** @throw ConfigurationNotFoundException
	  * @throw ConfigurationHasInvalidFormatException
	  */
	void SetLogPath(const std::wstring &);

	TunnelEx::LogLevel GetLogLevel() const;
	/** @throw ConfigurationNotFoundException
	  * @throw ConfigurationHasInvalidFormatException
	  */
	void SetLogLevel(TunnelEx::LogLevel);
	
	std::wstring GetRulesPath() const;
	/** @throw ConfigurationNotFoundException
	  * @throw ConfigurationHasInvalidFormatException
	  */
	void SetRulesPath(const std::wstring &);

	std::wstring GetCertificatesStoragePath() const;
	/** @throw ConfigurationNotFoundException
	  * @throw ConfigurationHasInvalidFormatException
	  */
	void SetCertificatesStoragePath(const std::wstring &);

	unsigned long GetMaxLogSize() const;
	/** @throw ConfigurationNotFoundException
	  * @throw ConfigurationHasInvalidFormatException
	  */
	void SetMaxLogSize(unsigned long);

	bool IsServerStarted() const;
	/** @throw ConfigurationNotFoundException
	  * @throw ConfigurationHasInvalidFormatException
	  */
	void SetServerStarted(bool);

	bool Save(const wchar_t *confFilePath = 0);
	bool IsChanged() const;

	static std::wstring GetConfigurationFileDir();
	static std::wstring GetConfigurationFilePath();
	static const wchar_t * GetConfigurationFile();

	/** @throw ConfigurationHasInvalidFormatException
	  */
	static std::auto_ptr<ServiceConfiguration> GetDefault();
	static boost::shared_ptr<TunnelEx::Helpers::Xml::Document> GetDefaultConfigurationDoc();

	static void SetTestModeToggle(bool);
	static bool IsTestMode();

private:

	class Implementation;
	std::auto_ptr<Implementation> m_pimpl;

	static bool m_testModeToggle;

};

#endif // INCLUDED_FILE__TUNNELEX__Configuration_h__0712272245
