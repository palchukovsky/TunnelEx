/**************************************************************************
 *   Created: 2010/05/15 16:21
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Rule.hpp 942 2010-05-29 20:46:50Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__RuleEntity_hpp__1005151621
#define INCLUDED_FILE__TUNNELEX__RuleEntity_hpp__1005151621

#include "Endpoint.hpp"
#include "Exceptions.hpp"
#include "Collection.hpp"
#include "String.hpp"
#include "Api.h"

namespace TunnelEx {

	//////////////////////////////////////////////////////////////////////////

	class TUNNELEX_CORE_API Rule {

	public:

		enum ErrorsTreatment {
			ERRORS_TREATMENT_INFO,
			ERRORS_TREATMENT_WARN,
			ERRORS_TREATMENT_ERROR
		};

	public:
		
		Rule();
		explicit Rule(const ::TunnelEx::WString &uuid);
		Rule(const Rule &);
		virtual ~Rule() throw();

	public:

		const Rule & operator =(const Rule &);
		void Swap(Rule &) throw();

	public:

		const ::TunnelEx::WString & GetUuid() const;

		const ::TunnelEx::WString & GetName() const;
		void SetName(const ::TunnelEx::WString &);

		ErrorsTreatment GetErrorsTreatment() const;
		void SetErrorsTreatment(ErrorsTreatment);

		bool IsEnabled() const;
		void Enable(bool);

	protected:

		void SetUuid(const ::TunnelEx::WString &);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

		class ServiceRule;

	class TUNNELEX_CORE_API ServiceRuleSet
			: public ::TunnelEx::Collection<::TunnelEx::ServiceRule> {

	public:

		typedef ::TunnelEx::Collection<::TunnelEx::ServiceRule> Base;

	public:

		ServiceRuleSet();
		explicit ServiceRuleSet(size_t reserve);
		ServiceRuleSet(const ServiceRuleSet &);
		
		~ServiceRuleSet() throw();

		const ServiceRuleSet & operator =(const ServiceRuleSet &);

		void Swap(ServiceRuleSet &) throw();

	};

	//////////////////////////////////////////////////////////////////////////

	class TUNNELEX_CORE_API ServiceRule : public ::TunnelEx::Rule {

	public:

		struct Service {
			::TunnelEx::WString uuid;
			::TunnelEx::WString name;
			::TunnelEx::WString param;
		};

		typedef ::TunnelEx::Collection<Service> ServiceSet;

	public:
		
		ServiceRule();
		explicit ServiceRule(const ::TunnelEx::WString &uuid);
		ServiceRule(const ServiceRule &);
		
		~ServiceRule() throw();

	public:

		const ServiceRule & operator =(const ServiceRule &);
		void Swap(ServiceRule &) throw();

		ServiceRule MakeCopy() const;

	public:

		const ServiceSet & GetServices() const;
		ServiceSet & GetServices();

		void SetServices(const ServiceSet &);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

	//////////////////////////////////////////////////////////////////////////

	class TunnelRule;

	class TUNNELEX_CORE_API TunnelRuleSet
			: public ::TunnelEx::Collection<::TunnelEx::TunnelRule> {

	public:

		typedef ::TunnelEx::Collection<::TunnelEx::TunnelRule> Base;

	public:

		TunnelRuleSet();
		explicit TunnelRuleSet(size_t reserve);
		TunnelRuleSet(const TunnelRuleSet &);
		
		~TunnelRuleSet() throw();

		const TunnelRuleSet & operator =(const TunnelRuleSet &);

		void Swap(TunnelRuleSet &) throw();

	};

	//////////////////////////////////////////////////////////////////////////

	class TUNNELEX_CORE_API TunnelRule : public ::TunnelEx::Rule {

	public:

		typedef ::TunnelEx::Collection<::TunnelEx::WString> Filters;

	public:
		
		//! Constructs empty rule.
		TunnelRule();
		//! Constructs empty rule with UUID.
		explicit TunnelRule(const ::TunnelEx::WString &uuid);
		TunnelRule(const TunnelRule &);
		
		~TunnelRule() throw();

	public:

		const TunnelRule & operator =(const TunnelRule &);
		void Swap(TunnelRule &) throw();

		//! Creates copy of rule and generates new UUIDs for it and for all members.
		TunnelRule MakeCopy() const;

	public:

		//! Returns a collection with tunnel input endpoints.
		/** For all endpoints tunnel entry will be opened. */
		const ::TunnelEx::RuleEndpointCollection & GetInputs() const;
		::TunnelEx::RuleEndpointCollection & GetInputs();
		//! Sets a collection with tunnel input endpoints.
		/** For all endpoints tunnel entry will be opened. */
		void SetInputs(const ::TunnelEx::RuleEndpointCollection &);

		//! Returns a collection with tunnel destination endpoints.
		const ::TunnelEx::RuleEndpointCollection & GetDestinations() const;
		::TunnelEx::RuleEndpointCollection & GetDestinations();
		//! Sets a collection with tunnel destination endpoints.
		void SetDestinations(const ::TunnelEx::RuleEndpointCollection &);
		
		const Filters & GetFilters() const;
		Filters & GetFilters();
		void SetFilters(const Filters &);

		//! Returns limit for accepted connections.
		/** @return zero - unlimited, or limit as positive value;
		  */
		unsigned long GetAcceptedConnectionsLimit() const;
		//! Sets limit for accepted connections.
		/** @param	limit	zero - unlimited, or limit as positive value;
		  */
		void SetAcceptedConnectionsLimit(unsigned long limit);
		
	private:

		class Implementation;
		Implementation *m_pimpl;

	};

	//////////////////////////////////////////////////////////////////////////

	class TUNNELEX_CORE_API RuleSet {

	public:

		RuleSet();

		explicit RuleSet(
				const ::TunnelEx::WString &)
			throw(
				::TunnelEx::InvalidXmlException,
				::TunnelEx::XmlDoesNotMatchException);

		explicit RuleSet(
				const ::TunnelEx::ServiceRuleSet &,
				const ::TunnelEx::TunnelRuleSet &);

		RuleSet(const RuleSet &);

		~RuleSet() throw();

	public:

		const RuleSet & operator =(const RuleSet &);
		void Swap(RuleSet &) throw();

	public:

		void GetXml(::TunnelEx::UString &destinationBuffer) const;

		void GetXml(::TunnelEx::WString &destinationBuffer) const;

		static void GetXml(
				const ::TunnelEx::ServiceRuleSet &,
				const ::TunnelEx::TunnelRuleSet &,
				::TunnelEx::UString &destinationBuffer);

		static void GetXml(
				const ::TunnelEx::ServiceRuleSet &,
				const ::TunnelEx::TunnelRuleSet &,
				::TunnelEx::WString &destinationBuffer);

	public:

		const ::TunnelEx::ServiceRuleSet & GetServices() const;
		::TunnelEx::ServiceRuleSet & GetServices();
		
		const ::TunnelEx::TunnelRuleSet & GetTunnels() const;
		::TunnelEx::TunnelRuleSet & GetTunnels();

	public:

		size_t GetSize() const;

		void Append(const ServiceRule &);
		void Append(const TunnelRule &);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};


}

#endif // INCLUDED_FILE__TUNNELEX__RuleEntity_hpp__1005151621
