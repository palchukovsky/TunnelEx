/**************************************************************************
 *   Created: 2008/01/12 2:36
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.com
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Filter_h__0801120236
#define INCLUDED_FILE__TUNNELEX__Filter_h__0801120236

#include "Instance.hpp"
#include "SmartPtr.hpp"
#include "Locking.hpp"
#include "Api.h"

namespace TunnelEx {

	class TunnelRule;
	class RecursiveMutex;

	//! Interface for rule filter.
	class TUNNELEX_CORE_API Filter : public ::TunnelEx::Instance {

	public:
		
		Filter(
			::TunnelEx::SharedPtr<::TunnelEx::TunnelRule> rule,
			::TunnelEx::SharedPtr<::TunnelEx::RecursiveMutex> ruleChangingMutex);
		virtual ~Filter();

	public:

		//! Schedules rule changing. 
		/** When it will be able, base class will call ChangeRule and pass
		  * rule as parameter.
		  * @sa		ChangeRule()
		  */
		void ScheduleRuleChange();

		const ::TunnelEx::TunnelRule & GetRule() const;

	protected:

		//! Locks rule for changes and returns lock.
		::TunnelEx::AutoPtr<::TunnelEx::RecursiveLock> LockRule();

		//! Rules changing.
		/** Must be implemented by any derived class. Rule must be changed 
		  * only in this method. Changes outside isn't safely. Control also
		  * must be return as soon as possible, as other filters for this
		  * rule will be stopped.
		  *	@sa		ScheduleRuleChange
		  * @param	rule rule to change
		  */
		virtual void ChangeRule(::TunnelEx::TunnelRule &rule) = 0;

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}

#endif // INCLUDED_FILE__TUNNELEX__Filter_h__0801120236
