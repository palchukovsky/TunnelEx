/**************************************************************************
 *   Created: 2008/06/12 20:16
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: LegacySupporter.cpp 1107 2010-12-20 12:24:07Z palchukovsky $
 **************************************************************************/

#include "Prec.h"
#include "Rules.h"
#include "ServiceConfiguration.h"
#include "ServiceControl/Configuration.hpp"
#include "ServiceFilesSecurity.hpp"
#include <TunnelEx/Rule.hpp>

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::posix_time;
using namespace TunnelEx;
using namespace TunnelEx::Helpers;

void BackupPreviousVersionFile(const wpath &filePath) {
	//! \todo: reimplement with facets [2008/06/12 20:20]
	tm now(to_tm(second_clock::local_time()));
	WFormat backupExt(L".%04d%02d%02d%02d%02d%02d.bak.xml");
	backupExt % (now.tm_year + 1900) % (now.tm_mon + 1) % now.tm_mday;
	backupExt % now.tm_hour % now.tm_min % now.tm_sec;
	const wpath configurationBackup(filePath.string() + backupExt.str());
	if (exists(filePath)) {
		try {
			copy_file(filePath, configurationBackup);
		} catch (const boost::filesystem::filesystem_error &) {
			// silent for user (could not block work if backup is fail),
			// but assert for developer
			BOOST_ASSERT(false);
		}
	}
}

void MigrateAllAndSave() {

	shared_ptr<ServiceConfiguration> configuration(MigrateCurrentServiceConfiguration());
	if (configuration->IsChanged()) {
		BackupPreviousVersionFile(ServiceConfiguration::GetConfigurationFilePath());
		configuration->Save();
		if (!ServiceConfiguration::IsTestMode()) {
			ServiceFilesSecurity::Set(ServiceConfiguration::GetConfigurationFileDir());
		}
	}

	RuleSet ruleSet;
	if (MigrateCurrentRuleSet(ruleSet)) {
		const wpath ruleSetFilePath(ServiceConfiguration().GetRulesPath());
		BackupPreviousVersionFile(ruleSetFilePath);
		try {
			create_directories(ruleSetFilePath.branch_path());
			wofstream rulesFile(
				ruleSetFilePath.string().c_str(),
				ios::binary | ios::out | ios::trunc);
			if (rulesFile) {
				WString rulesXml;
				ruleSet.GetXml(rulesXml);
				rulesFile << rulesXml.GetCStr();
			} else {
				Format message("Could not save new rule set: could not open file \"%1%\".");
				message % ConvertString<String>(ruleSetFilePath.string().c_str()).GetCStr();
				Log::GetInstance().AppendFatalError(message.str().c_str());
			}
		} catch (const boost::filesystem::filesystem_error &ex) {
			Format message("Could not save new rule set: could not create directory \"%1%\".");
			message % ex.what();
			Log::GetInstance().AppendSystemError(message.str().c_str());
		}
		if (ServiceConfiguration::GetConfigurationFileDir() == ruleSetFilePath.branch_path()) {
			ServiceFilesSecurity::Set(ruleSetFilePath.branch_path());
		}
	}

}

void SetTestModeToggle(bool val) {
	ServiceConfiguration::SetTestModeToggle(val);
}
