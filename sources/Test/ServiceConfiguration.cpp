/**************************************************************************
 *   Created: 2008/01/01 18:06
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.com
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 * -------------------------------------------------------------------
 *       $Id: ServiceConfiguration.cpp 959 2010-06-22 11:36:04Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "ServiceControl/Configuration.hpp"

#include <TunnelEx/String.hpp>

namespace ut = boost::unit_test;
namespace tex = TunnelEx;
namespace fs = boost::filesystem;
namespace xml = TunnelEx::Helpers::Xml;
using namespace std;
using namespace boost;

namespace Test { BOOST_AUTO_TEST_SUITE(ServiceConfiguration)

	BOOST_AUTO_TEST_CASE(DefaultValues) {
		fs::wpath configurationFile = fs::initial_path<fs::wpath>();
		configurationFile /= L"DefaultServiceConfigurationTest.xml";
		if (fs::exists(configurationFile)) {
			BOOST_REQUIRE(fs::remove(configurationFile));
		}
		{
			shared_ptr<::ServiceConfiguration> defaultConf(::ServiceConfiguration::GetDefault());
			BOOST_REQUIRE(defaultConf->Save(configurationFile.string().c_str()));
		}
		::ServiceConfiguration configuration(configurationFile.string().c_str());
		fs::wpath logFile = ::ServiceConfiguration::GetConfigurationFileDir();
		logFile /= L"Service.log";
		BOOST_CHECK(configuration.GetLogPath() == logFile.string());
		BOOST_CHECK(configuration.GetLogLevel() == tex::LOG_LEVEL_INFO);
		fs::wpath rulesFile = ::ServiceConfiguration::GetConfigurationFileDir();
		rulesFile /= L"RuleSet.xml";
		BOOST_CHECK(configuration.GetRulesPath() == rulesFile.string());
		BOOST_CHECK(configuration.GetMaxLogSize() == (1024 * 1024) * 1);
		BOOST_CHECK(configuration.IsServerStarted() == false);
	}

	BOOST_AUTO_TEST_CASE(ServiceConfiguration_Validation) {
		fs::wpath configurationFile = fs::initial_path<fs::wpath>();
		configurationFile /= L"DefaultServiceConfigurationTest.xml";
		if (fs::exists(configurationFile)) {
			BOOST_REQUIRE(fs::remove(configurationFile));
		}
		{
			shared_ptr<::ServiceConfiguration> defaultConf(::ServiceConfiguration::GetDefault());
			BOOST_REQUIRE(defaultConf->Save(configurationFile.string().c_str()));
		}
		::ServiceConfiguration configuration(configurationFile.string().c_str());
		BOOST_CHECK_THROW(configuration.SetLogPath(L"DD:\\xxx\\vvv.log"), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
		BOOST_CHECK_THROW(configuration.SetLogPath(L"Ä:\\xxx\\vvv.log"), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
		BOOST_CHECK_THROW(configuration.SetLogPath(L"D:\\xxx\vvv\\"), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
		BOOST_CHECK_THROW(configuration.SetLogPath(L"D:\\x\"xx\vvv.log"), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
		BOOST_CHECK_THROW(configuration.SetLogPath(L"D:\\x>xx\vvv.log"), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
		BOOST_CHECK_THROW(configuration.SetLogPath(L"xxx\\vvv.log"), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
		BOOST_CHECK_THROW(configuration.SetLogPath(L"\\xxx\\vvv.log"), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
		BOOST_CHECK_THROW(configuration.SetLogLevel(static_cast<tex::LogLevel>(1000)), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
		BOOST_CHECK_THROW(configuration.SetRulesPath(L"DD:\\xxx\\vvv.log"), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
		BOOST_CHECK_THROW(configuration.SetRulesPath(L"Ä:\\xxx\\vvv.log"), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
		BOOST_CHECK_THROW(configuration.SetRulesPath(L"D:\\xxx\vvv\\"), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
		BOOST_CHECK_THROW(configuration.SetRulesPath(L"D:\\x\"xx\vvv.log"), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
		BOOST_CHECK_THROW(configuration.SetRulesPath(L"D:\\x>xx\vvv.log"), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
		BOOST_CHECK_THROW(configuration.SetRulesPath(L"xxx\\vvv.log"), ::ServiceConfiguration::ConfigurationHasInvalidFormatException);
	}

	BOOST_AUTO_TEST_CASE(Set) {
		fs::wpath configurationFile = fs::initial_path<fs::wpath>();
		configurationFile /= L"DefaultServiceConfigurationTest.xml";
		if (fs::exists(configurationFile)) {
			BOOST_REQUIRE(fs::remove(configurationFile));
		}
		{
			shared_ptr<::ServiceConfiguration> defaultConf(::ServiceConfiguration::GetDefault());
			BOOST_REQUIRE(defaultConf->Save(configurationFile.string().c_str()));
		}
		{
			::ServiceConfiguration configuration(configurationFile.string().c_str());
			BOOST_CHECK_NO_THROW(configuration.SetLogPath(L"D:\\xxx yyy hhh\\vvv kkkks.log"));
			BOOST_CHECK_NO_THROW(configuration.SetLogLevel(tex::LOG_LEVEL_TRACK));
			BOOST_CHECK_NO_THROW(configuration.SetRulesPath(L"X:\\xxx yyy hhh\\vv ooov.xml"));
			BOOST_CHECK_NO_THROW(configuration.SetMaxLogSize(123456789));
			BOOST_CHECK_NO_THROW(configuration.SetServerStarted(true));
			BOOST_CHECK(configuration.GetLogPath() == L"D:\\xxx yyy hhh\\vvv kkkks.log");
			BOOST_CHECK(configuration.GetLogLevel() == tex::LOG_LEVEL_TRACK);
			BOOST_CHECK(configuration.GetRulesPath() == L"X:\\xxx yyy hhh\\vv ooov.xml");
			BOOST_CHECK(configuration.GetMaxLogSize() == 123456789);
			BOOST_CHECK(configuration.IsServerStarted() == true);
			configuration.Save(configurationFile.string().c_str());
		}
		shared_ptr<const xml::XPath> xpath(
			xml::Document::LoadFromFile(configurationFile.string().c_str())->GetXPath());
		xml::ConstNodeCollection  queryResult;
		string buffer;
		xpath->Query("//Configuration[@Version = '1.1']/Rules", queryResult);
		BOOST_REQUIRE(1 == queryResult.size());
		BOOST_CHECK(queryResult[0]->GetContent(buffer) == "X:\\xxx yyy hhh\\vv ooov.xml");
		xpath->Query("//Configuration[@Version = '1.1']/Log", queryResult);
		BOOST_REQUIRE(1 == queryResult.size());
		BOOST_CHECK(queryResult[0]->GetContent(buffer) == "D:\\xxx yyy hhh\\vvv kkkks.log");
		BOOST_CHECK(queryResult[0]->GetAttribute("Level", buffer) == "track");
		BOOST_CHECK(queryResult[0]->GetAttribute("MaxSize", buffer) == "123456789");
		xpath->Query("//Configuration[@Version = '1.1']/ServerState", queryResult);
		BOOST_REQUIRE(1 == queryResult.size());
		BOOST_CHECK(queryResult[0]->GetContent(buffer) == "started");
	}

BOOST_AUTO_TEST_SUITE_END() }
