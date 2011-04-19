/**************************************************************************
 *   Created: 2008/06/13 1:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Rules_h__0806130122
#define INCLUDED_FILE__TUNNELEX__Rules_h__0806130122

#include "Core/String.hpp"

namespace TunnelEx {
	class RuleSet;
}

bool MigrateCurrentRuleSet(TunnelEx::RuleSet &);
void MigrateRuleSet(const TunnelEx::WString &xml, TunnelEx::RuleSet &);

#endif // INCLUDED_FILE__TUNNELEX__Rules_h__0806130122
