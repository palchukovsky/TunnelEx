/**************************************************************************
 *   Created: 2009/08/05 11:57
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: License.hpp 1036 2010-10-19 17:26:19Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__License_hpp__0908051157
#define INCLUDED_FILE__TUNNELEX__License_hpp__0908051157

#include "CheckPolicies.hpp"
#include "KeyRetrievePolicies.hpp"
#include "QueryPolicies.hpp"
#include "StoragePolicies.hpp"
#include "Traits.hpp"
#include "Request.hpp"

namespace TunnelEx { namespace Licensing {

	//////////////////////////////////////////////////////////////////////////

	typedef ClientTrait<CLIENT_START, false>::License ExeLicense;
	typedef ClientTrait<CLIENT_START, true>::License ExeLicenseTesting;
	typedef ClientTrait<CLIENT_SERVER_START, false>::License ServiceStartLicense;
	typedef ClientTrait<CLIENT_TUNNEL, false>::License TunnelLicense;
	typedef ClientTrait<CLIENT_TUNNEL, true>::License TunnelLicenseTesting;
	typedef ClientTrait<CLIENT_RULESET, false>::License RuleSetLicense;
	typedef ClientTrait<CLIENT_RULESET, true>::License RuleSetLicenseTesting;
	typedef ClientTrait<CLIENT_PROXY, false>::License ProxyLicense;
	typedef ClientTrait<CLIENT_PROXY, true>::License ProxyLicenseTesting;
	typedef ClientTrait<CLIENT_PROXY_CASCADE, false>::License ProxyCascadeLicense;
	typedef ClientTrait<CLIENT_LICENSE_INFO_DLG, false>::License InfoDlgLicense;
	typedef ClientTrait<CLIENT_INFO, false>::License InfoLicense;
	typedef ClientTrait<CLIENT_ENDPOINT_IO_SEPARATION, false>::License EndpointIoSeparationLicense;
	typedef ClientTrait<CLIENT_ENDPOINT_IO_SEPARATION, true>::License EndpointIoSeparationLicenseTesting;
	typedef ClientTrait<CLIENT_FTP_TUNNEL, false>::License FtpTunnelLicense;
	typedef ClientTrait<CLIENT_PATHFINDER, false>::License PathfinderLicense;
	typedef ClientTrait<CLIENT_SSL, false>::License SslLicense;

	//////////////////////////////////////////////////////////////////////////

	template<class ClientTrait>
	class License : private boost::noncopyable {

	public:

		typedef typename ClientTrait Client;
		typedef typename Client::KeyRetrieve KeyRetrieve;
		typedef typename Client::Feature Feature;
		typedef typename Feature::Value FeatureValue;
		typedef typename Client::FeatureInfo FeatureInfo;
		typedef typename Client::FeatureQuery FeatureQuery;
		typedef typename Client::LicenseKeyInfo LicenseKeyInfo;
		typedef typename Client::LicenseKeyQuery LicenseKeyQuery;
		typedef typename Client::OptionsQuery OptionsQuery;
		typedef typename Client::LocalInfo LocalInfo;
		typedef typename Client::LocalInfoQuery LocalInfoQuery;
		typedef typename Client::Check Check;
		typedef typename Client::WorkstationPropertiesQuery WorkstationPropertiesQuery;
		typedef typename Client::WorkstationPropertiesLocal WorkstationPropertiesLocal;
		typedef typename Client::EditionQuery EditionQuery;
		typedef typename Client::LocalStorage LocalStorage;
				
	private:

		struct Cache : private boost::noncopyable {

			explicit inline Cache(
						bool isActive,
						const FeatureValue &value,
						UnactivityReason unactivityReason,
						const boost::optional<FeatureValue> &defaultValue)
					: since(boost::posix_time::second_clock::universal_time()),
					checkCount(0),
					value(value),
					defaultValue(defaultValue),
					isActive(isActive),
					unactivityReason(unactivityReason) {
				BOOST_ASSERT(isActive || unactivityReason != UR_NO);
			}
			
			explicit inline Cache(
						bool isActive,
						UnactivityReason unactivityReason,
						const boost::optional<FeatureValue> defaultValue)
					: since(boost::posix_time::second_clock::universal_time()),
					checkCount(0),
					value(defaultValue),
					defaultValue(defaultValue),
					isActive(isActive),
					unactivityReason(unactivityReason) {
				BOOST_ASSERT(isActive || unactivityReason != UR_NO);
			}

		public:

			const boost::posix_time::ptime since;
			unsigned char checkCount;
			const boost::optional<FeatureValue> value;
			const boost::optional<FeatureValue> defaultValue;
			const bool isActive;
			const UnactivityReason unactivityReason;

		};

	public:

		inline License() {
			//...//
		}

		explicit inline License(const boost::any &clientParam)
				: m_clientParam(clientParam) {
			//...//
		}

		inline ~License() {
			//...//
		}

	public:

		//! Common check for feature availability.
		/** Automates caching, checks workstation properties and other limits.
		 *  Should be used each time when client has to check availability,
		 *  to make a decision to executing.
		 *
		 *  @param	valueToCheck	value to check
		 *  @return	true if feature available, false otherwise
		 */
		inline bool IsFeatureAvailable(const FeatureValue &valueToCheck) const {
			const_cast<License *>(this)->UpdateCache();
			const bool result = IsFeatureValueAvailable(*m_cache, valueToCheck);
			++m_cache->checkCount;
			return result;
		}

		//! Check for feature value availability.
		/** Works only with cache. If cache not exists - always returns false.
		 *
		 *  @param	valueToCheck	value to check
		 *  @return	true if feature available, false otherwise
		 */
		inline bool IsFeatureValueAvailable(const FeatureValue &valueToCheck) const {
			BOOST_ASSERT(m_cache.get());
			return m_cache.get() && IsFeatureValueAvailable(*m_cache, valueToCheck);
		}

		inline bool GetFeatureValue(FeatureValue &value) {
			BOOST_ASSERT(m_cache.get());
			if (	!m_cache.get()
					||	!m_cache->isActive
					||	!m_cache->value) {
				return false;
			}
			value = *m_cache->value;
			return true;
		}

		UnactivityReason GetUnactivityReason() const {
			BOOST_ASSERT(m_cache.get());
			return !m_cache.get() ? UR_NO : m_cache->unactivityReason;
		}

		inline void ResetFeatureAvailabilityCache() {
			using namespace std;
			auto_ptr<const Options> options;
			auto_ptr<const LocalInfo> localInfo;
			auto_ptr<const LicenseKeyInfo> licenseInfo;
			auto_ptr<const FeatureInfo> featureInfo;
			LocalStorage::ResetLicenseKeyUpdateState(m_clientParam);
			const bool isExists = ReadKey(
				options,
				localInfo,
				licenseInfo,
				featureInfo,
				m_clientParam);
			auto_ptr<Cache> cache;
			if (!isExists) {
				cache.reset(new Cache(false, UR_NOT_EXISTS, Feature::GetDefaultValue()));
			} else if (!localInfo.get() || !licenseInfo.get() || !options.get()) {
				cache.reset(new Cache(false, UR_FORMAT, Feature::GetDefaultValue()));
			} else if (!featureInfo.get()) {
				cache.reset(new Cache(false, UR_FEATURE, Feature::GetDefaultValue()));
			} else if (!Check::IsLicenseKeyActive(*licenseInfo, *localInfo)) {
				cache = GetCacheWithError(
					*licenseInfo,
					*localInfo,
					featureInfo->value,
					Feature::GetDefaultValue());
			} else if (!Check::IsFeatureActive(*featureInfo, *localInfo)) {
				cache = GetCacheWithError(
					*featureInfo,
					*localInfo,
					featureInfo->value,
					Feature::GetDefaultValue());
			} else {
				cache.reset(
					new Cache(
						true,
						featureInfo->value,
						UR_NO,
						Feature::GetDefaultValue()));
			}
			m_cache = cache;
		}

		inline size_t GetMaximumChecksCountFromCache() const {
			return 250;
		}

		inline static bool TestKey(
					const std::string &key,
					boost::any clientParam = boost::any()) {
			using namespace std;
			auto_ptr<const Options> options;
			auto_ptr<const LocalInfo> localInfo;
			auto_ptr<const LicenseKeyInfo> licenseInfo;
			auto_ptr<const FeatureInfo> featureInfo;
			const bool isExists = ReadKey(
				options,
				localInfo,
				licenseInfo,
				featureInfo,
				clientParam,
				&key);
			return isExists && localInfo.get() && licenseInfo.get() && options.get();
		}

		inline std::string GetOwner() const {
			const std::auto_ptr<const LicenseKeyInfo> licenseInfo
				= GetLicenseInfo();
			return licenseInfo.get() ? licenseInfo->owner : string();
		}

		inline Edition GetEdition() const {
			const std::auto_ptr<const LicenseKeyInfo> licenseInfo
				= GetLicenseInfo();
			return licenseInfo.get()
				?	licenseInfo->edition
				:	EDITION_STANDARD;
		}

		inline std::string GetEditionName() const {
			return EditionQuery::CastEditionToName(GetEdition());
		}

		inline boost::optional<boost::posix_time::ptime> GetLimitationTimeFrom() const {
			const std::auto_ptr<const LicenseKeyInfo> licenseInfo
				= GetLicenseInfo();
			return licenseInfo.get()
				?	licenseInfo->limitations.timeInterval.first
				:	boost::optional<boost::posix_time::ptime>();
		}

		inline boost::optional<boost::posix_time::ptime> GetLimitationTimeTo() const {
			const std::auto_ptr<const LicenseKeyInfo> licenseInfo
				= GetLicenseInfo();
			return licenseInfo.get()
				?	licenseInfo->limitations.timeInterval.second
				:	boost::optional<boost::posix_time::ptime>();
		}

		inline boost::optional<boost::posix_time::ptime> GetUpdateTimeFrom() const {
			const std::auto_ptr<const LicenseKeyInfo> licenseInfo
				= GetLicenseInfo();
			return licenseInfo.get()
				?	licenseInfo->update.first
				:	boost::optional<boost::posix_time::ptime>();
		}

		inline boost::optional<boost::posix_time::ptime> GetUpdateTimeTo() const {
			const std::auto_ptr<const LicenseKeyInfo> licenseInfo
				= GetLicenseInfo();
			return licenseInfo.get()
				?	licenseInfo->update.second
				:	boost::optional<boost::posix_time::ptime>();
		}

		inline bool IsTrial() const {
			const std::auto_ptr<const LicenseKeyInfo> licenseInfo
				= GetLicenseInfo();
			return licenseInfo.get()
				?	licenseInfo->isTrial
				:	false;
		}

		inline std::string GetLicense() const {
			const std::auto_ptr<const LicenseKeyInfo> licenseInfo
				= GetLicenseInfo();
			if (!licenseInfo.get()) {
				return string();
			}
			return licenseInfo->license;
		}

		inline std::string GetKey() const {
			const std::auto_ptr<const LicenseKeyInfo> licenseInfo
				= GetLicenseInfo();
			if (!licenseInfo.get()) {
				return string();
			}
			return licenseInfo->id;
		}

		inline static std::string GetLicense(
					const std::string &key,
					const boost::any &clientParam = boost::any()) {
			using namespace std;
			auto_ptr<const Options> options;
			auto_ptr<const LocalInfo> localInfo;
			auto_ptr<const LicenseKeyInfo> licenseInfo;
			auto_ptr<const FeatureInfo> featureInfo;
			const bool isExists = ReadKey(
				options,
				localInfo,
				licenseInfo,
				featureInfo,
				clientParam,
				&key);
			if (!isExists || !localInfo.get() || !licenseInfo.get() || !options.get()) {
				return string();
			}
			return licenseInfo->license;
		}

		inline void UpdateCache() {
			using namespace boost::posix_time;
			const ptime currentTime = second_clock::universal_time();
			if (	!m_cache.get()
					|| m_cache->checkCount > GetMaximumChecksCountFromCache()
					|| m_cache->since < (currentTime - minutes(30))
					|| m_cache->since > currentTime
					|| LocalStorage::IsLicenseKeyChanged(m_clientParam)) {
				ResetFeatureAvailabilityCache();
			}
		}

	private:

		inline static bool ReadKey(
					std::auto_ptr<const Options> &options,
					std::auto_ptr<const LocalInfo> &localInfo,
					std::auto_ptr<const LicenseKeyInfo> &licenseInfo,
					std::auto_ptr<const FeatureInfo> &featureInfo,
					const boost::any &clientParam,
					const std::string *clientKey = 0) {
			using namespace std;
			using namespace boost;
			using namespace Helpers::Xml;
			shared_ptr<const Document> doc;
			bool isExists;
			string key;

			if (clientKey) {
				key = *clientKey;
				isExists = true;
			} else {
				isExists = KeyRetrieve::Get(key, clientParam);
			}
			if (!isExists) {
				return false;
			}
#			if defined(_DEBUG) || defined(TEST)
			{
				filesystem::path dumpPath
					= Helpers::GetModuleFilePathA().branch_path();
				dumpPath /= "LicenseKey.xml";
				ofstream f(dumpPath.string().c_str(), ios::trunc | ios::binary);
				f << key;
			}
#			endif
			try {
				doc = Document::LoadFromString(key);
				options = OptionsQuery::Parse(*doc);
				if (options.get()) {
					localInfo = LocalInfoQuery::Get(*doc, *options, clientParam);
				}
				licenseInfo = LicenseKeyQuery::Parse(*doc);
				featureInfo = FeatureQuery::Parse(*doc);
			} catch (const TunnelEx::Helpers::Xml::Exception &) {
				//...//
			}
			return true;
		}

		inline std::auto_ptr<const LicenseKeyInfo> GetLicenseInfo() const {
			using namespace std;
			auto_ptr<const Options> options;
			auto_ptr<const LocalInfo> localInfo;
			auto_ptr<const LicenseKeyInfo> result;
			auto_ptr<const FeatureInfo> featureInfo;
			const bool isExists = ReadKey(
				options,
				localInfo,
				result,
				featureInfo,
				m_clientParam);
			if (!isExists || !localInfo.get() || !result.get() || !options.get()) {
				const_cast<License *>(this)->m_cache.reset(
					new Cache(
						false,
						!isExists ? UR_NOT_EXISTS : UR_FORMAT,
						Feature::GetDefaultValue()));
				return auto_ptr<const LicenseKeyInfo>();
			}
			return result;
		}

		inline bool IsFeatureValueAvailable(
					const Cache &cache,
					const FeatureValue &valueToCheck)
				const {
			return cache.isActive && cache.value
				?	Check::IsFeatureAvailable(*cache.value, valueToCheck)
				:	cache.defaultValue 
					?	Check::IsFeatureAvailable(*cache.defaultValue, valueToCheck)
					:	false;
		}

		template<class KeyInfo, class LocalInfo>
		inline std::auto_ptr<Cache> GetCacheWithError(
					const KeyInfo &keyInfo,
					const LocalInfo &localInfo,
					const FeatureValue &valueToCheck,
					const boost::optional<FeatureValue> &defaultValue)
				const {
			std::auto_ptr<Cache> result;
			if (!Check::CheckWorkstationPropetiesChanges(keyInfo, localInfo)) {
				result.reset(
					new Cache(
						false,
						valueToCheck,
						UR_WORSTATION,
						defaultValue));
			} else if (!Check::CheckScores(keyInfo, localInfo)) {
				result.reset(
					new Cache(
						false,
						valueToCheck,
						UR_SCORES,
						defaultValue));
			} else if (!Check::CheckTime(keyInfo, localInfo)) {
				result.reset(
					new Cache(
						false,
						valueToCheck,
						Check::GetCheckTimeFailReason(keyInfo, localInfo),
						defaultValue));
			} else {
				BOOST_ASSERT(false);
				result.reset(
					new Cache(
						false,
						valueToCheck,
						UR_FORMAT,
						defaultValue));
			}
			return result;
		}


	private:

		std::auto_ptr<Cache> m_cache;
		const boost::any m_clientParam;

	};

	//////////////////////////////////////////////////////////////////////////

} }

#endif // #ifndef INCLUDED_FILE__TUNNELEX__License_hpp__0908051157
