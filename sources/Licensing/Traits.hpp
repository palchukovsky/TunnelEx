/**************************************************************************
 *   Created: 2009/09/08 18:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Traits_hpp__0909081852
#define INCLUDED_FILE__TUNNELEX__Traits_hpp__0909081852

#include "Types.hpp"

namespace TunnelEx { namespace Licensing {

	//////////////////////////////////////////////////////////////////////////

	template<class ClientTrait, bool isTestMode>
	struct LocalStoragePolicy;

	template<class ClientTrait, bool isTestMode>
	struct ConstantStoragePolicy;

	template<class ClientTrait, bool isTestMode>
	struct XmlLicenseKeyRetrievePolicy;

	template<class ClientTrait>
	struct CheckPolicy;

	template<typename ClientTrait>
	struct LicenseKeyQueryPolicy;

	template<typename ClientTrait>
	struct FeatureQueryPolicy;

	template<typename ClientTrait>
	struct OptionsQueryPolicy;

	template<typename ClientTrait, bool isTestMode>
	struct LocalInfoQueryPolicy;

	template<class ClientTrait, bool isTestMode>
	struct WorkstationPropertiesQueryPolicy;

	template<class ClientTrait, bool isTestMode>
	struct WorkstationPropertiesLocalPolicy;

	template<class ClientTrait>
	struct LimitationsQueryPolicy;

	template<class ClientTrait>
	class License;

	template<class ClientTrait>
	class KeyRequest;
	
	template<class ClientTrait, bool isTestMode>
	struct WinInetCommPolicy;
	
	template<class ClientTrait, bool isTestMode>
	struct RequestGenerationPolicy;
	
	template<Client client, bool isTestMode = false>
	struct ClientTrait;

	template<typename ClientTrait>
	struct EditionQueryPolicy;

	template<typename ClientTrait>
	struct NotificationPolicy;

	//////////////////////////////////////////////////////////////////////////

	template<Product product>
	struct ProductTrait {
		//...//
	};

	template<>
	struct ProductTrait<PRODUCT_TUNNELEX> {

		static const char *const GetUuid() {
			return "185B591A-9206-499E-915B-450A52DDB916";
		}

	};

	//////////////////////////////////////////////////////////////////////////

	template<Edition edition>
	struct EditionTrait {
		//...//
	};

	template<>
	struct EditionTrait<EDITION_STANDARD> {

		static const char *const GetUuid() {
			return "8805D784-A027-11DE-AC3C-8BAA55D89593";
		}

		static Edition GetId() {
			return EDITION_STANDARD;
		}

		static const char * GetName() {
			return "Standard";
		}

	};

	template<>
	struct EditionTrait<EDITION_PROF> {

		static const char *const GetUuid() {
			return "4B7D5CEF-9EC3-4BB8-A051-189C4D0E591B";
		}

		static Edition GetId() {
			return EDITION_PROF;
		}

		static const char * GetName() {
			return "Professional";
		}

	};

	template<>
	struct EditionTrait<EDITION_ENT> {

		static const char *const GetUuid() {
			return "1DC88948-53F6-4016-AC4C-6BEF644E044D";
		}

		static Edition GetId() {
			return EDITION_ENT;
		}

		static const char * GetName() {
			return "Enterprise";
		}

	};

	//////////////////////////////////////////////////////////////////////////

	template<WorkstationProperty workstationProperty>
	struct WorkstationPropertyTrait {
		//...//
	};

	template<>
	struct WorkstationPropertyTrait<WORKSTATION_PROPERTY_OS_VER> {

		static const char *const GetUuid() {
			return "B7BB6274-2943-4E9F-8408-6A3F7587A3FD";
		}

		static WorkstationProperty GetId() {
			return WORKSTATION_PROPERTY_OS_VER;
		}

	};

	template<>
	struct WorkstationPropertyTrait<WORKSTATION_PROPERTY_INSTALLATION_INFO> {

		static const char *const GetUuid() {
			return "4790BEFA-ACE6-457A-A3DE-BEEE0B90591D";
		}

		static WorkstationProperty GetId() {
			return WORKSTATION_PROPERTY_INSTALLATION_INFO;
		}

	};

	template<>
	struct WorkstationPropertyTrait<WORKSTATION_PROPERTY_ADAPTER> {

		static const char *const GetUuid() {
			return "80187FDC-8BDE-4A8B-9082-4C05765C39B5";
		}

		static WorkstationProperty GetId() {
			return WORKSTATION_PROPERTY_ADAPTER;
		}

	};

	template<>
	struct WorkstationPropertyTrait<WORKSTATION_PROPERTY_OS_VOLUME> {

		static const char *const GetUuid() {
			return "81D96D9C-3A5D-4FB2-8A9B-8658470B972F";
		}

		static WorkstationProperty GetId() {
			return WORKSTATION_PROPERTY_OS_VOLUME;
		}

	};

	//////////////////////////////////////////////////////////////////////////

	template<Feature feature>
	struct FeatureTrait {
		
		// Default feature has only value type - just for compilation, not for usage.
		
		typedef bool Value;

		static const char *const GetUuid() {
			return "";
		}

		inline static boost::optional<Value> GetDefaultValue() {
			return boost::optional<Value>();
		}

	};

	template<>
	struct FeatureTrait<FEATURE_EXECUTION> {

		typedef bool Value;

		static const char *const GetUuid() {
			return "91D848B9-3F43-4594-84D2-FF8EB6AD2F61";
		}

		inline static boost::optional<Value> GetDefaultValue() {
			return boost::optional<Value>();
		}

	};

	template<>
	struct FeatureTrait<FEATURE_EXECUTION_SERVER_OS> {

		typedef bool Value;

		static const char *const GetUuid() {
			return "234ECD32-AB75-43C2-BE1E-AB1343BACCD3";
		}

		inline static boost::optional<Value> GetDefaultValue() {
			return boost::optional<Value>();
		}

	};

	template<>
	struct FeatureTrait<FEATURE_FTP_TUNNELING_ENABLED> {

		typedef bool Value;

		static const char *const GetUuid() {
			return "F1A6E2D2-CAF2-11DE-8353-1CF755D89593";
		}

		inline static boost::optional<Value> GetDefaultValue() {
			return boost::optional<Value>();
		}

	};

	template<>
	struct FeatureTrait<FEATURE_IO_CHANNELS_SEPARATION> {

		typedef bool Value;

		static const char *const GetUuid() {
			return "B4340B2A-CAEB-11DE-98B0-DAA555D89593";
		}

		inline static boost::optional<Value> GetDefaultValue() {
			return boost::optional<Value>();
		}

	};

	template<>
	struct FeatureTrait<FEATURE_TUNNEL_COUNT> {

		typedef size_t Value;

		static const char *const GetUuid() {
			return "565E33DA-FC88-4E7D-B613-39F39D9F496B";
		}

		inline static boost::optional<Value> GetDefaultValue() {
			return Value(0);
		}

	};

	template<>
	struct FeatureTrait<FEATURE_RULE_COUNT> {

		typedef size_t Value;

		static const char *const GetUuid() {
			return "B70D4EE4-377C-4E11-B8DC-2E36E06BFDD8";
		}

		inline static boost::optional<Value> GetDefaultValue() {
			return Value(0);
		}

	};

	template<>
	struct FeatureTrait<FEATURE_PROXY_ENABLED> {

		typedef bool Value;

		static const char *const GetUuid() {
			return "E55F3478-E0EA-442E-A14E-AD961029F1AF";
		}

		inline static boost::optional<Value> GetDefaultValue() {
			return boost::optional<Value>();
		}

	};

	template<>
	struct FeatureTrait<FEATURE_PROXY_CASCADE_ENABLED> {

		typedef bool Value;

		static const char *const GetUuid() {
			return "03397D50-BC5D-11DF-9DBD-DC20E0D72085";
		}

		inline static boost::optional<Value> GetDefaultValue() {
			return boost::optional<Value>();
		}

	};

	template<>
	struct FeatureTrait<FEATURE_SSL> {

		typedef bool Value;

		static const char *const GetUuid() {
			return "F478F4C6-D94E-11DF-B960-D843DFD72085";
		}

		inline static boost::optional<Value> GetDefaultValue() {
			return boost::optional<Value>();
		}

	};

	//////////////////////////////////////////////////////////////////////////

	template<Client client>
	struct Client2Feature {
		// no feature for this client
		typedef FeatureTrait<FEATURE_NONE> Feature;
	};

	template<>
	struct Client2Feature<CLIENT_START> {
		typedef FeatureTrait<FEATURE_EXECUTION> Feature;
	};

	template<>
	struct Client2Feature<CLIENT_TUNNEL> {
		typedef FeatureTrait<FEATURE_TUNNEL_COUNT> Feature;
	};

	template<>
	struct Client2Feature<CLIENT_SERVER_START> {
		typedef FeatureTrait<FEATURE_EXECUTION_SERVER_OS> Feature;
	};

	template<>
	struct Client2Feature<CLIENT_RULESET> {
		typedef FeatureTrait<FEATURE_RULE_COUNT> Feature;
	};

	template<>
	struct Client2Feature<CLIENT_PROXY> {
		typedef FeatureTrait<FEATURE_PROXY_ENABLED> Feature;
	};

	template<>
	struct Client2Feature<CLIENT_PROXY_CASCADE> {
		typedef FeatureTrait<FEATURE_PROXY_CASCADE_ENABLED> Feature;
	};

	template<>
	struct Client2Feature<CLIENT_LICENSE_INFO_DLG> {
		typedef FeatureTrait<FEATURE_EXECUTION> Feature;
	};

	template<>
	struct Client2Feature<CLIENT_INFO> {
		typedef FeatureTrait<FEATURE_EXECUTION> Feature;
	};

	template<>
	struct Client2Feature<CLIENT_ENDPOINT_IO_SEPARATION> {
		typedef FeatureTrait<FEATURE_IO_CHANNELS_SEPARATION> Feature;
	};

	template<>
	struct Client2Feature<CLIENT_FTP_TUNNEL> {
		typedef FeatureTrait<FEATURE_FTP_TUNNELING_ENABLED> Feature;
	};

	template<>
	struct Client2Feature<CLIENT_PATHFINDER> {
		typedef Client2Feature<CLIENT_PROXY>::Feature Feature;
	};

	template<>
	struct Client2Feature<CLIENT_SSL> {
		typedef FeatureTrait<FEATURE_SSL> Feature;
	};

	//////////////////////////////////////////////////////////////////////////
	
	template<Client client, bool isTestMode>
	struct Client2Comm {
		// no communication for this client
		typedef void Policy;
	};
	
	template<bool isTestMode>
	struct Client2Comm<CLIENT_ONLINE_KEY_REQUST, isTestMode> {
		typedef WinInetCommPolicy<
				ClientTrait<CLIENT_ONLINE_KEY_REQUST, isTestMode>,
				isTestMode>
			Policy;
	};

	//////////////////////////////////////////////////////////////////////////

	template<Client client, bool isTestMode>
	struct ClientTrait {
		typedef ClientTrait<client, isTestMode> MyType;
		typedef License<MyType> License;
		typedef KeyRequest<MyType> KeyRequest;
		typedef LocalStoragePolicy<MyType, isTestMode> LocalStorage;
		typedef ConstantStoragePolicy<MyType, isTestMode> ConstantStorage;
		typedef XmlLicenseKeyRetrievePolicy<MyType, isTestMode> KeyRetrieve;
		typedef typename Client2Feature<client>::Feature Feature;
		typedef typename Feature::Value FeatureValue;
		typedef FeatureInfo<FeatureValue> FeatureInfo;
		typedef FeatureQueryPolicy<MyType> FeatureQuery;
		typedef LicenseKeyInfo LicenseKeyInfo;
		typedef LicenseKeyQueryPolicy<MyType> LicenseKeyQuery;
		typedef OptionsQueryPolicy<MyType> OptionsQuery;
		typedef LocalInfo LocalInfo;
		typedef LocalInfoQueryPolicy<MyType, isTestMode> LocalInfoQuery;
		typedef CheckPolicy<MyType> Check;
		typedef WorkstationPropertiesQueryPolicy<MyType, isTestMode>
			WorkstationPropertiesQuery;
		typedef WorkstationPropertiesLocalPolicy<MyType, isTestMode>
			WorkstationPropertiesLocal;
		typedef LimitationsQueryPolicy<MyType> LimitationsQuery;
		typedef typename Client2Comm<client, isTestMode>::Policy Comminication;
		typedef RequestGenerationPolicy<MyType, isTestMode> RequestGeneration;
		typedef EditionQueryPolicy<MyType> EditionQuery;
		typedef NotificationPolicy<MyType> Notification;
	};

	//////////////////////////////////////////////////////////////////////////

} }

#endif // #ifndef INCLUDED_FILE__TUNNELEX__Traits_hpp__0909081852
