/**************************************************************************
 *   Created: 2008/06/08 21:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: LegacySupporter.hpp 1046 2010-11-02 12:07:07Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__LegacySupporter_hpp__0806082134
#define INCLUDED_FILE__TUNNELEX__LegacySupporter_hpp__0806082134

#include "Dll.hpp"

namespace TunnelEx {

	class RuleSet;

}

class ServiceConfiguration;

class LegacySupporter : private boost::noncopyable {

public:
	
	LegacySupporter()
			: m_myDll(new TunnelEx::Helpers::Dll(GetMyDllFile())) {
		//...//
	}

	void MigrateAllAndSave() const {
		m_myDll->GetFunction<void(void)>("MigrateAllAndSave")();
	}

	TunnelEx::Helpers::DllObjectPtr<ServiceConfiguration>
	MigrateCurrentServiceConfiguration()
			const {
		using namespace boost;
		using namespace TunnelEx::Helpers;
		return DllObjectPtr<ServiceConfiguration>(
			m_myDll,
			m_myDll->GetFunction<shared_ptr<ServiceConfiguration>(void)>("MigrateCurrentServiceConfiguration")());
	}

	void MigrateCurrentRuleSet(TunnelEx::RuleSet &ruleSet) const {
		typedef void(Proto)(TunnelEx::RuleSet &);
		m_myDll->GetFunction<Proto>("MigrateCurrentRuleSet")(ruleSet);
	}

	void MigrateRuleSet(
				const TunnelEx::WString &xml,
				TunnelEx::RuleSet &ruleSet)
			const {
		using namespace TunnelEx;
		typedef void(Proto)(const WString &, RuleSet &);
		m_myDll->GetFunction<Proto>("MigrateRuleSet")(xml, ruleSet);
	}

	void SetTestModeToggle(bool val) {
		m_myDll->GetFunction<void(bool)>("SetTestModeToggle")(val);
	}

protected:

	static const char * GetMyDllFile() {
		return TUNNELEX_LEGACY_DLL_FILE_NAME;
	}

private:
	
	boost::shared_ptr<TunnelEx::Helpers::Dll> m_myDll;

};

#endif // INCLUDED_FILE__TUNNELEX__LegacySupporter_hpp__0806082134
