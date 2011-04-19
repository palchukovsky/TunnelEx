/**************************************************************************
 *   Created: 2009/09/09 18:30
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__QueryPolicies_hpp__0909091830
#define INCLUDED_FILE__TUNNELEX__QueryPolicies_hpp__0909091830

#include "Traits.hpp"
#include "Types.hpp"
#include "CastingPolicies.hpp"

namespace TunnelEx { namespace Licensing {
	
	//////////////////////////////////////////////////////////////////////////

	template<typename ClientTrait>
	struct QueryPathPolicy {

		inline static std::string GetFeaturesQueryPath(
					const char *const featureUuid) {
			boost::format result("/%1%%2%[@%3%=\"%4%\"]/%5%Set/%5%[@%6%=\"%7%\"]");
			result
				% "License"
				% "Key"
				% "Version"
				% "1.1"
				% "Feature"
				% "Name"
				% featureUuid;
			return result.str();
		}

		inline static std::string GetLicenseKeyRequestQueryPath() {
			boost::format result("/%1%%2%[@%3%=\"%4%\"]/%1%%2%%5%[@%3%=\"%4%\"]");
			result
				% "License"
				% "Key"
				% "Version"
				% "1.1"
				% "Request";
			return result.str();
		}

		inline static std::string GetLicenseQueryPath() {
			boost::format result("/%1%%2%[@%3%=\"%4%\"]/%1%%2%%5%[@%3%=\"%4%\"]/%1%");
			result
				% "License"
				% "Key"
				% "Version"
				% "1.1"
				% "Request";
			return result.str();
		}

		inline static std::string GetLimitationsWorkstationsQueryPath() {
			boost::format result("%1%/%1%%2%");
			result % "Workstation" % "Property";
			return result.str();
		}

		inline static std::string GetLicenseKeyLimitationsQueryPath() {
			boost::format result("/%1%%2%[@%3%=\"%4%\"]/%5%");
			result
				% "License"
				% "Key"
				% "Version"
				% "1.1"
				% "Limitations";
			return result.str();
		}

		inline static std::string GetLicenseKeyReleaseTimeQueryPath() {
			boost::format result("/%1%%2%[@%3%=\"%4%.%7%\"]/%5%/%6%");
			result
				% "License"
				% "Key"
				% "Version"
				% "1"
				% "Release"
				% "Time"
				% "1";
			return result.str();
		}

		inline static std::string GetLicenseKeyUpdateQueryPath() {
			boost::format result("/%1%%2%[@%3%=\"%4%.%6%\"]/%5%");
			result
				% "License"
				% "Key"
				% "Version"
				% "1"
				% "Update"
				% "1";
			return result.str();
		}

		inline static std::string GetLicenseKeyTypeQueryPath() {
			boost::format result("/%1%%2%[@%3%=\"%4%.%6%\"]/%5%");
			result
				% "License"
				% "Key"
				% "Version"
				% "1"
				% "Type"
				% "1";
			return result.str();
		}

		inline static std::string GetLicenseKeyOwnerQueryPath() {
			boost::format result("/%1%%2%[@%3%=\"%4%\"]/%5%");
			result
				% "License"
				% "Key"
				% "Version"
				% "1.1"
				% "Owner";
			return result.str();
		}

		inline static std::string GetLicenseKeyProductQueryPath() {
			boost::format result("/%1%%2%[@%3%=\"%4%\"]/%6%/%5%");
			result
				% "License"
				% "Key"
				% "Version"
				% "1.1"
				% "Product"
				% "Release";
			return result.str();
		}

		inline static std::string GetLicenseKeyWorkstationPropsScoresQueryPath() {
			boost::format result("/%1%%2%[@%3%=\"%4%\"]/%5%/%7%%5%/%6%%7%");
			result
				% "License"
				% "Key"
				% "Version"
				% "1.1"
				% "Options"
				% "Workstation"
				% "Scores";
			return result.str();
		}

	};

	//////////////////////////////////////////////////////////////////////////

	template<typename ClientTrait>
	struct EditionQueryPolicy {
	
		typedef EditionTrait<EDITION_STANDARD> StandardEdition;
		typedef EditionTrait<EDITION_PROF> ProfessionalEdition;
		typedef EditionTrait<EDITION_ENT> EnterpriseEdition;

		typedef QueryPathPolicy<ClientTrait> QueryPath;

		inline static Edition CastStringToEdition(
					const std::string &editionUuid)
				/*throw(std::bad_cast)*/ {
			if (editionUuid == StandardEdition::GetUuid()) {
				return StandardEdition::GetId();
			} else if (editionUuid == ProfessionalEdition::GetUuid()) {
				return ProfessionalEdition::GetId();
			} else if (editionUuid == EnterpriseEdition::GetUuid()) {
				return EnterpriseEdition::GetId();
			} else {
				BOOST_ASSERT(false);
				throw std::bad_cast();
			}
		}
		
		inline static const char * CastEditionToString(
					Edition editionUuid)
				/*throw(std::bad_cast)*/ {
			if (editionUuid == StandardEdition::GetId()) {
				return StandardEdition::GetUuid();
			} else if (editionUuid == ProfessionalEdition::GetId()) {
				return ProfessionalEdition::GetUuid();
			} else if (editionUuid == EnterpriseEdition::GetId()) {
				return EnterpriseEdition::GetUuid();
			} else {
				BOOST_ASSERT(false);
				throw std::bad_cast();
			}
		}
		
		inline static const char * CastEditionToName(
					Edition editionUuid)
				/*throw(std::bad_cast)*/ {
			if (editionUuid == StandardEdition::GetId()) {
				return StandardEdition::GetName();
			} else if (editionUuid == ProfessionalEdition::GetId()) {
				return ProfessionalEdition::GetName();
			} else if (editionUuid == EnterpriseEdition::GetId()) {
				return EnterpriseEdition::GetName();
			} else {
				BOOST_ASSERT(false);
				throw std::bad_cast();
			}
		}

		inline static Edition Parse(const TunnelEx::Helpers::Xml::Document &doc) {
			using namespace Helpers::Xml;
			WorkstationPropertyValues resultTmp;
			const boost::shared_ptr<const Node> node
				= doc.GetXPath()->Query(QueryPath::GetLicenseKeyProductQueryPath().c_str());
			if (!node || !node->HasAttribute("Edition")) {
				return EDITION_STANDARD;
			}
			std::string buffer;
			try {
				const Edition id
					= CastStringToEdition(node->GetAttribute("Edition", buffer));
				return id;
			} catch (const std::bad_cast &) {
				return EDITION_STANDARD;
			}
		}

	};

	//////////////////////////////////////////////////////////////////////////

	template<typename ClientTrait, bool isTestMode>
	struct WorkstationPropertiesQueryPolicy {
	
		typedef WorkstationPropertyTrait<WORKSTATION_PROPERTY_OS_VER> OsVerProperty;
		typedef WorkstationPropertyTrait<WORKSTATION_PROPERTY_INSTALLATION_INFO> OsInstallationProperty;
		typedef WorkstationPropertyTrait<WORKSTATION_PROPERTY_ADAPTER> AdapterProperty;
		typedef WorkstationPropertyTrait<WORKSTATION_PROPERTY_OS_VOLUME> OsVolumeProperty;

		typedef QueryPathPolicy<ClientTrait> QueryPath;

		inline static WorkstationProperty CastStringToProperty(
					const std::string &propertyUuid)
				/*throw(std::bad_cast)*/ {
			if (propertyUuid == OsVerProperty::GetUuid()) {
				return OsVerProperty::GetId();
			} else if (propertyUuid == OsInstallationProperty::GetUuid()) {
				return OsInstallationProperty::GetId();
			} else if (propertyUuid == AdapterProperty::GetUuid()) {
				return AdapterProperty::GetId();
			} else if (propertyUuid == OsVolumeProperty::GetUuid()) {
				return OsVolumeProperty::GetId();
			} else {
				BOOST_ASSERT(false);
				throw std::bad_cast();
			}
		}
		
		inline static const char * CastPropertyToString(
					WorkstationProperty prop)
				/*throw(std::bad_cast)*/ {
			if (prop == OsVerProperty::GetId()) {
				return OsVerProperty::GetUuid();
			} else if (prop == OsInstallationProperty::GetId()) {
				return OsInstallationProperty::GetUuid();
			} else if (prop == AdapterProperty::GetId()) {
				return AdapterProperty::GetUuid();
			} else if (prop == OsVolumeProperty::GetId()) {
				return OsVolumeProperty::GetUuid();
			} else {
				BOOST_ASSERT(false);
				throw std::bad_cast();
			}
		}
		
		inline static bool Parse(
					const TunnelEx::Helpers::Xml::Node &parentNode,
					WorkstationPropertyValues &result) {
			using namespace Helpers::Xml;
			WorkstationPropertyValues resultTmp;
			ConstNodeCollection nodes;
			parentNode.GetXPath()->Query(
				QueryPath::GetLimitationsWorkstationsQueryPath().c_str(),
				nodes);
			std::string buffer;
			foreach (boost::shared_ptr<const Node> &node, nodes) {
				if (!node->HasAttribute("Name")) {
					return false;
				}
				try {
					const WorkstationProperty id
						= CastStringToProperty(node->GetAttribute("Name", buffer));
					BOOST_ASSERT(resultTmp.find(id) == resultTmp.end());
					resultTmp[id] = node->GetContent(buffer);
				} catch (const std::bad_cast &) {
					return false;
				}
			}
			resultTmp.swap(result);
			return true;
		}

	};

	//////////////////////////////////////////////////////////////////////////

	template<typename ClientTrait>
	struct TimeIntervalQueryPolicy {

		inline static bool Parse(
					const TunnelEx::Helpers::Xml::Node &node,
					TimeInterval &result) {
			namespace pt = boost::posix_time;
			if (!node.HasAttribute("From") && !node.HasAttribute("To")) {
				return false;
			}
			try {
				TimeInterval resultTmp;
				std::string buffer;
				if (node.HasAttribute("From")) {
					resultTmp.first
						= pt::time_from_string(node.GetAttribute("From", buffer));
				}
				if (node.HasAttribute("To")) {
					resultTmp.second
						= pt::time_from_string(node.GetAttribute("To", buffer));
				}
				resultTmp.swap(result);
				return true;
			} catch (const std::bad_cast &) {
				return false;
			}
		}

	};

	//////////////////////////////////////////////////////////////////////////

	template<typename ClientTrait>
	struct LimitationsQueryPolicy {

		typedef typename ClientTrait::WorkstationPropertiesQuery
			WorkstationPropertiesQuery;
		typedef typename ClientTrait::FeatureValue FeatureValue;
		typedef typename ValueCastingPolicy<ClientTrait> ValueCasting;
		typedef typename TimeIntervalQueryPolicy<ClientTrait> TimeIntervalQuery;

		inline static bool Parse(
					const TunnelEx::Helpers::Xml::Node &limitationsNode,
					Limitations &result) {
			
			using namespace Helpers::Xml;
			
			Limitations resultTmp;
			std::string buffer;
			boost::shared_ptr<const XPath> xpath = limitationsNode.GetXPath();

			boost::shared_ptr<const Node> node = xpath->Query("TimeInterval");
			if (node && !TimeIntervalQuery::Parse(*node, resultTmp.timeInterval)) {
				return false;
			}

			node = xpath->Query("Scores");
			if (node) {
				try {
					resultTmp.scores
						= ValueCasting::Cast<Scores>(node->GetContent(buffer));
				} catch (const std::bad_cast &) {
					return false;
				}
			}

			if (	!WorkstationPropertiesQuery::Parse(
						limitationsNode,
						resultTmp.workstationProperties)) {
				return false;
			}

			std::swap(resultTmp, result);
			return true;

		}
	};

	//////////////////////////////////////////////////////////////////////////

	template<typename ClientTrait>
	struct FeatureQueryPolicy {

		typedef typename ClientTrait::FeatureInfo FeatureInfo;
		typedef typename ClientTrait::Feature Feature;
		typedef typename Feature::Value FeatureValue;
		typedef typename ClientTrait::LimitationsQuery LimitationsQuery;
		typedef QueryPathPolicy<ClientTrait> QueryPath;
		typedef typename ValueCastingPolicy<ClientTrait> ValueCasting;

		inline static std::auto_ptr<const FeatureInfo> Parse(
					const TunnelEx::Helpers::Xml::Document &doc) {

			using namespace Helpers::Xml;

			std::auto_ptr<FeatureInfo> result(new FeatureInfo);

			const boost::shared_ptr<const Helpers::Xml::Node> featureNode
				= doc.GetXPath()->Query(
					QueryPath::GetFeaturesQueryPath(Feature::GetUuid()).c_str());
			if (!featureNode) {
				
				if (!Feature::GetDefaultValue()) {
					return std::auto_ptr<const FeatureInfo>();
				}
				
				result->value = *Feature::GetDefaultValue();
			
			} else {

				boost::shared_ptr<const XPath> xpath = featureNode->GetXPath();

				boost::shared_ptr<const Node> node = xpath->Query("Limitations");
				if (node && !LimitationsQuery::Parse(*node, result->limitations)) {
					return std::auto_ptr<const FeatureInfo>();
				}
				
				node = xpath->Query("Value");
				if (!node) {
					return std::auto_ptr<const FeatureInfo>();
				}
				try {
					std::string buffer;				
					result->value
						= ValueCasting::Cast<FeatureValue>(node->GetContent(buffer));
				} catch (const std::bad_cast &) {
					return std::auto_ptr<const FeatureInfo>();
				}

			}

			return result;

		}

	};

	//////////////////////////////////////////////////////////////////////////

	template<typename ClientTrait>
	struct LicenseKeyQueryPolicy {

		typedef typename ClientTrait::LimitationsQuery LimitationsQuery;
		typedef QueryPathPolicy<ClientTrait> QueryPath;
		typedef typename ClientTrait::LicenseKeyInfo LicenseKeyInfo;
		typedef typename TimeIntervalQueryPolicy<ClientTrait> TimeIntervalQuery;

		inline static std::auto_ptr<const LicenseKeyInfo> Parse(
					const TunnelEx::Helpers::Xml::Document &doc) {
			
			using namespace Helpers::Xml;

			std::auto_ptr<LicenseKeyInfo> result(new LicenseKeyInfo);

			boost::shared_ptr<const XPath> xpath = doc.GetXPath();
			std::string buffer;

			/* boost::shared_ptr<const Node> node
				= doc.GetXPath()->Query(QueryPath::GetLicenseKeyProductQueryPath().c_str());
			if (	!node
					|| !node->HasAttribute("Name")!= 
					|| node->GetAttribute("Name", buffer) ) {
				return EDITION_STANDARD;
			} */

			boost::shared_ptr<const Node> node
				= xpath->Query(QueryPath::GetLicenseKeyTypeQueryPath().c_str());
			if (!node) {
				return std::auto_ptr<const LicenseKeyInfo>();
			} else {
				node->GetContent(buffer);
				if (buffer == "trial") {
					result->isTrial = true;
				} else if (buffer == "regular") {
					result->isTrial = false;
				} else {
					return std::auto_ptr<const LicenseKeyInfo>();
				}
			}

			node = xpath->Query(QueryPath::GetLicenseKeyLimitationsQueryPath().c_str());
			if (!node || !LimitationsQuery::Parse(*node, result->limitations)) {
				return std::auto_ptr<const LicenseKeyInfo>();
			}

			node = xpath->Query(QueryPath::GetLicenseKeyUpdateQueryPath().c_str());
			if (node && !TimeIntervalQuery::Parse(*node, result->update)) {
				return std::auto_ptr<const LicenseKeyInfo>();
			}

			node = xpath->Query(QueryPath::GetLicenseKeyReleaseTimeQueryPath().c_str());
			if (!node) {
				return std::auto_ptr<const LicenseKeyInfo>();
			}
			try {
				result->releaseTime
					= boost::posix_time::time_from_string(node->GetContent(buffer));
			} catch (const std::bad_cast &) {
				return std::auto_ptr<const LicenseKeyInfo>();
			}

			node = xpath->Query(QueryPath::GetLicenseKeyOwnerQueryPath().c_str());
			if (!node) {
				return std::auto_ptr<const LicenseKeyInfo>();
			}
			node->GetContent(result->owner);

			node = node->GetParent();
			if (!node || !node->HasAttribute("Id")) {
				return std::auto_ptr<const LicenseKeyInfo>();
			}
			node->GetAttribute("Id", result->id);

			result->edition = ClientTrait::EditionQuery::Parse(doc);

			node = xpath->Query(QueryPath::GetLicenseQueryPath().c_str());
			if (!node) {
				return std::auto_ptr<const LicenseKeyInfo>();
			}
			node->GetContent(result->license);

			return result;

		}

	};

	//////////////////////////////////////////////////////////////////////////

	template<typename ClientTrait>
	struct OptionsQueryPolicy {

		typedef typename ClientTrait::WorkstationPropertiesQuery
			WorkstationPropertiesQuery;
		typedef QueryPathPolicy<ClientTrait> QueryPath;
		typedef typename ClientTrait::FeatureValue FeatureValue;
		typedef typename ValueCastingPolicy<ClientTrait> ValueCasting;

		inline static std::auto_ptr<const Options> Parse(
					const TunnelEx::Helpers::Xml::Document &doc) {

			using namespace Helpers::Xml;

			std::auto_ptr<Options> result(new Options);

			ConstNodeCollection nodes;
			doc.GetXPath()->Query(
				QueryPath::GetLicenseKeyWorkstationPropsScoresQueryPath().c_str(),
				nodes);
			std::string buffer;
			foreach (boost::shared_ptr<const Node> &node, nodes) {
				if (!node->HasAttribute("Property")) {
					return std::auto_ptr<const Options>();
				}
				try {
					const WorkstationProperty id
						= WorkstationPropertiesQuery::CastStringToProperty(
							node->GetAttribute("Property", buffer));
					BOOST_ASSERT(result->workstationPropertiesScores.find(id)
						== result->workstationPropertiesScores.end());
					result->workstationPropertiesScores[id]
						= ValueCasting::Cast<Scores>(node->GetContent(buffer));
				} catch (const std::bad_cast &) {
					return std::auto_ptr<const Options>();
				}
			}

			return result;

		}

	};

	//////////////////////////////////////////////////////////////////////////

	template<typename ClientTrait, bool isTestMode>
	struct LocalInfoQueryPolicy {

		typedef typename ClientTrait::Feature Feature;
		typedef typename Feature::Value FeatureValue;
		typedef typename ClientTrait::LocalInfo LocalInfo;
		typedef typename ClientTrait::WorkstationPropertiesQuery
			WorkstationPropertiesQuery;
		typedef typename ClientTrait::WorkstationPropertiesLocal
			WorkstationPropertiesLocal;
		typedef typename ClientTrait::Check Check;
		typedef QueryPathPolicy<ClientTrait> QueryPath;

		inline static std::auto_ptr<const LocalInfo> Get(
					const TunnelEx::Helpers::Xml::Document &doc,
					const Options &options,
					const boost::any &clientParam) {

			using namespace Helpers::Xml;

			std::auto_ptr<LocalInfo> result(new LocalInfo);

			if (	!WorkstationPropertiesLocal::Get(
						result->workstationProperties, clientParam)) {
				return std::auto_ptr<const LocalInfo>();
			}

			WorkstationPropertyValues controlWorkstationProperties;
			const boost::shared_ptr<const Node> licenseKeyRequestNode
				= doc.GetXPath()->Query(
					QueryPath::GetLicenseKeyRequestQueryPath().c_str());
			if (	!licenseKeyRequestNode
					||	!WorkstationPropertiesQuery::Parse(
							*licenseKeyRequestNode,
							controlWorkstationProperties)
					|| !Check::CalculateScores(
							controlWorkstationProperties,
							options.workstationPropertiesScores,
							result->workstationProperties,
							result->scores)) {
				return std::auto_ptr<const LocalInfo>();
			}
			
			result->time = boost::posix_time::second_clock::universal_time();

			return result;

		}

	};

	//////////////////////////////////////////////////////////////////////////

} }

#endif // #ifndef INCLUDED_FILE__TUNNELEX__QueryPolicies_hpp__0909091830
