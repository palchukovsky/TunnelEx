/**************************************************************************
 *   Created: 2008/06/12 20:16
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "Rules.h"
#include "ServiceConfiguration.h"
#include "ServiceControl/Configuration.hpp"
#include "ServiceFilesSecurity.hpp"
#include "Core/Rule.hpp"

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
using namespace TunnelEx;
using namespace TunnelEx::Helpers;

void BackupPreviousVersionFile(const fs::wpath &filePath) {
	//! \todo: reimplement with facets [2008/06/12 20:20]
	tm now(pt::to_tm(pt::second_clock::local_time()));
	WFormat backupExt(L".%04d%02d%02d%02d%02d%02d.bak.xml");
	backupExt % (now.tm_year + 1900) % (now.tm_mon + 1) % now.tm_mday;
	backupExt % now.tm_hour % now.tm_min % now.tm_sec;
	const fs::wpath configurationBackup(filePath.string() + backupExt.str());
	if (fs::exists(filePath)) {
		try {
			copy_file(filePath, configurationBackup);
		} catch (const fs::filesystem_error &) {
			// silent for user (could not block work if backup is fail),
			// but assert for developer
			BOOST_ASSERT(false);
		}
	}
}

void MigrateAllAndSave() {

	boost::shared_ptr<ServiceConfiguration> configuration(MigrateCurrentServiceConfiguration());
	if (configuration->IsChanged()) {
		BackupPreviousVersionFile(ServiceConfiguration::GetConfigurationFilePath());
		configuration->Save();
		if (!ServiceConfiguration::IsTestMode()) {
			ServiceFilesSecurity::Set(ServiceConfiguration::GetConfigurationFileDir());
		}
	}

	RuleSet ruleSet;
	if (MigrateCurrentRuleSet(ruleSet)) {
		const fs::wpath ruleSetFilePath(ServiceConfiguration().GetRulesPath());
		BackupPreviousVersionFile(ruleSetFilePath);
		try {
			fs::create_directories(ruleSetFilePath.branch_path());
			std::wofstream rulesFile(
				ruleSetFilePath.string().c_str(),
				std::ios::binary | std::ios::out | std::ios::trunc);
			if (rulesFile) {
				WString rulesXml;
				ruleSet.GetXml(rulesXml);
				rulesFile << rulesXml.GetCStr();
			} else {
				Format message("Could not save new rule std::set: could not open file \"%1%\".");
				message % ConvertString<String>(ruleSetFilePath.string().c_str()).GetCStr();
				Log::GetInstance().AppendFatalError(message.str().c_str());
			}
		} catch (const boost::filesystem::filesystem_error &ex) {
			Format message("Could not save new rule std::set: could not create directory \"%1%\".");
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
