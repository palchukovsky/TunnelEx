/**************************************************************************
 *   Created: 2008/06/08 13:55
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: ServiceConfiguration.cpp 1107 2010-12-20 12:24:07Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "ServiceConfiguration.h"
#include "ServiceControl/Configuration.hpp"

#include "Core/Log.hpp"
#include "Core/Exceptions.hpp"

namespace fs = boost::filesystem;
using namespace TunnelEx;
using namespace TunnelEx::Helpers::Xml;

//! Migrates service configuration XML in format 1.0 to current. Throws an exception on error.
/** @param	source	the DOM-document, rules in format 1.0.
*	@return		DOM-document, rules in current format.
*/
boost::shared_ptr<Document> MigrateServiceConfigurationFrom_1_0(
			boost::shared_ptr<const Document> source) {

	boost::shared_ptr<const XPath> oldDocXpath(source->GetXPath());
	ConstNodeCollection oldDocQueryResult;

	boost::shared_ptr<Document> newDoc(ServiceConfiguration::GetDefaultConfigurationDoc());
	boost::shared_ptr<XPath> newDocXPath(newDoc->GetXPath());
	NodeCollection newDocQueryResult;
	UString buffer;

	oldDocXpath->Query("/Configuration/Rules", oldDocQueryResult);
	if (oldDocQueryResult.size() > 0) {
		Log::GetInstance().AppendInfo(
			"Legacy version supports (service configuration migration):"
				" Importing \"Rules\"...");
		newDocXPath->Query("/Configuration/Rules", newDocQueryResult);
		BOOST_ASSERT(newDocQueryResult.size() == 1);
		newDocQueryResult[0]->SetContent(oldDocQueryResult[0]->GetContent(buffer));
	}

	oldDocXpath->Query("/Configuration/Log", oldDocQueryResult);
	if (oldDocQueryResult.size() > 0) {
		Log::GetInstance().AppendInfo(
				"Legacy version supports (service configuration migration):"
					" Importing \"Log\"...");
		newDocXPath->Query("/Configuration/Log", newDocQueryResult);
		BOOST_ASSERT(newDocQueryResult.size() == 1);
		newDocQueryResult[0]->SetContent(oldDocQueryResult[0]->GetContent(buffer));
		if (oldDocQueryResult[0]->HasAttribute("Level")) {
			Log::GetInstance().AppendInfo(
				"Legacy version supports (service configuration migration):"
					" Importing \"Log/Level\"...");
			newDocQueryResult[0]->SetAttribute(
				"Level",
				oldDocQueryResult[0]->GetAttribute("Level", buffer));
		}
		if (oldDocQueryResult[0]->HasAttribute("MaxSize")) {
			Log::GetInstance().AppendInfo(
				"Legacy version supports (service configuration migration):"
					" Importing \"Log/MaxSize\"...");
			newDocQueryResult[0]->SetAttribute(
				"MaxSize",
				oldDocQueryResult[0]->GetAttribute("MaxSize", buffer));
		}
	}

	return newDoc;

}

boost::shared_ptr<Document> MigrateServiceConfigurationFrom_1_1(
			boost::shared_ptr<const Document> source) {

	boost::shared_ptr<Document> result = MigrateServiceConfigurationFrom_1_0(source);
	boost::shared_ptr<XPath> newDocXPath(result->GetXPath());
	NodeCollection newDocQueryResult;

	UString buffer;

	boost::shared_ptr<const XPath> oldDocXpath(source->GetXPath());
	ConstNodeCollection oldDocQueryResult;
	oldDocXpath->Query("/Configuration/ServerState", oldDocQueryResult);
	if (oldDocQueryResult.size() > 0) {
		Log::GetInstance().AppendInfo(
			"Legacy version supports (service configuration migration):"
			" Importing \"ServerState\"...");
		newDocXPath->Query("/Configuration/ServerState", newDocQueryResult);
		BOOST_ASSERT(newDocQueryResult.size() == 1);
		newDocQueryResult[0]->SetContent(oldDocQueryResult[0]->GetContent(buffer));
	}

	return result;

}

boost::shared_ptr<ServiceConfiguration> MigrateCurrentServiceConfiguration() {

	// shared_ptr (not auto_ptr): result exports from DLL and deleter must be safe.

	Log::GetInstance().AppendInfo("Starting service migration...");

	try {
		Log::GetInstance().AppendInfo("Opening current configuration file...");
		boost::shared_ptr<ServiceConfiguration> conf(new ServiceConfiguration());
		Log::GetInstance().AppendInfo(
			"Migration is not required, migration stopped.");
		return conf;
	} catch (const ServiceConfiguration::ConfigurationNotFoundException &) {
		Log::GetInstance().AppendInfo(
			"File not found, creating default configuration...");
		boost::shared_ptr<ServiceConfiguration> conf(
			ServiceConfiguration::GetDefault());
		Log::GetInstance().AppendInfo("Migration stopped.");
		return conf;
	} catch (const ServiceConfiguration::ConfigurationException &) {
		//...//
	}

	if (!fs::exists(ServiceConfiguration::GetConfigurationFilePath())) {
		Log::GetInstance().AppendInfo(
			"File not found, creating default configuration...");
		boost::shared_ptr<ServiceConfiguration> conf(
			ServiceConfiguration::GetDefault());
		Log::GetInstance().AppendInfo("Migration stopped.");
		return conf;
	}

	boost::shared_ptr<const Document> oldDoc;
	try {
		oldDoc
			= Document::LoadFromFile(ServiceConfiguration::GetConfigurationFilePath());
	} catch (const Document::ParseException &) {
		Log::GetInstance().AppendWarn(
			"File has invalid format, stopping migration and creating default configuration...");
		boost::shared_ptr<ServiceConfiguration> conf(
			ServiceConfiguration::GetDefault());
		Log::GetInstance().AppendInfo("Migration stopped.");
		return conf;
	}

	boost::shared_ptr<ServiceConfiguration> result;
	try {
		boost::shared_ptr<const XPath> xpath(oldDoc->GetXPath());
		ConstNodeCollection  queryResult;
		xpath->Query("/Configuration", queryResult);
		boost::shared_ptr<Document> newDoc;
		WString buffer;
		if (queryResult.size() == 1 && queryResult[0]->HasAttribute("Version")) {
			if (queryResult[0]->GetAttribute("Version", buffer) == L"1.0") {
				newDoc = MigrateServiceConfigurationFrom_1_0(oldDoc);
			} else if (queryResult[0]->GetAttribute("Version", buffer) == L"1.1") {
				newDoc = MigrateServiceConfigurationFrom_1_1(oldDoc);
			}
		}
		if (newDoc) {
			result.reset(new ServiceConfiguration(newDoc));
		} else {
			Log::GetInstance().AppendWarn("Unknown version, migration failed...");
			result = ServiceConfiguration::GetDefault();
		}
	} catch (const std::exception &) {
		Log::GetInstance().AppendWarn(
			"Migration failed, creating default configuration...");
		result = ServiceConfiguration::GetDefault();
	} catch (const TunnelEx::LocalException &) {
		Log::GetInstance().AppendWarn(
			"Migration failed, creating default configuration...");
		result = ServiceConfiguration::GetDefault();
	}

	Log::GetInstance().AppendInfo("Service migration finished.");
	return result;

}
