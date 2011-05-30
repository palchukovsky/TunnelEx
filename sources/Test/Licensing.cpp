/**************************************************************************
 *   Created: 2009/09/08 21:16
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "Licensing/RequestGenPolicies.hpp"
#include "Licensing/WinInetCommPolicy.hpp"
#include "Licensing/IpHelperWorkstationPropertiesQueryPolicy.hpp"
#include "Licensing/License.hpp"

#include "Core/String.hpp"

namespace tex = TunnelEx;
namespace pt = boost::posix_time;

//////////////////////////////////////////////////////////////////////////

namespace {

	//////////////////////////////////////////////////////////////////////////

	class LicenseKeyTestServer : private boost::noncopyable {

	public:

		typedef boost::function<bool(std::string &, const boost::any &)> XmlLicenseKeyRetrieverFunc;
		typedef boost::function<bool(tex::Licensing::WorkstationPropertyValues &, const boost::any &)>
			LocalWorkstationPropertyValuesGetterFunc;
		typedef boost::function<std::string(void)> EncryptedLicenseKeyRetrieverFunc;

	public:

		LicenseKeyTestServer() {
			//...//
		}

		explicit LicenseKeyTestServer(
					XmlLicenseKeyRetrieverFunc xmlLicenseKeyRetrieverFunc,
					LocalWorkstationPropertyValuesGetterFunc localWorkstationPropertyValuesGetterFunc,
					const char *const asymmetricPrivateKeyFileModif = "")
				: m_xmlLicenseKeyRetrieverFunc(xmlLicenseKeyRetrieverFunc),
				m_asymmetricPrivateKeyFileModif(asymmetricPrivateKeyFileModif),
				m_localWorkstationPropertyValuesGetterFunc(localWorkstationPropertyValuesGetterFunc) {
			//...//
		}

		explicit LicenseKeyTestServer(
					XmlLicenseKeyRetrieverFunc xmlLicenseKeyRetrieverFunc,
					LocalWorkstationPropertyValuesGetterFunc localWorkstationPropertyValuesGetterFunc,
					pt::ptime currentTime,
					const char *const asymmetricPrivateKeyFileModif = "")
				: m_xmlLicenseKeyRetrieverFunc(xmlLicenseKeyRetrieverFunc),
				m_localWorkstationPropertyValuesGetterFunc(localWorkstationPropertyValuesGetterFunc),
				m_asymmetricPrivateKeyFileModif(asymmetricPrivateKeyFileModif),
				m_currentTime(currentTime) {
			//...//
		}
		
		explicit LicenseKeyTestServer(
					const char *const licenseKeyFileModif,
					LocalWorkstationPropertyValuesGetterFunc localWorkstationPropertyValuesGetterFunc,
					const char *const asymmetricPrivateKeyFileModif = "")
				: m_localWorkstationPropertyValuesGetterFunc(localWorkstationPropertyValuesGetterFunc),
				m_licenseKeyFileModif(licenseKeyFileModif),
				m_asymmetricPrivateKeyFileModif(asymmetricPrivateKeyFileModif) {
			//...//
		}

		~LicenseKeyTestServer() {
			//...//
		}

	public:

		void SetCurrentTime(const pt::ptime &currentTime) {
			m_currentTime = currentTime;
		}

	public:

		XmlLicenseKeyRetrieverFunc GetXmlLicenseKeyRetriever() const {
			return m_xmlLicenseKeyRetrieverFunc;
		}

		LocalWorkstationPropertyValuesGetterFunc GetLocalWorkstationPropertyValuesGetter() const {
			return m_localWorkstationPropertyValuesGetterFunc;
		}
		
		std::string GetLicenseKeyFileModif() const {
			return m_licenseKeyFileModif;
		}
		
		std::string GetAsymmetricPrivateKeyFileModif() const {
			return m_asymmetricPrivateKeyFileModif;
		}

		pt::ptime GetCurrentTime() const {
			return m_currentTime;
		}
		
	private:

		XmlLicenseKeyRetrieverFunc m_xmlLicenseKeyRetrieverFunc;
		LocalWorkstationPropertyValuesGetterFunc m_localWorkstationPropertyValuesGetterFunc;
		const std::string m_licenseKeyFileModif;
		const std::string m_asymmetricPrivateKeyFileModif;
		pt::ptime m_currentTime;

	};
	
	std::auto_ptr<LicenseKeyTestServer> licenseKeyTestServer;

}

//////////////////////////////////////////////////////////////////////////

namespace TunnelEx { namespace Licensing {

	//////////////////////////////////////////////////////////////////////////
	
	template<class ClientTrait>
	struct LocalStoragePolicy<ClientTrait, true> {

		typedef LocalStoragePolicy<ClientTrait, false>
			Original;
		
		static void GetLicenseServerAsymmetricPublicKey(
					std::vector<unsigned char> &result) {
			Original::GetLicenseServerAsymmetricPublicKey(result);
		}
		
		static std::string GetLocalAsymmetricPrivateKey(const boost::any &) {
			std::ostringstream oss;
			oss
				<< "Resource\\LocalAsymmetricPrivateKey"
				<< licenseKeyTestServer->GetAsymmetricPrivateKeyFileModif()
				<< ".pem";
			std::ifstream f(oss.str().c_str());
			assert(f);
			f.unsetf(std::ios::skipws);
			return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
		}
		
		static std::string GetLicenseKey(const boost::any &) {
			std::ostringstream oss;
			oss << "Resource\\LicenseKey" << licenseKeyTestServer->GetLicenseKeyFileModif() << ".key";
			std::ifstream f(oss.str().c_str());
			assert(f);
			f.unsetf(std::ios::skipws);
			return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
		}
		
		static void StoreLocalAsymmetricPrivateKey(const std::string &) {
			//...//
		}
	
		static bool IsLicenseKeyChanged(const boost::any &) {
			return false;
		}

		static void ResetLicenseKeyUpdateState(const boost::any &) {
			//...//
		}

	};

	template<class ClientTrait>
	struct LocalStoragePolicy<ClientTrait, false> {

		typedef LocalStoragePolicy<ClientTrait, false>
			Original;

		static void GetLicenseServerAsymmetricPublicKey(
					std::vector<unsigned char> &result) {
			Original::GetLicenseServerAsymmetricPublicKey(result);
		}

		static std::string GetLocalAsymmetricPrivateKey(const boost::any &) {
			return std::string();
		}

		static std::string GetLicenseKey(const boost::any &) {
			return std::string();
		}

		static void StoreLocalAsymmetricPrivateKey(const std::string &) {
			//...//
		}

	};

	//////////////////////////////////////////////////////////////////////////

	template<class ClientTrait>
	struct XmlLicenseKeyRetrievePolicy<ClientTrait, true> {
	
		typedef XmlLicenseKeyRetrievePolicy<ClientTrait, false>
			Original;

		static std::string Import(const std::string &lKey, const std::string &pKey) {
			return Original::Import(lKey, pKey);
		}

		static bool Get(std::string &key, const boost::any &clientParam) {
			return licenseKeyTestServer->GetXmlLicenseKeyRetriever()
				?	licenseKeyTestServer->GetXmlLicenseKeyRetriever()(key, clientParam)
				:	Original::Get(key, clientParam);
		}

	};

	//////////////////////////////////////////////////////////////////////////

	template<class ClientTrait>
	struct WorkstationPropertiesLocalPolicy<ClientTrait, true> {

		typedef WorkstationPropertiesLocalPolicy<ClientTrait, false>
			Original;

		static bool Get(WorkstationPropertyValues &result, const boost::any &clientParam) {
			return licenseKeyTestServer->GetLocalWorkstationPropertyValuesGetter()
				?	licenseKeyTestServer
						->GetLocalWorkstationPropertyValuesGetter()(boost::ref(result), clientParam)
				:	Original::Get(result, clientParam);
		}

	};

	//////////////////////////////////////////////////////////////////////////

	template<typename ClientTrait>
	struct LocalInfoQueryPolicy<ClientTrait, true> {

		typedef LocalInfoQueryPolicy<ClientTrait, false> Original;

		inline static std::auto_ptr<const LocalInfo> Get(
					const tex::Helpers::Xml::Document &doc,
					const Options &options,
					const boost::any &clientParam) {
			std::auto_ptr<const LocalInfo> result = Original::Get(doc, options, clientParam);
			if (	result.get()
					&& !licenseKeyTestServer->GetCurrentTime().is_not_a_date_time()) {
				const_cast<LocalInfo &>(*result).time
					= licenseKeyTestServer->GetCurrentTime();
			}
			return result;
		}

	};

	//////////////////////////////////////////////////////////////////////////

} }

//////////////////////////////////////////////////////////////////////////

namespace {

	//////////////////////////////////////////////////////////////////////////
	
	std::string LoadEncryptedLicenseKey(const char *licenseKeyFileModif) {
		std::ostringstream oss;
		oss << "Resource\\LicenseKey" << licenseKeyFileModif << ".key";
		std::ifstream f(oss.str().c_str());
		f.unsetf(std::ios::skipws);
		return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
	}

	//////////////////////////////////////////////////////////////////////////

	bool GetValidLicenseKeyXml(std::string &result, const boost::any &) {
		std::ifstream f("Resource\\LicenseKeyValid.xml");
		assert(f);
		f.unsetf(std::ios::skipws);
		std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()).swap(result);
		return true;
	}

	bool GetEmptyLicenseKeyXml(std::string &result, const boost::any &) {
		result.clear();
		return false;
	}

	//////////////////////////////////////////////////////////////////////////

	bool GetInvalidLicenseKeyXml(std::string &result, const boost::any &userParam) {
		const bool isExists = GetValidLicenseKeyXml(result, userParam);
		result.resize(result.size() - (result.size() / 4));
		return isExists;
	}

	//////////////////////////////////////////////////////////////////////////

	bool GetValidLocalWorkstationProperties(
				tex::Licensing::WorkstationPropertyValues &result,
				const boost::any &) {
		result[tex::Licensing::WORKSTATION_PROPERTY_OS_VER]
			= "34D45871D8C3D98F00B204E9800998ECF8427389";
		result[tex::Licensing::WORKSTATION_PROPERTY_INSTALLATION_INFO]
			= "41D8CD98F00B204E9800998ECF8427E43F35A151";
		result[tex::Licensing::WORKSTATION_PROPERTY_ADAPTER]
			= "376489FD0CD97F005204E9840998EC183F13167D89867FD0CD97F00D5204E958840998AEC1893F1301D97F005204E98401FD1CD97F001201E9840996";
		result[tex::Licensing::WORKSTATION_PROPERTY_OS_VOLUME]
			= "10FD0CD9137F001204E495FA84D40998EC183765";
		return true;
	}
	
	bool GetLocalWorkstationPropertiesWithInvalidOsVer(
				tex::Licensing::WorkstationPropertyValues &result,
				const boost::any &userParam) {
		tex::Licensing::WorkstationPropertyValues resultTmp;
		if (!GetValidLocalWorkstationProperties(resultTmp, userParam)) {
			return false;
		}
		resultTmp[tex::Licensing::WORKSTATION_PROPERTY_OS_VER]
			= "34D45872D8C3D98F00B204E9800998ECF8427389";
		resultTmp.swap(result);
		return true;
	}

	bool GetLocalWorkstationPropertiesWithInvalidOsSerial(
				tex::Licensing::WorkstationPropertyValues &result,
				const boost::any &userParam) {
		tex::Licensing::WorkstationPropertyValues resultTmp;
		if (!GetValidLocalWorkstationProperties(resultTmp, userParam)) {
			return false;
		}
		resultTmp[tex::Licensing::WORKSTATION_PROPERTY_INSTALLATION_INFO]
			= "41D8CD91F00B204E9800998ECF8427E43F35A151";
		resultTmp.swap(result);
		return true;
	}

	bool GetLocalWorkstationPropertiesWithInvalidAdapter(
				tex::Licensing::WorkstationPropertyValues &result,
				const boost::any &userParam) {
		tex::Licensing::WorkstationPropertyValues resultTmp;
		if (!GetValidLocalWorkstationProperties(resultTmp, userParam)) {
			return false;
		}
		resultTmp[tex::Licensing::WORKSTATION_PROPERTY_ADAPTER]
			= "376489FD0CD97F005204E9840998EC183F13167D";
		resultTmp.swap(result);
		return true;
	}

	bool GetLocalWorkstationPropertiesWithInvalidOsVolume(
				tex::Licensing::WorkstationPropertyValues &result,
				const boost::any &userParam) {
		tex::Licensing::WorkstationPropertyValues resultTmp;
		if (!GetValidLocalWorkstationProperties(resultTmp, userParam)) {
			return false;
		}
		resultTmp[tex::Licensing::WORKSTATION_PROPERTY_OS_VOLUME]
			= "10FD0CD9137F001204E495FA81D40998EC183765";
		resultTmp.swap(result);
		return true;
	}

	//////////////////////////////////////////////////////////////////////////

}

//////////////////////////////////////////////////////////////////////////

namespace {

	TEST(Licensing, NotExistsLicenseKey) {

		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				&GetEmptyLicenseKeyXml,
				&GetValidLocalWorkstationProperties));

		{
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
			EXPECT_FALSE(license.IsFeatureValueAvailable(true));
			EXPECT_FALSE(license.IsFeatureValueAvailable(false));
			EXPECT_TRUE(license.GetOwner().empty());
			EXPECT_TRUE(license.GetLicense().empty());
			EXPECT_EQ(
				license.GetEditionName(),
				License::EditionQuery::CastEditionToName(tex::Licensing::EDITION_STANDARD));
			EXPECT_EQ(license.GetUnactivityReason(), tex::Licensing::UR_NOT_EXISTS);
		}

		{
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(10));
			EXPECT_FALSE(license.IsFeatureAvailable(11));
		}

		{
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(5));
			EXPECT_FALSE(license.IsFeatureAvailable(5 + 1));
		}

		{
			typedef tex::Licensing::ProxyLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
		}

		{
			typedef tex::Licensing::EndpointIoSeparationLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
		}

	}

	TEST(Licensing, ValidLicenseKey) {

		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				&GetValidLicenseKeyXml,
				&GetValidLocalWorkstationProperties));

		{
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			EXPECT_TRUE(license.IsFeatureAvailable(true));
			EXPECT_TRUE(license.IsFeatureValueAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
			EXPECT_FALSE(license.IsFeatureValueAvailable(false));
			EXPECT_EQ(tex::Licensing::UR_NO, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			ASSERT_TRUE(license.GetFeatureValue(featureValue));
			EXPECT_TRUE(featureValue);
			EXPECT_EQ("Join Smith and Co", license.GetOwner());
			EXPECT_EQ(tex::Licensing::UR_NO, license.GetUnactivityReason());
			EXPECT_EQ("Professional", license.GetEditionName());
			EXPECT_EQ(tex::Licensing::UR_NO, license.GetUnactivityReason());
			EXPECT_EQ("056F070E-2374-4A12-9DC6-9AAB0DB309E7", license.GetLicense());
			EXPECT_EQ(tex::Licensing::UR_NO, license.GetUnactivityReason());
			EXPECT_EQ(pt::time_from_string("2009-08-03 12:08:00"), license.GetLimitationTimeFrom());
			EXPECT_EQ(pt::time_from_string("2100-01-01 00:00:02"), license.GetLimitationTimeTo());
			EXPECT_EQ(pt::time_from_string("2009-08-01 12:08:00"), license.GetUpdateTimeFrom());
			EXPECT_EQ(pt::time_from_string("2100-01-03 00:00:00"), license.GetUpdateTimeTo());
		}
		{
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			EXPECT_TRUE(license.IsFeatureAvailable(30));
			EXPECT_TRUE(license.IsFeatureValueAvailable(30));
			EXPECT_FALSE(license.IsFeatureAvailable(30 + 1));
			EXPECT_FALSE(license.IsFeatureValueAvailable(30 + 1));
			EXPECT_TRUE(license.GetUnactivityReason() == tex::Licensing::UR_NO);
			License::FeatureValue featureValue;
			ASSERT_TRUE(license.GetFeatureValue(featureValue));
			ASSERT_EQ(30, featureValue);
		}
		{
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			EXPECT_TRUE(license.IsFeatureAvailable(40));
			EXPECT_TRUE(license.IsFeatureValueAvailable(40));
			EXPECT_FALSE(license.IsFeatureAvailable(40 + 1));
			EXPECT_FALSE(license.IsFeatureValueAvailable(40 + 1));
			EXPECT_EQ(tex::Licensing::UR_NO, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			ASSERT_TRUE(license.GetFeatureValue(featureValue));
			EXPECT_EQ(40, featureValue);
		}
		{
			typedef tex::Licensing::ProxyLicenseTesting License;
			License license;
			EXPECT_TRUE(license.IsFeatureAvailable(true));
			EXPECT_TRUE(license.IsFeatureValueAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
			EXPECT_FALSE(license.IsFeatureValueAvailable(false));
			EXPECT_EQ(tex::Licensing::UR_NO, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			ASSERT_TRUE(license.GetFeatureValue(featureValue));
			EXPECT_TRUE(featureValue);
		}
	}

	TEST(Licensing, InvalidWorkstationPropoertiesForLicenseKey) {

		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				&GetValidLicenseKeyXml,
				&GetLocalWorkstationPropertiesWithInvalidOsVolume));

		{
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(true));
			EXPECT_FALSE(license.IsFeatureValueAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
			EXPECT_FALSE(license.IsFeatureValueAvailable(false));
			EXPECT_EQ(tex::Licensing::UR_WORSTATION, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(10));
			EXPECT_FALSE(license.IsFeatureAvailable(11));
			EXPECT_EQ(tex::Licensing::UR_WORSTATION, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(5));
			EXPECT_FALSE(license.IsFeatureValueAvailable(5));
			EXPECT_FALSE(license.IsFeatureAvailable(6));
			EXPECT_FALSE(license.IsFeatureValueAvailable(6));
			EXPECT_EQ(tex::Licensing::UR_WORSTATION, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::ProxyLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(true));
			EXPECT_FALSE(license.IsFeatureValueAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
			EXPECT_FALSE(license.IsFeatureValueAvailable(false));
			EXPECT_EQ(tex::Licensing::UR_WORSTATION, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}

	}

	TEST(Licensing, InvalidWorkstationPropoertiesForFeature) {

		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				&GetValidLicenseKeyXml,
				&GetLocalWorkstationPropertiesWithInvalidAdapter));

		{
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			EXPECT_TRUE(license.IsFeatureAvailable(true));
			EXPECT_TRUE(license.IsFeatureValueAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
			EXPECT_FALSE(license.IsFeatureValueAvailable(false));
			EXPECT_EQ(tex::Licensing::UR_NO, license.GetUnactivityReason());
		}
		{
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(10));
			EXPECT_FALSE(license.IsFeatureValueAvailable(10));
			EXPECT_FALSE(license.IsFeatureAvailable(10 + 1));
			EXPECT_FALSE(license.IsFeatureValueAvailable(10 + 1));
			EXPECT_EQ(tex::Licensing::UR_WORSTATION, license.GetUnactivityReason());
		}
		{
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			EXPECT_TRUE(license.IsFeatureAvailable(40));
			EXPECT_TRUE(license.IsFeatureValueAvailable(40));
			EXPECT_FALSE(license.IsFeatureAvailable(40 + 1));
			EXPECT_FALSE(license.IsFeatureValueAvailable(40 + 1));
			EXPECT_EQ(tex::Licensing::UR_NO, license.GetUnactivityReason());
		}
		{
			typedef tex::Licensing::ProxyLicenseTesting License;
			License license;
			EXPECT_TRUE(license.IsFeatureAvailable(true));
			EXPECT_TRUE(license.IsFeatureValueAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
			EXPECT_FALSE(license.IsFeatureValueAvailable(false));
			EXPECT_EQ(tex::Licensing::UR_NO, license.GetUnactivityReason());
		}

	}

	TEST(Licensing, TooManyScoresForLicenseKey) {

		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				&GetValidLicenseKeyXml,
				&GetLocalWorkstationPropertiesWithInvalidOsVer));

		{
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(true));
			EXPECT_FALSE(license.IsFeatureValueAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
			EXPECT_FALSE(license.IsFeatureValueAvailable(false));
			EXPECT_EQ(tex::Licensing::UR_SCORES, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(10));
			EXPECT_FALSE(license.IsFeatureValueAvailable(10));
			EXPECT_FALSE(license.IsFeatureAvailable(10 + 1));
			EXPECT_FALSE(license.IsFeatureValueAvailable(10 + 1));
			EXPECT_EQ(tex::Licensing::UR_SCORES, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(5));
			EXPECT_FALSE(license.IsFeatureValueAvailable(5));
			EXPECT_FALSE(license.IsFeatureAvailable(5 + 1));
			EXPECT_FALSE(license.IsFeatureValueAvailable(5 + 1));
			EXPECT_EQ(tex::Licensing::UR_SCORES, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::ProxyLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(true));
			EXPECT_FALSE(license.IsFeatureValueAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
			EXPECT_FALSE(license.IsFeatureValueAvailable(false));
			EXPECT_EQ(tex::Licensing::UR_SCORES, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}

	}

	TEST(Licensing, TooManyScoresForFeature) {

		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				&GetValidLicenseKeyXml,
				&GetLocalWorkstationPropertiesWithInvalidOsSerial));

		{
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			EXPECT_TRUE(license.IsFeatureAvailable(true));
			EXPECT_TRUE(license.IsFeatureValueAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
			EXPECT_FALSE(license.IsFeatureValueAvailable(false));
			EXPECT_EQ(tex::Licensing::UR_NO, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			ASSERT_TRUE(license.GetFeatureValue(featureValue));
			EXPECT_TRUE(featureValue);
		}
		{
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			EXPECT_TRUE(license.IsFeatureAvailable(30));
			EXPECT_TRUE(license.IsFeatureValueAvailable(30));
			EXPECT_FALSE(license.IsFeatureAvailable(30 + 1));
			EXPECT_FALSE(license.IsFeatureValueAvailable(30 + 1));
			EXPECT_EQ(tex::Licensing::UR_NO, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			ASSERT_TRUE(license.GetFeatureValue(featureValue));
			EXPECT_EQ(30, featureValue);
		}
		{
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(5));
			EXPECT_FALSE(license.IsFeatureValueAvailable(5));
			EXPECT_FALSE(license.IsFeatureAvailable(5 + 1));
			EXPECT_FALSE(license.IsFeatureValueAvailable(5 + 1));
			EXPECT_EQ(tex::Licensing::UR_SCORES, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::ProxyLicenseTesting License;
			License license;
			EXPECT_TRUE(license.IsFeatureAvailable(true));
			EXPECT_TRUE(license.IsFeatureValueAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
			EXPECT_FALSE(license.IsFeatureValueAvailable(false));
			EXPECT_EQ(tex::Licensing::UR_NO, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			ASSERT_TRUE(license.GetFeatureValue(featureValue));
			EXPECT_TRUE(featureValue);
		}

	}

	TEST(Licensing, HalfScoresForLicenseKey) {
		// Checking in TooManyScoresForFeature test
	}

	TEST(Licensing, HalfScoresForFeature) {
		// Checking in TooManyScoresForFeature test
		// (for such features as "executing" and "tunnels count").
	}

	TEST(Licensing, LicenseCacheResetsByCheckCount) {
		
		struct KeyRequestProxy {
			KeyRequestProxy()
					: requestsCount(0) {
				//...//
			}
			//! workaround for VS compiler warning C4180
			bool GetKey(std::string &result, const boost::any &clientParam) {
				return const_cast<const KeyRequestProxy *>(this)
					->GetKey(result, clientParam);
			}
			bool GetKey(std::string &result, const boost::any &clientParam) const {
				++requestsCount;
				return GetValidLicenseKeyXml(result, clientParam);
			}
			mutable size_t requestsCount;
		} keyRequestProxy;

		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				boost::bind(&KeyRequestProxy::GetKey, &keyRequestProxy, _1, _2),
				&GetValidLocalWorkstationProperties));

		{
			keyRequestProxy.requestsCount = 0;
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			EXPECT_TRUE(license.IsFeatureAvailable(true));
			EXPECT_TRUE(license.IsFeatureValueAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
			EXPECT_FALSE(license.IsFeatureValueAvailable(false));
			EXPECT_EQ(tex::Licensing::UR_NO, license.GetUnactivityReason());
			EXPECT_EQ(1, keyRequestProxy.requestsCount);
		}
		{
			keyRequestProxy.requestsCount = 0;
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			EXPECT_TRUE(license.IsFeatureAvailable(30));
			EXPECT_TRUE(license.IsFeatureValueAvailable(30));
			license.ResetFeatureAvailabilityCache();
			EXPECT_FALSE(license.IsFeatureAvailable(30 + 1));
			EXPECT_FALSE(license.IsFeatureValueAvailable(30 + 1));
			EXPECT_EQ(tex::Licensing::UR_NO, license.GetUnactivityReason());
			EXPECT_EQ(2, keyRequestProxy.requestsCount);
		}
		{
			keyRequestProxy.requestsCount = 0;
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			for (size_t i = 1; i < license.GetMaximumChecksCountFromCache() + 1; ++i) {
				EXPECT_TRUE(license.IsFeatureAvailable(4));
				ASSERT_TRUE(
					(i <= license.GetMaximumChecksCountFromCache()
						&&  keyRequestProxy.requestsCount == 1)
					|| (i > license.GetMaximumChecksCountFromCache()
						&&  keyRequestProxy.requestsCount == 2));
			}
		}

	}

	TEST(Licensing, NoStartedTimeForLicenseKey) {

		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				&GetValidLicenseKeyXml,
				&GetValidLocalWorkstationProperties,
				pt::time_from_string("2009-08-02 12:07:59")));

		{
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(true));
			EXPECT_FALSE(license.IsFeatureValueAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
			EXPECT_FALSE(license.IsFeatureValueAvailable(false));
			EXPECT_EQ(tex::Licensing::UR_TIME, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(10));
			EXPECT_FALSE(license.IsFeatureValueAvailable(10));
			EXPECT_FALSE(license.IsFeatureAvailable(10 + 1));
			EXPECT_FALSE(license.IsFeatureValueAvailable(10 + 1));
			EXPECT_EQ(tex::Licensing::UR_TIME, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(5));
			EXPECT_FALSE(license.IsFeatureValueAvailable(5));
			EXPECT_FALSE(license.IsFeatureAvailable(5 + 1));
			EXPECT_FALSE(license.IsFeatureValueAvailable(5 + 1));
			EXPECT_EQ(tex::Licensing::UR_TIME, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::ProxyLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(true));
			EXPECT_FALSE(license.IsFeatureValueAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
			EXPECT_FALSE(license.IsFeatureValueAvailable(false));
			EXPECT_EQ(tex::Licensing::UR_TIME, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}

	}

	TEST(Licensing, NoStartedTimeForLicenseKeyUpdate) {

		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				&GetValidLicenseKeyXml,
				&GetValidLocalWorkstationProperties,
				pt::time_from_string("2009-08-01 00:01:59")));

		{
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(true));
			EXPECT_FALSE(license.IsFeatureValueAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
			EXPECT_FALSE(license.IsFeatureValueAvailable(false));
			EXPECT_EQ(tex::Licensing::UR_UPDATE, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(10));
			EXPECT_FALSE(license.IsFeatureValueAvailable(10));
			EXPECT_FALSE(license.IsFeatureAvailable(10 + 1));
			EXPECT_FALSE(license.IsFeatureValueAvailable(10 + 1));
			EXPECT_EQ(tex::Licensing::UR_UPDATE, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(5));
			EXPECT_FALSE(license.IsFeatureValueAvailable(5));
			EXPECT_FALSE(license.IsFeatureAvailable(5 + 1));
			EXPECT_FALSE(license.IsFeatureValueAvailable(5 + 1));
			EXPECT_EQ(tex::Licensing::UR_UPDATE, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::ProxyLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(true));
			EXPECT_FALSE(license.IsFeatureValueAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
			EXPECT_FALSE(license.IsFeatureValueAvailable(false));
			EXPECT_EQ(tex::Licensing::UR_UPDATE, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}

	}

	TEST(Licensing, ExpiredTimeForLicenseKey) {

		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				&GetValidLicenseKeyXml,
				&GetValidLocalWorkstationProperties,
				pt::time_from_string("2100-01-02 00:00:01")));

		{
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(true));
			EXPECT_FALSE(license.IsFeatureValueAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
			EXPECT_FALSE(license.IsFeatureValueAvailable(false));
			EXPECT_EQ(tex::Licensing::UR_TIME, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(10));
			EXPECT_FALSE(license.IsFeatureValueAvailable(10));
			EXPECT_FALSE(license.IsFeatureAvailable(10 + 1));
			EXPECT_FALSE(license.IsFeatureValueAvailable(10 + 1));
			EXPECT_EQ(tex::Licensing::UR_TIME, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(5));
			EXPECT_FALSE(license.IsFeatureValueAvailable(5));
			EXPECT_FALSE(license.IsFeatureAvailable(5 + 1));
			EXPECT_FALSE(license.IsFeatureValueAvailable(5 + 1));
			EXPECT_EQ(tex::Licensing::UR_TIME, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::ProxyLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(true));
			EXPECT_FALSE(license.IsFeatureValueAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
			EXPECT_FALSE(license.IsFeatureValueAvailable(false));
			EXPECT_EQ(tex::Licensing::UR_TIME, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}

	}

	TEST(Licensing, ExpiredTimeForLicenseKeyUpdate) {

		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				&GetValidLicenseKeyXml,
				&GetValidLocalWorkstationProperties,
				pt::time_from_string("2100-01-04 00:00:03")));

		{
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(true));
			EXPECT_FALSE(license.IsFeatureValueAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
			EXPECT_FALSE(license.IsFeatureValueAvailable(false));
			EXPECT_EQ(tex::Licensing::UR_UPDATE, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(10));
			EXPECT_FALSE(license.IsFeatureValueAvailable(10));
			EXPECT_FALSE(license.IsFeatureAvailable(10 + 1));
			EXPECT_FALSE(license.IsFeatureValueAvailable(10 + 1));
			EXPECT_EQ(tex::Licensing::UR_UPDATE, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(5));
			EXPECT_FALSE(license.IsFeatureValueAvailable(5));
			EXPECT_FALSE(license.IsFeatureAvailable(5 + 1));
			EXPECT_FALSE(license.IsFeatureValueAvailable(5 + 1));
			EXPECT_EQ(tex::Licensing::UR_UPDATE, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::ProxyLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(true));
			EXPECT_FALSE(license.IsFeatureValueAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
			EXPECT_FALSE(license.IsFeatureValueAvailable(false));
			EXPECT_EQ(tex::Licensing::UR_UPDATE, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}

	}

	TEST(Licensing, NotStartedTimeForFeature) {

		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				&GetValidLicenseKeyXml,
				&GetValidLocalWorkstationProperties,
				pt::time_from_string("2009-09-01 12:07:59")));

		{
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			EXPECT_TRUE(license.IsFeatureAvailable(true));
			EXPECT_TRUE(license.IsFeatureValueAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
			EXPECT_FALSE(license.IsFeatureValueAvailable(false));
			EXPECT_EQ(tex::Licensing::UR_NO, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			ASSERT_TRUE(license.GetFeatureValue(featureValue));
			EXPECT_TRUE(featureValue);
		}
		{
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			EXPECT_TRUE(license.IsFeatureAvailable(30));
			EXPECT_TRUE(license.IsFeatureValueAvailable(30));
			EXPECT_FALSE(license.IsFeatureAvailable(30 + 1));
			EXPECT_FALSE(license.IsFeatureValueAvailable(30 + 1));
			EXPECT_EQ(tex::Licensing::UR_NO, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			ASSERT_TRUE(license.GetFeatureValue(featureValue));
			EXPECT_EQ(30, featureValue);
		}
		{
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			EXPECT_TRUE(license.IsFeatureAvailable(40));
			EXPECT_TRUE(license.IsFeatureValueAvailable(40));
			EXPECT_FALSE(license.IsFeatureAvailable(40 + 1));
			EXPECT_FALSE(license.IsFeatureValueAvailable(40 + 1));
			EXPECT_EQ(tex::Licensing::UR_NO, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			ASSERT_TRUE(license.GetFeatureValue(featureValue));
			EXPECT_EQ(40, featureValue);
		}
		{
			typedef tex::Licensing::ProxyLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(true));
			EXPECT_FALSE(license.IsFeatureValueAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
			EXPECT_FALSE(license.IsFeatureValueAvailable(false));
			EXPECT_EQ(tex::Licensing::UR_TIME, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}

	}

	TEST(Licensing, ExpiredTimeForFeature) {

		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				&GetValidLicenseKeyXml,
				&GetValidLocalWorkstationProperties,
				pt::time_from_string("2100-01-01 00:00:01")));

		{
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			EXPECT_TRUE(license.IsFeatureAvailable(true));
			EXPECT_TRUE(license.IsFeatureValueAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
			EXPECT_FALSE(license.IsFeatureValueAvailable(false));
			EXPECT_EQ(tex::Licensing::UR_NO, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			ASSERT_TRUE(license.GetFeatureValue(featureValue));
			EXPECT_TRUE(featureValue);
		}
		{
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			EXPECT_TRUE(license.IsFeatureAvailable(30));
			EXPECT_TRUE(license.IsFeatureValueAvailable(30));
			EXPECT_FALSE(license.IsFeatureAvailable(30 + 1));
			EXPECT_FALSE(license.IsFeatureValueAvailable(30 + 1));
			EXPECT_EQ(tex::Licensing::UR_NO, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			ASSERT_TRUE(license.GetFeatureValue(featureValue));
			EXPECT_EQ(30, featureValue);
		}
		{
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			EXPECT_TRUE(license.IsFeatureAvailable(40));
			EXPECT_TRUE(license.IsFeatureValueAvailable(40));
			EXPECT_FALSE(license.IsFeatureAvailable(40 + 1));
			EXPECT_FALSE(license.IsFeatureValueAvailable(40 + 1));
			EXPECT_EQ(tex::Licensing::UR_NO, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			ASSERT_TRUE(license.GetFeatureValue(featureValue));
			EXPECT_EQ(40, featureValue);
		}
		{
			typedef tex::Licensing::ProxyLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(true));
			EXPECT_FALSE(license.IsFeatureValueAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
			EXPECT_FALSE(license.IsFeatureValueAvailable(false));
			EXPECT_EQ(tex::Licensing::UR_TIME, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}

	}

	TEST(Licensing, InvalidLicenseKeyXml) {
		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				&GetInvalidLicenseKeyXml,
				&GetValidLocalWorkstationProperties));
		{
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
			EXPECT_EQ(tex::Licensing::UR_FORMAT, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(10));
			EXPECT_FALSE(license.IsFeatureValueAvailable(10));
			EXPECT_FALSE(license.IsFeatureAvailable(10 + 1));
			EXPECT_FALSE(license.IsFeatureValueAvailable(10 + 1));
			EXPECT_EQ(tex::Licensing::UR_FORMAT, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(5));
			EXPECT_FALSE(license.IsFeatureValueAvailable(5));
			EXPECT_FALSE(license.IsFeatureAvailable(5 + 1));
			EXPECT_FALSE(license.IsFeatureValueAvailable(5 + 1));
			EXPECT_EQ(tex::Licensing::UR_FORMAT, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::ProxyLicenseTesting License;
			License license;
			EXPECT_FALSE(license.IsFeatureAvailable(true));
			EXPECT_FALSE(license.IsFeatureValueAvailable(true));
			EXPECT_FALSE(license.IsFeatureAvailable(false));
			EXPECT_FALSE(license.IsFeatureValueAvailable(false));
			EXPECT_EQ(tex::Licensing::UR_FORMAT, license.GetUnactivityReason());
			License::FeatureValue featureValue;
			EXPECT_FALSE(license.GetFeatureValue(featureValue));
		}
	}
	
	TEST(Licensing, WorstationInfo) {
		tex::Licensing::WorkstationPropertyValues propValues;
		tex::Licensing::ExeLicense::WorkstationPropertiesLocal::Get(propValues, boost::any());
		const size_t hashSize = 40;
		ASSERT_TRUE(propValues.find(tex::Licensing::WORKSTATION_PROPERTY_OS_VER) != propValues.end());
		EXPECT_TRUE(propValues[tex::Licensing::WORKSTATION_PROPERTY_OS_VER].size() == hashSize);
		ASSERT_TRUE(propValues.find(tex::Licensing::WORKSTATION_PROPERTY_INSTALLATION_INFO) != propValues.end());
		EXPECT_TRUE(propValues[tex::Licensing::WORKSTATION_PROPERTY_INSTALLATION_INFO].size() == hashSize);
		ASSERT_TRUE(propValues.find(tex::Licensing::WORKSTATION_PROPERTY_ADAPTER) != propValues.end());
		EXPECT_TRUE(propValues[tex::Licensing::WORKSTATION_PROPERTY_ADAPTER].size() >= hashSize);
		ASSERT_TRUE(propValues.find(tex::Licensing::WORKSTATION_PROPERTY_OS_VOLUME) != propValues.end());
		EXPECT_TRUE(propValues[tex::Licensing::WORKSTATION_PROPERTY_OS_VOLUME].size() == hashSize);
	}
	
	/* TEST(Licensing, KeyRequest) {
		licenseKeyTestServer.reset(new LicenseKeyTestServer);
		tex::Licensing::OnlineKeyRequestTesting request(
			"3BA8C77D-4EDE-4622-C666-AA8EE4C46AB3");
		request.Send();
		EXPECT_TRUE(request.TestKey<tex::Licensing::ExeLicense>());
	} */

}
