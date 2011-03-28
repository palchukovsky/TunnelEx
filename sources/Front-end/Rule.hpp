/**************************************************************************
 *   Created: 2008/02/18 21:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Rule.hpp 959 2010-06-22 11:36:04Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__RuleMap_h__0802182132
#define INCLUDED_FILE__TUNNELEX__RuleMap_h__0802182132

#include <TunnelEx/Rule.hpp>

typedef boost::ptr_map<std::wstring, TunnelEx::Rule> RulesMap;
typedef std::set<std::wstring> RulesUuids;

enum NotAppliedRuleState {
	NARS_MODIFIED,
	NARS_ADDED,
	NARS_DELETED
};
typedef std::map<std::wstring, NotAppliedRuleState> NotAppliedRulesUuids;

template<class CheckRule>
inline bool IsRule(const TunnelEx::Rule *const rule) {
	return dynamic_cast<const CheckRule *>(rule) != 0;
}

inline bool IsTunnel(const TunnelEx::Rule *const rule) {
	return IsRule<const TunnelEx::TunnelRule>(rule);
}

inline bool IsService(const TunnelEx::Rule *const rule) {
	return IsRule<const TunnelEx::ServiceRule>(rule);
}

inline bool IsTunnel(const TunnelEx::Rule &rule) {
	return IsTunnel(&rule);
}

inline bool IsService(const TunnelEx::Rule &rule) {
	return IsService(&rule);
}

inline void Append(const TunnelEx::Rule *const rule, TunnelEx::RuleSet &ruleSet) {
	using namespace boost;
	using namespace TunnelEx;
	if (IsTunnel(rule)) {
		ruleSet.Append(*polymorphic_downcast<const TunnelRule *>(rule));
	} else {
		ruleSet.Append(*polymorphic_downcast<const ServiceRule *>(rule));
	}
}

inline std::auto_ptr<TunnelEx::Rule> Clone(const TunnelEx::Rule *const source) {
	using namespace std;
	using namespace boost;
	using namespace TunnelEx;
	if (IsTunnel(source)) {
		return auto_ptr<Rule>(
			new TunnelRule(*polymorphic_downcast<const TunnelRule *>(source)));
	} else {
		return auto_ptr<Rule>(
			new ServiceRule(*polymorphic_downcast<const ServiceRule *>(source)));
	}
}

inline std::auto_ptr<TunnelEx::Rule> Clone(const TunnelEx::Rule &source) {
	return Clone(&source);
}

namespace boost {
	template<>
	inline TunnelEx::Rule * new_clone(const TunnelEx::Rule &source) {
		return Clone(source).release();
	}
}

#endif // INCLUDED_FILE__TUNNELEX__RuleMap_h__0802182132
