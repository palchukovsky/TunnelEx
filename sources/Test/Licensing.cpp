/**************************************************************************
 *   Created: 2009/09/08 21:16
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Licensing.cpp 1072 2010-11-25 20:02:26Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "Licensing/RequestGenPolicies.hpp"
#include "Licensing/WinInetCommPolicy.hpp"

#include <TunnelEx/String.hpp>

namespace ut = boost::unit_test;
namespace tex = TunnelEx;
using namespace std;
using namespace boost;
using namespace boost::posix_time;

//////////////////////////////////////////////////////////////////////////

namespace Test {

	//////////////////////////////////////////////////////////////////////////

	class LicenseKeyTestServer : private boost::noncopyable {

	public:

		typedef function<bool(string &, const any &)> XmlLicenseKeyRetrieverFunc;
		typedef function<bool(tex::Licensing::WorkstationPropertyValues &, const any &)>
			LocalWorkstationPropertyValuesGetterFunc;
		typedef function<std::string(void)> EncryptedLicenseKeyRetrieverFunc;

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
					ptime currentTime,
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

		void SetCurrentTime(const ptime &currentTime) {
			m_currentTime = currentTime;
		}

	public:

		XmlLicenseKeyRetrieverFunc GetXmlLicenseKeyRetriever() const {
			return m_xmlLicenseKeyRetrieverFunc;
		}

		LocalWorkstationPropertyValuesGetterFunc GetLocalWorkstationPropertyValuesGetter() const {
			return m_localWorkstationPropertyValuesGetterFunc;
		}
		
		string GetLicenseKeyFileModif() const {
			return m_licenseKeyFileModif;
		}
		
		string GetAsymmetricPrivateKeyFileModif() const {
			return m_asymmetricPrivateKeyFileModif;
		}

		ptime GetCurrentTime() const {
			return m_currentTime;
		}
		
	private:

		XmlLicenseKeyRetrieverFunc m_xmlLicenseKeyRetrieverFunc;
		LocalWorkstationPropertyValuesGetterFunc m_localWorkstationPropertyValuesGetterFunc;
		const string m_licenseKeyFileModif;
		const string m_asymmetricPrivateKeyFileModif;
		ptime m_currentTime;

	};
	
	auto_ptr<LicenseKeyTestServer> licenseKeyTestServer;

}

//////////////////////////////////////////////////////////////////////////

namespace TunnelEx { namespace Licensing {

	//////////////////////////////////////////////////////////////////////////
	
	template<class ClientTrait>
	struct LocalStoragePolicy<ClientTrait, true> {

		typedef LocalStoragePolicy<ClientTrait, false>
			Original;
		
		static void GetLicenseServerAsymmetricPublicKey(
					vector<unsigned char> &result) {
			Original::GetLicenseServerAsymmetricPublicKey(result);
		}
		
		static std::string GetLocalAsymmetricPrivateKey(const any &) {
			ostringstream oss;
			oss
				<< "Resource\\LocalAsymmetricPrivateKey"
				<< Test::licenseKeyTestServer->GetAsymmetricPrivateKeyFileModif()
				<< ".pem";
			ifstream f(oss.str().c_str());
			BOOST_ASSERT(f);
			f.unsetf(ios_base::skipws);
			return string(istreambuf_iterator<char>(f), istreambuf_iterator<char>());
		}
		
		static string GetLicenseKey(const any &) {
			ostringstream oss;
			oss << "Resource\\LicenseKey" << Test::licenseKeyTestServer->GetLicenseKeyFileModif() << ".key";
			ifstream f(oss.str().c_str());
			BOOST_ASSERT(f);
			f.unsetf(ios_base::skipws);
			return string(istreambuf_iterator<char>(f), istreambuf_iterator<char>());
		}
		
		static void StoreLocalAsymmetricPrivateKey(const string &) {
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
					vector<unsigned char> &result) {
			Original::GetLicenseServerAsymmetricPublicKey(result);
		}

		static std::string GetLocalAsymmetricPrivateKey(const any &) {
			return string();
		}

		static string GetLicenseKey(const any &) {
			return string();
		}

		static void StoreLocalAsymmetricPrivateKey(const string &) {
			//...//
		}

	};

	//////////////////////////////////////////////////////////////////////////

	template<class ClientTrait>
	struct XmlLicenseKeyRetrievePolicy<ClientTrait, true> {
	
		typedef XmlLicenseKeyRetrievePolicy<ClientTrait, false>
			Original;

		static string Import(const string &lKey, const string &pKey) {
			return Original::Import(lKey, pKey);
		}

		static bool Get(string &key, const any &clientParam) {
			return Test::licenseKeyTestServer->GetXmlLicenseKeyRetriever()
				?	Test::licenseKeyTestServer->GetXmlLicenseKeyRetriever()(key, clientParam)
				:	Original::Get(key, clientParam);
		}

	};

	//////////////////////////////////////////////////////////////////////////

	template<class ClientTrait>
	struct WorkstationPropertiesLocalPolicy<ClientTrait, true> {

		typedef WorkstationPropertiesLocalPolicy<ClientTrait, false>
			Original;

		static bool Get(WorkstationPropertyValues &result, const any &clientParam) {
			return Test::licenseKeyTestServer->GetLocalWorkstationPropertyValuesGetter()
				?	Test::licenseKeyTestServer
						->GetLocalWorkstationPropertyValuesGetter()(ref(result), clientParam)
				:	Original::Get(result, clientParam);
		}

	};

	//////////////////////////////////////////////////////////////////////////

	template<typename ClientTrait>
	struct LocalInfoQueryPolicy<ClientTrait, true> {

		typedef LocalInfoQueryPolicy<ClientTrait, false> Original;

		inline static auto_ptr<const LocalInfo> Get(
					const tex::Helpers::Xml::Document &doc,
					const Options &options,
					const any &clientParam) {
			auto_ptr<const LocalInfo> result = Original::Get(doc, options, clientParam);
			if (	result.get()
					&& !Test::licenseKeyTestServer->GetCurrentTime().is_not_a_date_time()) {
				const_cast<LocalInfo &>(*result).time
					= Test::licenseKeyTestServer->GetCurrentTime();
			}
			return result;
		}

	};

	//////////////////////////////////////////////////////////////////////////

} }

//////////////////////////////////////////////////////////////////////////

namespace Test {

	//////////////////////////////////////////////////////////////////////////
	
	string LoadEncryptedLicenseKey(const char *licenseKeyFileModif) {
		ostringstream oss;
		oss << "Resource\\LicenseKey" << licenseKeyFileModif << ".key";
		ifstream f(oss.str().c_str());
		f.unsetf(ios_base::skipws);
		return string(istreambuf_iterator<char>(f), istreambuf_iterator<char>());
	}

	//////////////////////////////////////////////////////////////////////////

	bool GetValidLicenseKeyXml(string &result, const any &) {
		ifstream f("Resource\\LicenseKeyValid.xml");
		BOOST_ASSERT(f);
		f.unsetf(ios_base::skipws);
		string(istreambuf_iterator<char>(f), istreambuf_iterator<char>()).swap(result);
		return true;
	}

	bool GetEmptyLicenseKeyXml(string &result, const any &) {
		result.clear();
		return false;
	}

	//////////////////////////////////////////////////////////////////////////

	bool GetInvalidLicenseKeyXml(string &result, const any &userParam) {
		const bool isExists = GetValidLicenseKeyXml(result, userParam);
		result.resize(result.size() - (result.size() / 4));
		return isExists;
	}

	//////////////////////////////////////////////////////////////////////////

	bool GetValidLocalWorkstationProperties(
				tex::Licensing::WorkstationPropertyValues &result,
				const any &) {
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
				const any &userParam) {
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
				const any &userParam) {
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
				const any &userParam) {
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
				const any &userParam) {
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

namespace Test { BOOST_AUTO_TEST_SUITE(Licensing) 

	BOOST_AUTO_TEST_CASE(NotExistsLicenseKey) {

		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				&GetEmptyLicenseKeyXml,
				&GetValidLocalWorkstationProperties));

		{
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
			BOOST_CHECK(!license.IsFeatureValueAvailable(true));
			BOOST_CHECK(!license.IsFeatureValueAvailable(false));
			BOOST_CHECK(license.GetOwner().empty());
			BOOST_CHECK(license.GetLicense().empty());
			BOOST_CHECK(
				license.GetEditionName()
				== License::EditionQuery::CastEditionToName(tex::Licensing::EDITION_STANDARD));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_NOT_EXISTS);
		}

		{
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(10));
			BOOST_CHECK(!license.IsFeatureAvailable(11));
		}

		{
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(5));
			BOOST_CHECK(!license.IsFeatureAvailable(5 + 1));
		}

		{
			typedef tex::Licensing::ProxyLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
		}

		{
			typedef tex::Licensing::EndpointIoSeparationLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
		}

	}

	BOOST_AUTO_TEST_CASE(ValidLicenseKey) {

		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				&GetValidLicenseKeyXml,
				&GetValidLocalWorkstationProperties));

		{
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			BOOST_CHECK(license.IsFeatureAvailable(true));
			BOOST_CHECK(license.IsFeatureValueAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
			BOOST_CHECK(!license.IsFeatureValueAvailable(false));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_NO);
			License::FeatureValue featureValue;
			BOOST_REQUIRE(license.GetFeatureValue(featureValue));
			BOOST_CHECK(featureValue == true);
			BOOST_CHECK(license.GetOwner() == "Join Smith and Co");
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_NO);
			BOOST_CHECK(license.GetEditionName() == "Professional");
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_NO);
			BOOST_CHECK(license.GetLicense() == "056F070E-2374-4A12-9DC6-9AAB0DB309E7");
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_NO);
			BOOST_CHECK(license.GetLimitationTimeFrom() == time_from_string("2009-08-03 12:08:00"));
			BOOST_CHECK(license.GetLimitationTimeTo() == time_from_string("2100-01-01 00:00:02"));
			BOOST_CHECK(license.GetUpdateTimeFrom() == time_from_string("2009-08-01 12:08:00"));
			BOOST_CHECK(license.GetUpdateTimeTo() == time_from_string("2100-01-03 00:00:00"));
		}
		{
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			BOOST_CHECK(license.IsFeatureAvailable(30));
			BOOST_CHECK(license.IsFeatureValueAvailable(30));
			BOOST_CHECK(!license.IsFeatureAvailable(30 + 1));
			BOOST_CHECK(!license.IsFeatureValueAvailable(30 + 1));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_NO);
			License::FeatureValue featureValue;
			BOOST_REQUIRE(license.GetFeatureValue(featureValue));
			BOOST_CHECK(featureValue == 30);
		}
		{
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			BOOST_CHECK(license.IsFeatureAvailable(40));
			BOOST_CHECK(license.IsFeatureValueAvailable(40));
			BOOST_CHECK(!license.IsFeatureAvailable(40 + 1));
			BOOST_CHECK(!license.IsFeatureValueAvailable(40 + 1));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_NO);
			License::FeatureValue featureValue;
			BOOST_REQUIRE(license.GetFeatureValue(featureValue));
			BOOST_CHECK(featureValue == 40);
		}
		{
			typedef tex::Licensing::ProxyLicenseTesting License;
			License license;
			BOOST_CHECK(license.IsFeatureAvailable(true));
			BOOST_CHECK(license.IsFeatureValueAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
			BOOST_CHECK(!license.IsFeatureValueAvailable(false));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_NO);
			License::FeatureValue featureValue;
			BOOST_REQUIRE(license.GetFeatureValue(featureValue));
			BOOST_CHECK(featureValue == true);
		}
	}

	BOOST_AUTO_TEST_CASE(InvalidWorkstationPropoertiesForLicenseKey) {

		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				&GetValidLicenseKeyXml,
				&GetLocalWorkstationPropertiesWithInvalidOsVolume));

		{
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(true));
			BOOST_CHECK(!license.IsFeatureValueAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
			BOOST_CHECK(!license.IsFeatureValueAvailable(false));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_WORSTATION);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(10));
			BOOST_CHECK(!license.IsFeatureAvailable(11));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_WORSTATION);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(5));
			BOOST_CHECK(!license.IsFeatureValueAvailable(5));
			BOOST_CHECK(!license.IsFeatureAvailable(6));
			BOOST_CHECK(!license.IsFeatureValueAvailable(6));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_WORSTATION);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::ProxyLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(true));
			BOOST_CHECK(!license.IsFeatureValueAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
			BOOST_CHECK(!license.IsFeatureValueAvailable(false));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_WORSTATION);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}

	}

	BOOST_AUTO_TEST_CASE(InvalidWorkstationPropoertiesForFeature) {

		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				&GetValidLicenseKeyXml,
				&GetLocalWorkstationPropertiesWithInvalidAdapter));

		{
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			BOOST_CHECK(license.IsFeatureAvailable(true));
			BOOST_CHECK(license.IsFeatureValueAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
			BOOST_CHECK(!license.IsFeatureValueAvailable(false));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_NO);
		}
		{
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(10));
			BOOST_CHECK(!license.IsFeatureValueAvailable(10));
			BOOST_CHECK(!license.IsFeatureAvailable(10 + 1));
			BOOST_CHECK(!license.IsFeatureValueAvailable(10 + 1));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_WORSTATION);
		}
		{
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			BOOST_CHECK(license.IsFeatureAvailable(40));
			BOOST_CHECK(license.IsFeatureValueAvailable(40));
			BOOST_CHECK(!license.IsFeatureAvailable(40 + 1));
			BOOST_CHECK(!license.IsFeatureValueAvailable(40 + 1));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_NO);
		}
		{
			typedef tex::Licensing::ProxyLicenseTesting License;
			License license;
			BOOST_CHECK(license.IsFeatureAvailable(true));
			BOOST_CHECK(license.IsFeatureValueAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
			BOOST_CHECK(!license.IsFeatureValueAvailable(false));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_NO);
		}

	}

	BOOST_AUTO_TEST_CASE(TooManyScoresForLicenseKey) {

		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				&GetValidLicenseKeyXml,
				&GetLocalWorkstationPropertiesWithInvalidOsVer));

		{
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(true));
			BOOST_CHECK(!license.IsFeatureValueAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
			BOOST_CHECK(!license.IsFeatureValueAvailable(false));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_SCORES);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(10));
			BOOST_CHECK(!license.IsFeatureValueAvailable(10));
			BOOST_CHECK(!license.IsFeatureAvailable(10 + 1));
			BOOST_CHECK(!license.IsFeatureValueAvailable(10 + 1));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_SCORES);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(5));
			BOOST_CHECK(!license.IsFeatureValueAvailable(5));
			BOOST_CHECK(!license.IsFeatureAvailable(5 + 1));
			BOOST_CHECK(!license.IsFeatureValueAvailable(5 + 1));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_SCORES);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::ProxyLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(true));
			BOOST_CHECK(!license.IsFeatureValueAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
			BOOST_CHECK(!license.IsFeatureValueAvailable(false));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_SCORES);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}

	}

	BOOST_AUTO_TEST_CASE(TooManyScoresForFeature) {

		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				&GetValidLicenseKeyXml,
				&GetLocalWorkstationPropertiesWithInvalidOsSerial));

		{
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			BOOST_CHECK(license.IsFeatureAvailable(true));
			BOOST_CHECK(license.IsFeatureValueAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
			BOOST_CHECK(!license.IsFeatureValueAvailable(false));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_NO);
			License::FeatureValue featureValue;
			BOOST_REQUIRE(license.GetFeatureValue(featureValue));
			BOOST_CHECK(featureValue == true);
		}
		{
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			BOOST_CHECK(license.IsFeatureAvailable(30));
			BOOST_CHECK(license.IsFeatureValueAvailable(30));
			BOOST_CHECK(!license.IsFeatureAvailable(30 + 1));
			BOOST_CHECK(!license.IsFeatureValueAvailable(30 + 1));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_NO);
			License::FeatureValue featureValue;
			BOOST_REQUIRE(license.GetFeatureValue(featureValue));
			BOOST_CHECK(featureValue == 30);
		}
		{
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(5));
			BOOST_CHECK(!license.IsFeatureValueAvailable(5));
			BOOST_CHECK(!license.IsFeatureAvailable(5 + 1));
			BOOST_CHECK(!license.IsFeatureValueAvailable(5 + 1));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_SCORES);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::ProxyLicenseTesting License;
			License license;
			BOOST_CHECK(license.IsFeatureAvailable(true));
			BOOST_CHECK(license.IsFeatureValueAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
			BOOST_CHECK(!license.IsFeatureValueAvailable(false));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_NO);
			License::FeatureValue featureValue;
			BOOST_REQUIRE(license.GetFeatureValue(featureValue));
			BOOST_CHECK(featureValue == true);
		}

	}

	BOOST_AUTO_TEST_CASE(HalfScoresForLicenseKey) {
		// Checking in TooManyScoresForFeature test
	}

	BOOST_AUTO_TEST_CASE(HalfScoresForFeature) {
		// Checking in TooManyScoresForFeature test
		// (for such features as "executing" and "tunnels count").
	}

	BOOST_AUTO_TEST_CASE(LicenseCacheResetsByCheckCount) {
		
		struct KeyRequestProxy {
			KeyRequestProxy()
					: requestsCount(0) {
				//...//
			}
			//! workaround for VS compiler warning C4180
			bool GetKey(string &result, const any &clientParam) {
				return const_cast<const KeyRequestProxy *>(this)
					->GetKey(result, clientParam);
			}
			bool GetKey(string &result, const any &clientParam) const {
				++requestsCount;
				return GetValidLicenseKeyXml(result, clientParam);
			}
			mutable size_t requestsCount;
		} keyRequestProxy;

		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				bind(&KeyRequestProxy::GetKey, &keyRequestProxy, _1, _2),
				&GetValidLocalWorkstationProperties));

		{
			keyRequestProxy.requestsCount = 0;
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			BOOST_CHECK(license.IsFeatureAvailable(true));
			BOOST_CHECK(license.IsFeatureValueAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
			BOOST_CHECK(!license.IsFeatureValueAvailable(false));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_NO);
			BOOST_CHECK(keyRequestProxy.requestsCount == 1);
		}
		{
			keyRequestProxy.requestsCount = 0;
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			BOOST_CHECK(license.IsFeatureAvailable(30));
			BOOST_CHECK(license.IsFeatureValueAvailable(30));
			license.ResetFeatureAvailabilityCache();
			BOOST_CHECK(!license.IsFeatureAvailable(30 + 1));
			BOOST_CHECK(!license.IsFeatureValueAvailable(30 + 1));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_NO);
			BOOST_CHECK(keyRequestProxy.requestsCount == 2);
		}
		{
			keyRequestProxy.requestsCount = 0;
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			for (size_t i = 1; i < license.GetMaximumChecksCountFromCache() + 1; ++i) {
				BOOST_CHECK(license.IsFeatureAvailable(4));
				BOOST_REQUIRE(
					(i <= license.GetMaximumChecksCountFromCache()
						&&  keyRequestProxy.requestsCount == 1)
					|| (i > license.GetMaximumChecksCountFromCache()
						&&  keyRequestProxy.requestsCount == 2));
			}
		}

	}

	BOOST_AUTO_TEST_CASE(NoStartedTimeForLicenseKey) {

		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				&GetValidLicenseKeyXml,
				&GetValidLocalWorkstationProperties,
				time_from_string("2009-08-02 12:07:59")));

		{
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(true));
			BOOST_CHECK(!license.IsFeatureValueAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
			BOOST_CHECK(!license.IsFeatureValueAvailable(false));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_TIME);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(10));
			BOOST_CHECK(!license.IsFeatureValueAvailable(10));
			BOOST_CHECK(!license.IsFeatureAvailable(10 + 1));
			BOOST_CHECK(!license.IsFeatureValueAvailable(10 + 1));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_TIME);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(5));
			BOOST_CHECK(!license.IsFeatureValueAvailable(5));
			BOOST_CHECK(!license.IsFeatureAvailable(5 + 1));
			BOOST_CHECK(!license.IsFeatureValueAvailable(5 + 1));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_TIME);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::ProxyLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(true));
			BOOST_CHECK(!license.IsFeatureValueAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
			BOOST_CHECK(!license.IsFeatureValueAvailable(false));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_TIME);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}

	}

	BOOST_AUTO_TEST_CASE(NoStartedTimeForLicenseKeyUpdate) {

		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				&GetValidLicenseKeyXml,
				&GetValidLocalWorkstationProperties,
				time_from_string("2009-08-01 00:01:59")));

		{
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(true));
			BOOST_CHECK(!license.IsFeatureValueAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
			BOOST_CHECK(!license.IsFeatureValueAvailable(false));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_UPDATE);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(10));
			BOOST_CHECK(!license.IsFeatureValueAvailable(10));
			BOOST_CHECK(!license.IsFeatureAvailable(10 + 1));
			BOOST_CHECK(!license.IsFeatureValueAvailable(10 + 1));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_UPDATE);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(5));
			BOOST_CHECK(!license.IsFeatureValueAvailable(5));
			BOOST_CHECK(!license.IsFeatureAvailable(5 + 1));
			BOOST_CHECK(!license.IsFeatureValueAvailable(5 + 1));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_UPDATE);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::ProxyLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(true));
			BOOST_CHECK(!license.IsFeatureValueAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
			BOOST_CHECK(!license.IsFeatureValueAvailable(false));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_UPDATE);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}

	}

	BOOST_AUTO_TEST_CASE(ExpiredTimeForLicenseKey) {

		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				&GetValidLicenseKeyXml,
				&GetValidLocalWorkstationProperties,
				time_from_string("2100-01-02 00:00:01")));

		{
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(true));
			BOOST_CHECK(!license.IsFeatureValueAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
			BOOST_CHECK(!license.IsFeatureValueAvailable(false));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_TIME);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(10));
			BOOST_CHECK(!license.IsFeatureValueAvailable(10));
			BOOST_CHECK(!license.IsFeatureAvailable(10 + 1));
			BOOST_CHECK(!license.IsFeatureValueAvailable(10 + 1));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_TIME);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(5));
			BOOST_CHECK(!license.IsFeatureValueAvailable(5));
			BOOST_CHECK(!license.IsFeatureAvailable(5 + 1));
			BOOST_CHECK(!license.IsFeatureValueAvailable(5 + 1));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_TIME);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::ProxyLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(true));
			BOOST_CHECK(!license.IsFeatureValueAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
			BOOST_CHECK(!license.IsFeatureValueAvailable(false));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_TIME);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}

	}

	BOOST_AUTO_TEST_CASE(ExpiredTimeForLicenseKeyUpdate) {

		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				&GetValidLicenseKeyXml,
				&GetValidLocalWorkstationProperties,
				time_from_string("2100-01-04 00:00:03")));

		{
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(true));
			BOOST_CHECK(!license.IsFeatureValueAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
			BOOST_CHECK(!license.IsFeatureValueAvailable(false));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_UPDATE);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(10));
			BOOST_CHECK(!license.IsFeatureValueAvailable(10));
			BOOST_CHECK(!license.IsFeatureAvailable(10 + 1));
			BOOST_CHECK(!license.IsFeatureValueAvailable(10 + 1));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_UPDATE);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(5));
			BOOST_CHECK(!license.IsFeatureValueAvailable(5));
			BOOST_CHECK(!license.IsFeatureAvailable(5 + 1));
			BOOST_CHECK(!license.IsFeatureValueAvailable(5 + 1));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_UPDATE);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::ProxyLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(true));
			BOOST_CHECK(!license.IsFeatureValueAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
			BOOST_CHECK(!license.IsFeatureValueAvailable(false));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_UPDATE);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}

	}

	BOOST_AUTO_TEST_CASE(NotStartedTimeForFeature) {

		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				&GetValidLicenseKeyXml,
				&GetValidLocalWorkstationProperties,
				time_from_string("2009-09-01 12:07:59")));

		{
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			BOOST_CHECK(license.IsFeatureAvailable(true));
			BOOST_CHECK(license.IsFeatureValueAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
			BOOST_CHECK(!license.IsFeatureValueAvailable(false));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_NO);
			License::FeatureValue featureValue;
			BOOST_REQUIRE(license.GetFeatureValue(featureValue));
			BOOST_CHECK(featureValue == true);
		}
		{
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			BOOST_CHECK(license.IsFeatureAvailable(30));
			BOOST_CHECK(license.IsFeatureValueAvailable(30));
			BOOST_CHECK(!license.IsFeatureAvailable(30 + 1));
			BOOST_CHECK(!license.IsFeatureValueAvailable(30 + 1));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_NO);
			License::FeatureValue featureValue;
			BOOST_REQUIRE(license.GetFeatureValue(featureValue));
			BOOST_CHECK(featureValue == 30);
		}
		{
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			BOOST_CHECK(license.IsFeatureAvailable(40));
			BOOST_CHECK(license.IsFeatureValueAvailable(40));
			BOOST_CHECK(!license.IsFeatureAvailable(40 + 1));
			BOOST_CHECK(!license.IsFeatureValueAvailable(40 + 1));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_NO);
			License::FeatureValue featureValue;
			BOOST_REQUIRE(license.GetFeatureValue(featureValue));
			BOOST_CHECK(featureValue == 40);
		}
		{
			typedef tex::Licensing::ProxyLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(true));
			BOOST_CHECK(!license.IsFeatureValueAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
			BOOST_CHECK(!license.IsFeatureValueAvailable(false));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_TIME);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}

	}

	BOOST_AUTO_TEST_CASE(ExpiredTimeForFeature) {

		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				&GetValidLicenseKeyXml,
				&GetValidLocalWorkstationProperties,
				time_from_string("2100-01-01 00:00:01")));

		{
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			BOOST_CHECK(license.IsFeatureAvailable(true));
			BOOST_CHECK(license.IsFeatureValueAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
			BOOST_CHECK(!license.IsFeatureValueAvailable(false));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_NO);
			License::FeatureValue featureValue;
			BOOST_REQUIRE(license.GetFeatureValue(featureValue));
			BOOST_CHECK(featureValue == true);
		}
		{
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			BOOST_CHECK(license.IsFeatureAvailable(30));
			BOOST_CHECK(license.IsFeatureValueAvailable(30));
			BOOST_CHECK(!license.IsFeatureAvailable(30 + 1));
			BOOST_CHECK(!license.IsFeatureValueAvailable(30 + 1));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_NO);
			License::FeatureValue featureValue;
			BOOST_REQUIRE(license.GetFeatureValue(featureValue));
			BOOST_CHECK(featureValue == 30);
		}
		{
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			BOOST_CHECK(license.IsFeatureAvailable(40));
			BOOST_CHECK(license.IsFeatureValueAvailable(40));
			BOOST_CHECK(!license.IsFeatureAvailable(40 + 1));
			BOOST_CHECK(!license.IsFeatureValueAvailable(40 + 1));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_NO);
			License::FeatureValue featureValue;
			BOOST_REQUIRE(license.GetFeatureValue(featureValue));
			BOOST_CHECK(featureValue == 40);
		}
		{
			typedef tex::Licensing::ProxyLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(true));
			BOOST_CHECK(!license.IsFeatureValueAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
			BOOST_CHECK(!license.IsFeatureValueAvailable(false));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_TIME);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}

	}

	BOOST_AUTO_TEST_CASE(InvalidLicenseKeyXml) {
		licenseKeyTestServer.reset(
			new LicenseKeyTestServer(
				&GetInvalidLicenseKeyXml,
				&GetValidLocalWorkstationProperties));
		{
			typedef tex::Licensing::ExeLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_FORMAT);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::TunnelLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(10));
			BOOST_CHECK(!license.IsFeatureValueAvailable(10));
			BOOST_CHECK(!license.IsFeatureAvailable(10 + 1));
			BOOST_CHECK(!license.IsFeatureValueAvailable(10 + 1));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_FORMAT);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::RuleSetLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(5));
			BOOST_CHECK(!license.IsFeatureValueAvailable(5));
			BOOST_CHECK(!license.IsFeatureAvailable(5 + 1));
			BOOST_CHECK(!license.IsFeatureValueAvailable(5 + 1));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_FORMAT);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}
		{
			typedef tex::Licensing::ProxyLicenseTesting License;
			License license;
			BOOST_CHECK(!license.IsFeatureAvailable(true));
			BOOST_CHECK(!license.IsFeatureValueAvailable(true));
			BOOST_CHECK(!license.IsFeatureAvailable(false));
			BOOST_CHECK(!license.IsFeatureValueAvailable(false));
			BOOST_CHECK(license.GetUnactivityReason() == tex::Licensing::UR_FORMAT);
			License::FeatureValue featureValue;
			BOOST_CHECK(!license.GetFeatureValue(featureValue));
		}
	}
	
	BOOST_AUTO_TEST_CASE(WorstationInfo) {
		tex::Licensing::WorkstationPropertyValues propValues;
		tex::Licensing::ExeLicense::WorkstationPropertiesLocal::Get(propValues, any());
		const size_t hashSize = 40;
		BOOST_REQUIRE(propValues.find(tex::Licensing::WORKSTATION_PROPERTY_OS_VER) != propValues.end());
		BOOST_CHECK(propValues[tex::Licensing::WORKSTATION_PROPERTY_OS_VER].size() == hashSize);
		BOOST_REQUIRE(propValues.find(tex::Licensing::WORKSTATION_PROPERTY_INSTALLATION_INFO) != propValues.end());
		BOOST_CHECK(propValues[tex::Licensing::WORKSTATION_PROPERTY_INSTALLATION_INFO].size() == hashSize);
		BOOST_REQUIRE(propValues.find(tex::Licensing::WORKSTATION_PROPERTY_ADAPTER) != propValues.end());
		BOOST_CHECK(propValues[tex::Licensing::WORKSTATION_PROPERTY_ADAPTER].size() >= hashSize);
		BOOST_REQUIRE(propValues.find(tex::Licensing::WORKSTATION_PROPERTY_OS_VOLUME) != propValues.end());
		BOOST_CHECK(propValues[tex::Licensing::WORKSTATION_PROPERTY_OS_VOLUME].size() == hashSize);
	}
	
	/* BOOST_AUTO_TEST_CASE(KeyRequest) {
		licenseKeyTestServer.reset(new LicenseKeyTestServer);
		tex::Licensing::OnlineKeyRequestTesting request(
			"3BA8C77D-4EDE-4622-C666-AA8EE4C46AB3");
		request.Send();
		BOOST_CHECK(request.TestKey<tex::Licensing::ExeLicense>());
	} */

BOOST_AUTO_TEST_SUITE_END() }
