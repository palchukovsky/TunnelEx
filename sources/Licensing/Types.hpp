/**************************************************************************
 *   Created: 2009/09/08 19:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__CheckPolicies_hpp__0909081944
#define INCLUDED_FILE__TUNNELEX__CheckPolicies_hpp__0909081944

namespace TunnelEx { namespace Licensing {

	enum UnactivityReason {
		UR_NO,
		// feature just unavailable (not found)
		UR_FEATURE,
		//! from license key format (XML, encrypted or sign)
		UR_FORMAT,
		//! time limit
		UR_TIME,
		//! should be updated
		UR_UPDATE,
		//! too more scores
		UR_SCORES,
		//! workstation property is changed
		UR_WORSTATION,
		//! license not exists
		UR_NOT_EXISTS
	};

	enum Product {
		PRODUCT_TUNNELEX
	};

	enum Edition {
		EDITION_STANDARD,
		EDITION_PROF,
		EDITION_ENT
	};

	enum WorkstationProperty {
		WORKSTATION_PROPERTY_OS_VER,
		WORKSTATION_PROPERTY_INSTALLATION_INFO,
		WORKSTATION_PROPERTY_ADAPTER,
		WORKSTATION_PROPERTY_OS_VOLUME
	};

	enum Feature {
		FEATURE_EXECUTION,
		FEATURE_EXECUTION_SERVER_OS,
		FEATURE_TUNNEL_COUNT,
		FEATURE_RULE_COUNT,
		FEATURE_PROXY_ENABLED,
		FEATURE_PROXY_CASCADE_ENABLED,
		FEATURE_FTP_TUNNELING_ENABLED,
		FEATURE_IO_CHANNELS_SEPARATION,
		FEATURE_SSL,
		FEATURE_NONE
	};

	enum Client {
		//! start enabling check
		CLIENT_START						= 1,
		//! service start
		CLIENT_SERVER_START					= 2,
		//! allowed tunnels number check
		CLIENT_TUNNEL						= 3,
		//! allowed rules number check
		CLIENT_RULESET						= 4,
		//! proxy enabling check
		CLIENT_PROXY						= 5,
		//! proxy cascade enabling check
		CLIENT_PROXY_CASCADE				= 6,
		//! online key request
		CLIENT_ONLINE_KEY_REQUST			= 7,
		//! offline key request
		CLIENT_OFFLINE_KEY_REQUST			= 8,
		//! License info dialog
		CLIENT_LICENSE_INFO_DLG				= 9,
		//! Service implementation
		CLIENT_SERVICE						= 10,
		//! Information about license
		CLIENT_INFO							= 11,
		//! Slit endpoint feature
		CLIENT_ENDPOINT_IO_SEPARATION		= 12,
		//! Ftp-tunneling
		CLIENT_FTP_TUNNEL					= 13,
		//! Pathfinder service
		CLIENT_PATHFINDER					= 14,
		CLIENT_SSL							= 15
	};

	typedef std::map<WorkstationProperty, std::string> WorkstationPropertyValues;

	typedef size_t Scores;

	typedef std::pair<
			boost::optional<boost::posix_time::ptime>, 
			boost::optional<boost::posix_time::ptime> >
		TimeInterval;

	struct Limitations {
		TimeInterval timeInterval;
		boost::optional<Scores> scores;
		WorkstationPropertyValues workstationProperties;
	};

	template<class Value>
	struct FeatureInfo {
		Limitations limitations;
		typename Value value;
	};

	struct LicenseKeyInfo {
		std::string id;
		std::string license;
		std::string owner;
		Edition edition;
		Limitations limitations;
		TimeInterval update;
		boost::posix_time::ptime releaseTime;
		bool isTrial;
	};

	typedef std::map<WorkstationProperty, Scores> WorkstationPropertiesScores;

	struct Options {
		WorkstationPropertiesScores workstationPropertiesScores;
	};

	struct LocalInfo {
		boost::posix_time::ptime time;
		Scores scores;
		WorkstationPropertyValues workstationProperties;
	};

	struct Error {
		Client client;
		std::string license;
		std::string time;
		std::string point;
		std::string code;
	
	};

} }

#endif // #ifndef INCLUDED_FILE__TUNNELEX__CheckPolicies_hpp__0909081944
