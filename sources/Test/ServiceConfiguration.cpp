/**************************************************************************
 *   Created: 2008/01/01 18:06
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.com
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.h"

#include "ServiceControl/Configuration.hpp"

#include "Core/String.hpp"

namespace tex = TunnelEx;
namespace fs = boost::filesystem;
namespace xml = TunnelEx::Helpers::Xml;

namespace Test {

	TEST(ServiceConfiguration, DefaultValues) {
		fs::wpath configurationFile = fs::initial_path<fs::wpath>();
		configurationFile /= L"DefaultServiceConfigurationTest.xml";
		if (fs::exists(configurationFile)) {
			ASSERT_TRUE(fs::remove(configurationFile));
		}
		{
		boost::shared_ptr<::ServiceConfiguration> defaultConf(::ServiceConfiguration::GetDefault());
			ASSERT_TRUE(defaultConf->Save(configurationFile.string().c_str()));
		}
		::ServiceConfiguration configuration(configurationFile.string().c_str());
		fs::wpath logFile = ::ServiceConfiguration::GetConfigurationFileDir();
		logFile /= L"Service.log";
		EXPECT_TRUE(configuration.GetLogPath() == logFile.string());
		EXPECT_TRUE(configuration.GetLogLevel() == tex::LOG_LEVEL_INFO);
		fs::wpath rulesFile = ::ServiceConfiguration::GetConfigurationFileDir();
		rulesFile /= L"RuleSet.xml";
		EXPECT_TRUE(configuration.GetRulesPath() == rulesFile.string());
		EXPECT_TRUE(configuration.GetMaxLogSize() == (1024 * 1024) * 1);
		EXPECT_TRUE(configuration.IsServerStarted() == false);
	}

	TEST(ServiceConfiguration, Validation) {
		fs::wpath configurationFile = fs::initial_path<fs::wpath>();
		configurationFile /= L"DefaultServiceConfigurationTest.xml";
		if (fs::exists(configurationFile)) {
			ASSERT_TRUE(fs::remove(configurationFile));
		}
		{
		boost::shared_ptr<::ServiceConfiguration> defaultConf(::ServiceConfiguration::GetDefault());
			ASSERT_TRUE(defaultConf->Save(configurationFile.string().c_str()));
		}
		::ServiceConfiguration configuration(configurationFile.string().c_str());
		EXPECT_THROW(configuration.SetLogPath(L"DD:\\xxx\\vvv.log"), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
		EXPECT_THROW(configuration.SetLogPath(L"Ä:\\xxx\\vvv.log"), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
		EXPECT_THROW(configuration.SetLogPath(L"D:\\xxx\vvv\\"), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
		EXPECT_THROW(configuration.SetLogPath(L"D:\\x\"xx\vvv.log"), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
		EXPECT_THROW(configuration.SetLogPath(L"D:\\x>xx\vvv.log"), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
		EXPECT_THROW(configuration.SetLogPath(L"xxx\\vvv.log"), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
		EXPECT_THROW(configuration.SetLogPath(L"\\xxx\\vvv.log"), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
		EXPECT_THROW(configuration.SetLogLevel(static_cast<tex::LogLevel>(1000)), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
		EXPECT_THROW(configuration.SetRulesPath(L"DD:\\xxx\\vvv.log"), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
		EXPECT_THROW(configuration.SetRulesPath(L"Ä:\\xxx\\vvv.log"), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
		EXPECT_THROW(configuration.SetRulesPath(L"D:\\xxx\vvv\\"), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
		EXPECT_THROW(configuration.SetRulesPath(L"D:\\x\"xx\vvv.log"), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
		EXPECT_THROW(configuration.SetRulesPath(L"D:\\x>xx\vvv.log"), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
		EXPECT_THROW(configuration.SetRulesPath(L"xxx\\vvv.log"), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
	}

	TEST(ServiceConfiguration, Set) {
		fs::wpath configurationFile = fs::initial_path<fs::wpath>();
		configurationFile /= L"DefaultServiceConfigurationTest.xml";
		if (fs::exists(configurationFile)) {
			ASSERT_TRUE(fs::remove(configurationFile));
		}
		{
		boost::shared_ptr<::ServiceConfiguration> defaultConf(::ServiceConfiguration::GetDefault());
			ASSERT_TRUE(defaultConf->Save(configurationFile.string().c_str()));
		}
		{
			::ServiceConfiguration configuration(configurationFile.string().c_str());
			EXPECT_NO_THROW(configuration.SetLogPath(L"D:\\xxx yyy hhh\\vvv kkkks.log"));
			EXPECT_NO_THROW(configuration.SetLogLevel(tex::LOG_LEVEL_TRACK));
			EXPECT_NO_THROW(configuration.SetRulesPath(L"X:\\xxx yyy hhh\\vv ooov.xml"));
			EXPECT_NO_THROW(configuration.SetMaxLogSize(123456789));
			EXPECT_NO_THROW(configuration.SetServerStarted(true));
			EXPECT_TRUE(configuration.GetLogPath() == L"D:\\xxx yyy hhh\\vvv kkkks.log");
			EXPECT_TRUE(configuration.GetLogLevel() == tex::LOG_LEVEL_TRACK);
			EXPECT_TRUE(configuration.GetRulesPath() == L"X:\\xxx yyy hhh\\vv ooov.xml");
			EXPECT_TRUE(configuration.GetMaxLogSize() == 123456789);
			EXPECT_TRUE(configuration.IsServerStarted() == true);
			configuration.Save(configurationFile.string().c_str());
		}
	boost::shared_ptr<const xml::XPath> xpath(
			xml::Document::LoadFromFile(configurationFile.string().c_str())->GetXPath());
		xml::ConstNodeCollection  queryResult;
		std::string buffer;
		xpath->Query("//Configuration[@Version = '1.2']/Rules", queryResult);
		ASSERT_TRUE(1 == queryResult.size());
		EXPECT_TRUE(queryResult[0]->GetContent(buffer) == "X:\\xxx yyy hhh\\vv ooov.xml");
		xpath->Query("//Configuration[@Version = '1.2']/Log", queryResult);
		ASSERT_TRUE(1 == queryResult.size());
		EXPECT_TRUE(queryResult[0]->GetContent(buffer) == "D:\\xxx yyy hhh\\vvv kkkks.log");
		EXPECT_TRUE(queryResult[0]->GetAttribute("Level", buffer) == "track");
		EXPECT_TRUE(queryResult[0]->GetAttribute("MaxSize", buffer) == "123456789");
		xpath->Query("//Configuration[@Version = '1.2']/ServerState", queryResult);
		ASSERT_TRUE(1 == queryResult.size());
		EXPECT_TRUE(queryResult[0]->GetContent(buffer) == "started");
	}

}
