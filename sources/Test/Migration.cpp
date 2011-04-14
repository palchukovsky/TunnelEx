/**************************************************************************
 *   Created: 2008/06/12 23:17
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Migration.cpp 959 2010-06-22 11:36:04Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "ServiceControl/Configuration.hpp"
#include "Legacy/LegacySupporter.hpp"

#include "Core/Endpoint.hpp"
#include "Core/EndpointAddress.hpp"
#include "Core/Rule.hpp"
#include "Core/String.hpp"

namespace ut = boost::unit_test;
namespace fs = boost::filesystem;
namespace tex = TunnelEx;
namespace xml = tex::Helpers::Xml;

namespace Test { BOOST_AUTO_TEST_SUITE(Migration)

	BOOST_AUTO_TEST_CASE(Rules_1_0) {

		const tex::String ruleSetXmlVer_1_0
			=	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
				"<RuleCollection Version=\"1.0\">"
					"<Rule Name=\"1-st rule\" Uuid=\"50f97b05-3621-4da6-94a0-91923de310bd\">"
						"<FilterCollection />"
						"<InputCollection>"
							"<Endpoint Address=\"host-212.213.214.1-from-hosts:21\" Uuid=\"e1579927-4838-4f05-b59d-2b1f664dfe05\" />"
							"<Endpoint Address=\"host-212.213.214.2-from-hosts:22\" Uuid=\"e2579927-4838-4f05-b59d-2b1f664dfe05\" />"
							"<Endpoint Address=\"host-212.213.214.3-from-hosts:23\" Uuid=\"e3579927-4838-4f05-b59d-2b1f664dfe05\" />"
						"</InputCollection>"
						"<DestinationCollection>"
							"<Endpoint Address=\"host-212.213.214.4-from-hosts:24\" Uuid=\"615e3014-7e4d-424e-a260-e1e915169f56\" />"
							"<Endpoint Address=\"host-212.213.214.5-from-hosts:25\" Uuid=\"625e3014-7e4d-424e-a260-e1e915169f56\" />"
							"<Endpoint Address=\"host-212.213.214.6-from-hosts:26\" Uuid=\"635e3014-7e4d-424e-a260-e1e915169f56\" />"
						"</DestinationCollection>"
					"</Rule>"
					"<Rule Name=\"2-nd rule\" Uuid=\"50f97b05-3621-4da6-94a0-91923de311bd\">"
						"<FilterCollection>"
							"<Filter Name=\"DestinationsSorter/Ping\" />"
							"<Filter Name=\"DestinationsSorter/Ping2\" />"
						"</FilterCollection>"
						"<InputCollection>"
							"<Endpoint Address=\"*:27\" Uuid=\"e9179927-4838-4f05-b59d-211f664dfe05\">"
								"<Listener Name=\"Forwarder/Ftp/Active\" Stream=\"in\" />"
								"<Listener Name=\"Forwarder/Ftp/Active2\" Stream=\"out\" />"
								"<Listener Name=\"Forwarder/Ftp/Active3\" Stream=\"out\" />"
							"</Endpoint>"
						"</InputCollection>"
						"<DestinationCollection>"
							"<Endpoint Address=\"host-212.213.214.8-from-hosts:28\" Uuid=\"622e3014-7e1d-424e-a260-e1e915169f56\">"
								"<Listener Name=\"Forwarder/Ftp/Passive\" Stream=\"in\" />"
							"</Endpoint>"
						"</DestinationCollection>"
					"</Rule>"
				"</RuleCollection>";

		LegacySupporter legacySupporter;
		legacySupporter.SetTestModeToggle(true);
		ServiceConfiguration::SetTestModeToggle(true);
		ServiceConfiguration::GetDefault()->Save();

		xml::Document::LoadFromString(ruleSetXmlVer_1_0)
			->Save(tex::ConvertString<tex::String>(ServiceConfiguration().GetRulesPath().c_str()).GetCStr());

		tex::RuleSet ruleSet;
		legacySupporter.MigrateCurrentRuleSet(ruleSet);
		const tex::TunnelRuleSet &t = ruleSet.GetTunnels();
		const tex::ServiceRuleSet &s = ruleSet.GetServices();

		BOOST_REQUIRE(t.GetSize() == 2);
		BOOST_REQUIRE(s.GetSize() == 0);

		BOOST_CHECK(t[0].GetName() == L"1-st rule");
		BOOST_CHECK(t[0].GetUuid() == L"50f97b05-3621-4da6-94a0-91923de310bd");
		BOOST_CHECK(t[0].GetErrorsTreatment() == tex::Rule::ERRORS_TREATMENT_ERROR);
		BOOST_CHECK(t[0].IsEnabled());
		BOOST_CHECK(t[1].GetName() == L"2-nd rule");
		BOOST_CHECK(t[1].GetUuid() == L"50f97b05-3621-4da6-94a0-91923de311bd");
		BOOST_CHECK(t[1].GetErrorsTreatment() == tex::Rule::ERRORS_TREATMENT_ERROR);
		BOOST_CHECK(t[1].IsEnabled());

		BOOST_CHECK(t[0].GetFilters().GetSize() == 0);

		BOOST_REQUIRE(t[1].GetFilters().GetSize() == 2);
		BOOST_CHECK(t[1].GetFilters()[0] == L"DestinationsSorter/Ping");
		BOOST_CHECK(t[1].GetFilters()[1] == L"DestinationsSorter/Ping2");

		BOOST_REQUIRE(t[0].GetInputs().GetSize() == 3);
		BOOST_CHECK(t[0].GetInputs()[0].GetUuid() == L"e1579927-4838-4f05-b59d-2b1f664dfe05");
		BOOST_CHECK(t[0].GetInputs()[1].GetUuid() == L"e2579927-4838-4f05-b59d-2b1f664dfe05");
		BOOST_CHECK(t[0].GetInputs()[2].GetUuid() == L"e3579927-4838-4f05-b59d-2b1f664dfe05");

		BOOST_REQUIRE(t[0].GetInputs()[0].IsCombined());
		BOOST_CHECK(t[0].GetInputs()[0].GetCombinedAddress()->GetResourceIdentifier() == L"tcp://host-212.213.214.1-from-hosts:21");
		BOOST_CHECK(t[0].GetInputs()[0].IsCombinedAcceptor());
		BOOST_REQUIRE(t[0].GetInputs()[1].IsCombined());
		BOOST_CHECK(t[0].GetInputs()[1].GetCombinedAddress()->GetResourceIdentifier() == L"tcp://host-212.213.214.2-from-hosts:22");
		BOOST_CHECK(t[0].GetInputs()[1].IsCombinedAcceptor());
		BOOST_REQUIRE(t[0].GetInputs()[2].IsCombined());
		BOOST_CHECK(t[0].GetInputs()[2].GetCombinedAddress()->GetResourceIdentifier() == L"tcp://host-212.213.214.3-from-hosts:23");
		BOOST_CHECK(t[0].GetInputs()[2].IsCombinedAcceptor());
		
		BOOST_REQUIRE(t[1].GetInputs().GetSize() == 1);
		BOOST_CHECK(t[1].GetInputs()[0].GetUuid() == L"e9179927-4838-4f05-b59d-211f664dfe05");

		BOOST_REQUIRE(t[1].GetInputs()[0].IsCombined());
		BOOST_CHECK(t[1].GetInputs()[0].GetCombinedAddress()->GetResourceIdentifier() == L"tcp://*:27");
		BOOST_CHECK(t[1].GetInputs()[0].IsCombinedAcceptor());

		BOOST_CHECK(t[0].GetInputs()[0].GetPreListeners().GetSize() == 0);
		BOOST_CHECK(t[0].GetInputs()[1].GetPreListeners().GetSize() == 0);
		BOOST_CHECK(t[0].GetInputs()[2].GetPreListeners().GetSize() == 0);
		BOOST_REQUIRE(t[1].GetInputs()[0].GetPreListeners().GetSize() == 1);
		BOOST_CHECK(t[1].GetInputs()[0].GetPreListeners()[0].name == L"Tunnel/Ftp/Passive");
		BOOST_CHECK(t[1].GetInputs()[0].GetPreListeners()[0].param.IsEmpty());
		
		BOOST_CHECK(t[0].GetInputs()[0].GetPostListeners().GetSize() == 0);
		BOOST_CHECK(t[0].GetInputs()[1].GetPostListeners().GetSize() == 0);
		BOOST_CHECK(t[0].GetInputs()[2].GetPostListeners().GetSize() == 0);
		BOOST_REQUIRE(t[1].GetInputs()[0].GetPostListeners().GetSize() == 2);
		BOOST_REQUIRE(t[1].GetInputs()[0].GetPostListeners()[0].name == L"Forwarder/Ftp/Active2");
		BOOST_REQUIRE(t[1].GetInputs()[0].GetPostListeners()[0].param.IsEmpty());
		BOOST_REQUIRE(t[1].GetInputs()[0].GetPostListeners()[1].name == L"Forwarder/Ftp/Active3");
		BOOST_REQUIRE(t[1].GetInputs()[0].GetPostListeners()[1].param.IsEmpty());

		BOOST_REQUIRE(t[0].GetDestinations().GetSize() == 3);
		BOOST_CHECK(t[0].GetDestinations()[0].GetUuid() == L"615e3014-7e4d-424e-a260-e1e915169f56");
		BOOST_CHECK(t[0].GetDestinations()[1].GetUuid() == L"625e3014-7e4d-424e-a260-e1e915169f56");
		BOOST_CHECK(t[0].GetDestinations()[2].GetUuid() == L"635e3014-7e4d-424e-a260-e1e915169f56");

		BOOST_REQUIRE(t[0].GetDestinations()[0].IsCombined());
		BOOST_CHECK(t[0].GetDestinations()[0].GetCombinedAddress()->GetResourceIdentifier() == L"tcp://host-212.213.214.4-from-hosts:24");
		BOOST_CHECK(!t[0].GetDestinations()[0].IsCombinedAcceptor());
		BOOST_REQUIRE(t[0].GetDestinations()[1].IsCombined());
		BOOST_CHECK(t[0].GetDestinations()[1].GetCombinedAddress()->GetResourceIdentifier() == L"tcp://host-212.213.214.5-from-hosts:25");
		BOOST_CHECK(!t[0].GetDestinations()[1].IsCombinedAcceptor());
		BOOST_REQUIRE(t[0].GetDestinations()[2].IsCombined());
		BOOST_CHECK(t[0].GetDestinations()[2].GetCombinedAddress()->GetResourceIdentifier() == L"tcp://host-212.213.214.6-from-hosts:26");
		BOOST_CHECK(!t[0].GetDestinations()[2].IsCombinedAcceptor());

		BOOST_REQUIRE(t[0].GetDestinations().GetSize() == 3);
		BOOST_CHECK(t[0].GetDestinations()[0].GetUuid() == L"615e3014-7e4d-424e-a260-e1e915169f56");
		BOOST_REQUIRE(t[0].GetDestinations()[0].IsCombined());
		BOOST_CHECK(t[0].GetDestinations()[0].GetCombinedAddress()->GetResourceIdentifier() == L"tcp://host-212.213.214.4-from-hosts:24");
		BOOST_CHECK(!t[0].GetDestinations()[0].IsCombinedAcceptor());
		BOOST_CHECK(t[0].GetDestinations()[1].GetUuid() == L"625e3014-7e4d-424e-a260-e1e915169f56");
		BOOST_REQUIRE(t[0].GetDestinations()[1].IsCombined());
		BOOST_CHECK(t[0].GetDestinations()[1].GetCombinedAddress()->GetResourceIdentifier() == L"tcp://host-212.213.214.5-from-hosts:25");
		BOOST_CHECK(!t[0].GetDestinations()[1].IsCombinedAcceptor());
		BOOST_CHECK(t[0].GetDestinations()[2].GetUuid() == L"635e3014-7e4d-424e-a260-e1e915169f56");
		BOOST_REQUIRE(t[0].GetDestinations()[2].IsCombined());
		BOOST_CHECK(t[0].GetDestinations()[2].GetCombinedAddress()->GetResourceIdentifier() == L"tcp://host-212.213.214.6-from-hosts:26");
		BOOST_CHECK(!t[0].GetDestinations()[2].IsCombinedAcceptor());

		BOOST_CHECK(t[0].GetDestinations()[0].GetPreListeners().GetSize() == 0);
		BOOST_CHECK(t[0].GetDestinations()[1].GetPreListeners().GetSize() == 0);
		BOOST_CHECK(t[0].GetDestinations()[2].GetPreListeners().GetSize() == 0);

		BOOST_CHECK(t[0].GetDestinations()[0].GetPostListeners().GetSize() == 0);
		BOOST_CHECK(t[0].GetDestinations()[1].GetPostListeners().GetSize() == 0);
		BOOST_CHECK(t[0].GetDestinations()[2].GetPostListeners().GetSize() == 0);

		BOOST_REQUIRE(t[1].GetDestinations()[0].GetPreListeners().GetSize() == 1);
		BOOST_CHECK(t[1].GetDestinations()[0].GetPreListeners()[0].name == L"Tunnel/Ftp/Active");
		BOOST_CHECK(t[1].GetDestinations()[0].GetPreListeners()[0].param.IsEmpty());

		BOOST_CHECK(t[1].GetDestinations()[0].GetPostListeners().GetSize() == 0);

	}

	BOOST_AUTO_TEST_CASE(Rules_1_1) {

		const tex::String ruleSetXmlVer_1_1
			=	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
				"<RuleCollection Version=\"1.1\">"
					"<Rule Name=\"1-st rule\" Uuid=\"50f97b05-3621-4da6-94a0-91923de310bd\">"
						"<FilterCollection />"
						"<InputCollection>"
							"<Endpoint ResourceIdentifier=\"tcp://host-212.213.214.1-from-hosts:21\" Uuid=\"e1579927-4838-4f05-b59d-2b1f664dfe05\" />"
							"<Endpoint ResourceIdentifier=\"tcp://host-212.213.214.2-from-hosts:22\" Uuid=\"e2579927-4838-4f05-b59d-2b1f664dfe05\" />"
							"<Endpoint ResourceIdentifier=\"udp://host-212.213.214.3-from-hosts:23\" Uuid=\"e3579927-4838-4f05-b59d-2b1f664dfe05\" />"
						"</InputCollection>"
						"<DestinationCollection>"
							"<Endpoint ResourceIdentifier=\"udp://host-212.213.214.4-from-hosts:24\" Uuid=\"615e3014-7e4d-424e-a260-e1e915169f56\" />"
							"<Endpoint ResourceIdentifier=\"tcp://host-212.213.214.5-from-hosts:25\" Uuid=\"625e3014-7e4d-424e-a260-e1e915169f56\" />"
							"<Endpoint ResourceIdentifier=\"pipe://asdasdasddddd\" Uuid=\"635e3014-7e4d-424e-a260-e1e915169f56\" />"
						"</DestinationCollection>"
					"</Rule>"
					"<Rule Name=\"2-nd rule\" Uuid=\"50f97b05-3621-4da6-94a0-91923de311bd\">"
						"<FilterCollection>"
							"<Filter Name=\"DestinationsSorter/Ping\" />"
							"<Filter Name=\"DestinationsSorter/Ping2\" />"
						"</FilterCollection>"
						"<InputCollection>"
							"<Endpoint ResourceIdentifier=\"tcp://*:27\" Uuid=\"e9179927-4838-4f05-b59d-211f664dfe05\">"
								"<PreListener Name=\"Forwarder/Ftp/Passive\" />"
								"<PostListener Name=\"Forwarder/Ftp/Active2\" />"
								"<PostListener Name=\"Forwarder/Ftp/Active3\" />"
							"</Endpoint>"
						"</InputCollection>"
						"<DestinationCollection>"
							"<Endpoint ResourceIdentifier=\"tcp://host-212.213.214.8-from-hosts:28\" Uuid=\"622e3014-7e1d-424e-a260-e1e915169f56\">"
								"<PreListener Name=\"Forwarder/Ftp/Active\" />"
							"</Endpoint>"
						"</DestinationCollection>"
					"</Rule>"
				"</RuleCollection>";

		LegacySupporter legacySupporter;
		legacySupporter.SetTestModeToggle(true);
		ServiceConfiguration::SetTestModeToggle(true);
		ServiceConfiguration::GetDefault()->Save();

		xml::Document::LoadFromString(ruleSetXmlVer_1_1)
			->Save(tex::ConvertString<tex::String>(ServiceConfiguration().GetRulesPath().c_str()).GetCStr());

		tex::RuleSet ruleSet;
		legacySupporter.MigrateCurrentRuleSet(ruleSet);
		const tex::ServiceRuleSet &s = ruleSet.GetServices();
		const tex::TunnelRuleSet &t = ruleSet.GetTunnels();

		BOOST_CHECK(s.GetSize() == 0);
		BOOST_REQUIRE(t.GetSize() == 2);
		
		BOOST_CHECK(t[0].GetName() == L"1-st rule");
		BOOST_CHECK(t[0].GetUuid() == L"50f97b05-3621-4da6-94a0-91923de310bd");
		BOOST_CHECK(t[0].GetErrorsTreatment() == tex::Rule::ERRORS_TREATMENT_ERROR);
		BOOST_CHECK(t[0].IsEnabled());
		BOOST_CHECK(t[1].GetName() == L"2-nd rule");
		BOOST_CHECK(t[1].GetUuid() == L"50f97b05-3621-4da6-94a0-91923de311bd");
		BOOST_CHECK(t[1].GetErrorsTreatment() == tex::Rule::ERRORS_TREATMENT_ERROR);
		BOOST_CHECK(t[1].IsEnabled());

		BOOST_CHECK(t[0].GetFilters().GetSize() == 0);
		BOOST_REQUIRE(t[1].GetFilters().GetSize() == 2);
		BOOST_CHECK(t[1].GetFilters()[0] == L"DestinationsSorter/Ping");
		BOOST_CHECK(t[1].GetFilters()[1] == L"DestinationsSorter/Ping2");

		BOOST_REQUIRE(t[0].GetInputs().GetSize() == 3);
		BOOST_CHECK(t[0].GetInputs()[0].GetUuid() == L"e1579927-4838-4f05-b59d-2b1f664dfe05");
		BOOST_CHECK(t[0].GetInputs()[1].GetUuid() == L"e2579927-4838-4f05-b59d-2b1f664dfe05");
		BOOST_CHECK(t[0].GetInputs()[2].GetUuid() == L"e3579927-4838-4f05-b59d-2b1f664dfe05");

		BOOST_REQUIRE(t[0].GetInputs()[0].IsCombined());
		BOOST_CHECK(t[0].GetInputs()[0].GetCombinedAddress()->GetResourceIdentifier() == L"tcp://host-212.213.214.1-from-hosts:21");
		BOOST_CHECK(t[0].GetInputs()[0].IsCombinedAcceptor());
		BOOST_REQUIRE(t[0].GetInputs()[1].IsCombined());
		BOOST_CHECK(t[0].GetInputs()[1].GetCombinedAddress()->GetResourceIdentifier() == L"tcp://host-212.213.214.2-from-hosts:22");
		BOOST_CHECK(t[0].GetInputs()[1].IsCombinedAcceptor());
		BOOST_REQUIRE(t[0].GetInputs()[2].IsCombined());
		BOOST_CHECK(t[0].GetInputs()[2].GetCombinedAddress()->GetResourceIdentifier() == L"udp://host-212.213.214.3-from-hosts:23");
		BOOST_CHECK(!t[0].GetInputs()[2].IsCombinedAcceptor());

		BOOST_REQUIRE(t[1].GetInputs().GetSize() == 1);
		BOOST_CHECK(t[1].GetInputs()[0].GetUuid() == L"e9179927-4838-4f05-b59d-211f664dfe05");
		BOOST_REQUIRE(t[1].GetInputs()[0].IsCombined());
		BOOST_CHECK(t[1].GetInputs()[0].GetCombinedAddress()->GetResourceIdentifier() == L"tcp://*:27");
		BOOST_CHECK(t[1].GetInputs()[0].IsCombinedAcceptor());
		
		BOOST_CHECK(t[0].GetInputs()[0].GetPreListeners().GetSize() == 0);
		BOOST_CHECK(t[0].GetInputs()[1].GetPreListeners().GetSize() == 0);
		BOOST_CHECK(t[0].GetInputs()[2].GetPreListeners().GetSize() == 0);
		BOOST_REQUIRE(t[1].GetInputs()[0].GetPreListeners().GetSize() == 1);
		BOOST_CHECK(t[1].GetInputs()[0].GetPreListeners()[0].name == L"Tunnel/Ftp/Passive");
		BOOST_CHECK(t[1].GetInputs()[0].GetPreListeners()[0].param.IsEmpty());

		BOOST_CHECK(t[0].GetInputs()[0].GetPostListeners().GetSize() == 0);
		BOOST_CHECK(t[0].GetInputs()[1].GetPostListeners().GetSize() == 0);
		BOOST_CHECK(t[0].GetInputs()[2].GetPostListeners().GetSize() == 0);
		BOOST_REQUIRE(t[1].GetInputs()[0].GetPostListeners().GetSize() == 2);
		BOOST_CHECK(t[1].GetInputs()[0].GetPostListeners()[0].name == L"Forwarder/Ftp/Active2");
		BOOST_CHECK(t[1].GetInputs()[0].GetPostListeners()[0].param.IsEmpty());
		BOOST_CHECK(t[1].GetInputs()[0].GetPostListeners()[1].name == L"Forwarder/Ftp/Active3");
		BOOST_CHECK(t[1].GetInputs()[0].GetPostListeners()[1].param.IsEmpty());

		BOOST_REQUIRE(t[0].GetDestinations().GetSize() == 3);
		BOOST_CHECK(t[0].GetDestinations()[0].GetUuid() == L"615e3014-7e4d-424e-a260-e1e915169f56");
		BOOST_CHECK(t[0].GetDestinations()[1].GetUuid() == L"625e3014-7e4d-424e-a260-e1e915169f56");
		BOOST_CHECK(t[0].GetDestinations()[2].GetUuid() == L"635e3014-7e4d-424e-a260-e1e915169f56");
		BOOST_CHECK(t[1].GetDestinations()[0].GetUuid() == L"622e3014-7e1d-424e-a260-e1e915169f56");

		BOOST_REQUIRE(t[0].GetDestinations()[0].IsCombined());
		BOOST_CHECK(t[0].GetDestinations()[0].GetCombinedAddress()->GetResourceIdentifier() == L"udp://host-212.213.214.4-from-hosts:24");
		BOOST_CHECK(!t[0].GetDestinations()[0].IsCombinedAcceptor());
		BOOST_REQUIRE(t[0].GetDestinations()[1].IsCombined());
		BOOST_CHECK(t[0].GetDestinations()[1].GetCombinedAddress()->GetResourceIdentifier() == L"tcp://host-212.213.214.5-from-hosts:25");
		BOOST_CHECK(!t[0].GetDestinations()[1].IsCombinedAcceptor());
		BOOST_REQUIRE(t[0].GetDestinations()[2].IsCombined());
		BOOST_CHECK(t[0].GetDestinations()[2].GetCombinedAddress()->GetResourceIdentifier() == L"pipe://asdasdasddddd");
		BOOST_CHECK(!t[0].GetDestinations()[2].IsCombinedAcceptor());
		BOOST_REQUIRE(t[1].GetDestinations()[0].IsCombined());
		BOOST_CHECK(t[1].GetDestinations()[0].GetCombinedAddress()->GetResourceIdentifier() == L"tcp://host-212.213.214.8-from-hosts:28");
		BOOST_CHECK(!t[1].GetDestinations()[0].IsCombinedAcceptor());

		BOOST_CHECK(t[0].GetDestinations()[0].GetPreListeners().GetSize() == 0);
		BOOST_CHECK(t[0].GetDestinations()[1].GetPreListeners().GetSize() == 0);
		BOOST_CHECK(t[0].GetDestinations()[2].GetPreListeners().GetSize() == 0);

		BOOST_CHECK(t[0].GetDestinations()[0].GetPostListeners().GetSize() == 0);
		BOOST_CHECK(t[0].GetDestinations()[1].GetPostListeners().GetSize() == 0);
		BOOST_CHECK(t[0].GetDestinations()[2].GetPostListeners().GetSize() == 0);

		BOOST_REQUIRE(t[1].GetDestinations()[0].GetPreListeners().GetSize() == 1);
		BOOST_CHECK(t[1].GetDestinations()[0].GetPreListeners()[0].name == L"Tunnel/Ftp/Active");
		BOOST_CHECK(t[1].GetDestinations()[0].GetPreListeners()[0].param.IsEmpty());

		BOOST_CHECK(t[1].GetDestinations()[0].GetPostListeners().GetSize() == 0);

	}

	BOOST_AUTO_TEST_CASE(Rules_1_2) {

		const tex::String ruleSetXmlVer_1_2
			=	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
				"<RuleSet Version=\"1.2\">"
					"<Rule Name=\"1-st rule\" IsEnabled=\"true\" ErrorsTreatment=\"information\" Uuid=\"50f97b05-3621-4da6-94a0-91923de310bd\">"
						"<FilterSet />"
						"<InputSet>"
							"<Endpoint ResourceIdentifier=\"tcp://host-212.213.214.1-from-hosts:21\" Uuid=\"e1579927-4838-4f05-b59d-2b1f664dfe05\" />"
							"<Endpoint ResourceIdentifier=\"pipe://ssdsdsdsd/dddd\" Uuid=\"e2579927-4838-4f05-b59d-2b1f664dfe05\" />"
							"<Endpoint ResourceIdentifier=\"udp://host-212.213.214.3-from-hosts:23\" Uuid=\"e3579927-4838-4f05-b59d-2b1f664dfe05\" />"
						"</InputSet>"
						"<DestinationSet>"
							"<Endpoint ResourceIdentifier=\"udp://host-212.213.214.4-from-hosts:24\" Uuid=\"615e3014-7e4d-424e-a260-e1e915169f56\" />"
							"<Endpoint ResourceIdentifier=\"tcp://host-212.213.214.5-from-hosts:25\" Uuid=\"625e3014-7e4d-424e-a260-e1e915169f56\" />"
							"<Endpoint ResourceIdentifier=\"pipe://asdasdasddddd\" Uuid=\"635e3014-7e4d-424e-a260-e1e915169f56\" />"
						"</DestinationSet>"
					"</Rule>"
					"<Rule Name=\"2-nd rule\" IsEnabled=\"false\" ErrorsTreatment=\"warning\"  Uuid=\"50f97b05-3621-4da6-94a0-91923de311bd\">"
						"<FilterSet>"
							"<Filter Name=\"DestinationsSorter/Ping\" />"
							"<Filter Name=\"DestinationsSorter/Ping2\" />"
						"</FilterSet>"
						"<InputSet>"
							"<Endpoint ResourceIdentifier=\"tcp://*:27\" Uuid=\"e9179927-4838-4f05-b59d-211f664dfe05\">"
								"<PreListener Name=\"Forwarder/Ftp/Passive\" />"
								"<PostListener Name=\"Forwarder/Ftp/Active2\" />"
								"<PostListener Name=\"Forwarder/Ftp/Active3\" />"
							"</Endpoint>"
						"</InputSet>"
						"<DestinationSet>"
							"<Endpoint ResourceIdentifier=\"tcp://host-212.213.214.8-from-hosts:28\" Uuid=\"622e3014-7e1d-424e-a260-e1e915169f56\">"
								"<PreListener Name=\"Forwarder/Ftp/Active\" />"
							"</Endpoint>"
						"</DestinationSet>"
					"</Rule>"
				"</RuleSet>";

		LegacySupporter legacySupporter;
		legacySupporter.SetTestModeToggle(true);
		ServiceConfiguration::SetTestModeToggle(true);
		ServiceConfiguration::GetDefault()->Save();

		xml::Document::LoadFromString(ruleSetXmlVer_1_2)
			->Save(tex::ConvertString<tex::String>(ServiceConfiguration().GetRulesPath().c_str()).GetCStr());

		tex::RuleSet ruleSet;
		legacySupporter.MigrateCurrentRuleSet(ruleSet);
		const tex::ServiceRuleSet &s = ruleSet.GetServices();
		const tex::TunnelRuleSet &t = ruleSet.GetTunnels();

		BOOST_CHECK(s.GetSize() == 0);
		BOOST_REQUIRE(t.GetSize() == 2);
		
		BOOST_CHECK(t[0].GetName() == L"1-st rule");
		BOOST_CHECK(t[0].GetUuid() == L"50f97b05-3621-4da6-94a0-91923de310bd");
		BOOST_CHECK(t[0].GetErrorsTreatment() == tex::Rule::ERRORS_TREATMENT_INFO);
		BOOST_CHECK(t[0].IsEnabled());
		BOOST_CHECK(t[1].GetName() == L"2-nd rule");
		BOOST_CHECK(t[1].GetUuid() == L"50f97b05-3621-4da6-94a0-91923de311bd");
		BOOST_CHECK(t[1].GetErrorsTreatment() == tex::Rule::ERRORS_TREATMENT_WARN);
		BOOST_CHECK(!t[1].IsEnabled());

		BOOST_CHECK(t[0].GetFilters().GetSize() == 0);
		BOOST_REQUIRE(t[1].GetFilters().GetSize() == 2);
		BOOST_CHECK(t[1].GetFilters()[0] == L"DestinationsSorter/Ping");
		BOOST_CHECK(t[1].GetFilters()[1] == L"DestinationsSorter/Ping2");

		BOOST_REQUIRE(t[0].GetInputs().GetSize() == 3);
		BOOST_CHECK(t[0].GetInputs()[0].GetUuid() == L"e1579927-4838-4f05-b59d-2b1f664dfe05");
		BOOST_CHECK(t[0].GetInputs()[1].GetUuid() == L"e2579927-4838-4f05-b59d-2b1f664dfe05");
		BOOST_CHECK(t[0].GetInputs()[2].GetUuid() == L"e3579927-4838-4f05-b59d-2b1f664dfe05");

		BOOST_REQUIRE(t[0].GetInputs()[0].IsCombined());
		BOOST_CHECK(t[0].GetInputs()[0].GetCombinedAddress()->GetResourceIdentifier() == L"tcp://host-212.213.214.1-from-hosts:21");
		BOOST_CHECK(t[0].GetInputs()[0].IsCombinedAcceptor());
		BOOST_REQUIRE(t[0].GetInputs()[1].IsCombined());
		BOOST_CHECK(t[0].GetInputs()[1].GetCombinedAddress()->GetResourceIdentifier() == L"pipe://ssdsdsdsd/dddd");
		BOOST_CHECK(t[0].GetInputs()[1].IsCombinedAcceptor());
		BOOST_REQUIRE(t[0].GetInputs()[2].IsCombined());
		BOOST_CHECK(t[0].GetInputs()[2].GetCombinedAddress()->GetResourceIdentifier() == L"udp://host-212.213.214.3-from-hosts:23");
		BOOST_CHECK(!t[0].GetInputs()[2].IsCombinedAcceptor());

		BOOST_REQUIRE(t[1].GetInputs().GetSize() == 1);
		BOOST_CHECK(t[1].GetInputs()[0].GetUuid() == L"e9179927-4838-4f05-b59d-211f664dfe05");
		BOOST_REQUIRE(t[1].GetInputs()[0].IsCombined());
		BOOST_CHECK(t[1].GetInputs()[0].GetCombinedAddress()->GetResourceIdentifier() == L"tcp://*:27");
		BOOST_CHECK(t[1].GetInputs()[0].IsCombinedAcceptor());

		BOOST_CHECK(t[0].GetInputs()[0].GetPreListeners().GetSize() == 0);
		BOOST_CHECK(t[0].GetInputs()[1].GetPreListeners().GetSize() == 0);
		BOOST_CHECK(t[0].GetInputs()[2].GetPreListeners().GetSize() == 0);
		BOOST_REQUIRE(t[1].GetInputs()[0].GetPreListeners().GetSize() == 1);
		BOOST_CHECK(t[1].GetInputs()[0].GetPreListeners()[0].name == L"Tunnel/Ftp/Passive");
		BOOST_CHECK(t[1].GetInputs()[0].GetPreListeners()[0].param.IsEmpty());

		BOOST_CHECK(t[0].GetInputs()[0].GetPostListeners().GetSize() == 0);
		BOOST_CHECK(t[0].GetInputs()[1].GetPostListeners().GetSize() == 0);
		BOOST_CHECK(t[0].GetInputs()[2].GetPostListeners().GetSize() == 0);
		BOOST_REQUIRE(t[1].GetInputs()[0].GetPostListeners().GetSize() == 2);
		BOOST_CHECK(t[1].GetInputs()[0].GetPostListeners()[0].name == L"Forwarder/Ftp/Active2");
		BOOST_CHECK(t[1].GetInputs()[0].GetPostListeners()[0].param.IsEmpty());
		BOOST_CHECK(t[1].GetInputs()[0].GetPostListeners()[1].name == L"Forwarder/Ftp/Active3");
		BOOST_CHECK(t[1].GetInputs()[0].GetPostListeners()[1].param.IsEmpty());

		BOOST_REQUIRE(t[0].GetDestinations().GetSize() == 3);
		BOOST_CHECK(t[0].GetDestinations()[0].GetUuid() == L"615e3014-7e4d-424e-a260-e1e915169f56");
		BOOST_CHECK(t[0].GetDestinations()[1].GetUuid() == L"625e3014-7e4d-424e-a260-e1e915169f56");
		BOOST_CHECK(t[0].GetDestinations()[2].GetUuid() == L"635e3014-7e4d-424e-a260-e1e915169f56");
		BOOST_CHECK(t[1].GetDestinations()[0].GetUuid() == L"622e3014-7e1d-424e-a260-e1e915169f56");

		BOOST_REQUIRE(t[0].GetDestinations()[0].IsCombined());
		BOOST_CHECK(t[0].GetDestinations()[0].GetCombinedAddress()->GetResourceIdentifier() == L"udp://host-212.213.214.4-from-hosts:24");
		BOOST_CHECK(!t[0].GetDestinations()[0].IsCombinedAcceptor());
		BOOST_REQUIRE(t[0].GetDestinations()[1].IsCombined());
		BOOST_CHECK(t[0].GetDestinations()[1].GetCombinedAddress()->GetResourceIdentifier() == L"tcp://host-212.213.214.5-from-hosts:25");
		BOOST_CHECK(!t[0].GetDestinations()[1].IsCombinedAcceptor());
		BOOST_REQUIRE(t[0].GetDestinations()[2].IsCombined());
		BOOST_CHECK(t[0].GetDestinations()[2].GetCombinedAddress()->GetResourceIdentifier() == L"pipe://asdasdasddddd");
		BOOST_CHECK(!t[0].GetDestinations()[2].IsCombinedAcceptor());
		BOOST_REQUIRE(t[1].GetDestinations()[0].IsCombined());
		BOOST_CHECK(t[1].GetDestinations()[0].GetCombinedAddress()->GetResourceIdentifier() == L"tcp://host-212.213.214.8-from-hosts:28");
		BOOST_CHECK(!t[1].GetDestinations()[0].IsCombinedAcceptor());

		BOOST_CHECK(t[0].GetDestinations()[0].GetPreListeners().GetSize() == 0);
		BOOST_CHECK(t[0].GetDestinations()[1].GetPreListeners().GetSize() == 0);
		BOOST_CHECK(t[0].GetDestinations()[2].GetPreListeners().GetSize() == 0);

		BOOST_CHECK(t[0].GetDestinations()[0].GetPostListeners().GetSize() == 0);
		BOOST_CHECK(t[0].GetDestinations()[1].GetPostListeners().GetSize() == 0);
		BOOST_CHECK(t[0].GetDestinations()[2].GetPostListeners().GetSize() == 0);

		BOOST_REQUIRE(t[1].GetDestinations()[0].GetPreListeners().GetSize() == 1);
		BOOST_CHECK(t[1].GetDestinations()[0].GetPreListeners()[0].name == L"Tunnel/Ftp/Active");
		BOOST_CHECK(t[1].GetDestinations()[0].GetPreListeners()[0].param.IsEmpty());

		BOOST_CHECK(t[1].GetDestinations()[0].GetPostListeners().GetSize() == 0);

	}

	BOOST_AUTO_TEST_CASE(Rules_1_3) {

		LegacySupporter legacySupporter;
		legacySupporter.SetTestModeToggle(true);
		ServiceConfiguration::SetTestModeToggle(true);
		ServiceConfiguration::GetDefault()->Save();

		{
			fs::wpath ruleSetPath = L"Resource";
			ruleSetPath /= L"RuleSet_1_3.xml";
			std::ifstream orig(ruleSetPath.string().c_str());
			BOOST_REQUIRE(orig);
			std::ofstream test(ServiceConfiguration().GetRulesPath().c_str(), std::ios::trunc);
			BOOST_REQUIRE(test);
			test << std::string(
				std::istreambuf_iterator<char>(orig),
				std::istreambuf_iterator<char>());
		}

		tex::RuleSet ruleSet;
		legacySupporter.MigrateCurrentRuleSet(ruleSet);
		const tex::ServiceRuleSet &services = ruleSet.GetServices();
		const tex::TunnelRuleSet &tunnels = ruleSet.GetTunnels();

		BOOST_CHECK(services.GetSize() == 0);
		BOOST_REQUIRE(tunnels.GetSize() == 3);
		BOOST_REQUIRE(tunnels[0].GetFilters().GetSize() == 0);
		BOOST_REQUIRE(tunnels[0].GetInputs().GetSize() == 1);
		BOOST_REQUIRE(tunnels[0].GetInputs()[0].GetPreListeners().GetSize() == 0);
		BOOST_REQUIRE(tunnels[0].GetInputs()[0].GetPostListeners().GetSize() == 0);
		BOOST_REQUIRE(tunnels[0].GetDestinations().GetSize() == 1);
		BOOST_REQUIRE(tunnels[0].GetDestinations()[0].GetPreListeners().GetSize() == 0);
		BOOST_REQUIRE(tunnels[0].GetDestinations()[0].GetPostListeners().GetSize() == 0);
		BOOST_REQUIRE(tunnels[1].GetFilters().GetSize() == 1);
		BOOST_REQUIRE(tunnels[1].GetInputs().GetSize() == 2);
		BOOST_REQUIRE(tunnels[1].GetInputs()[0].GetPreListeners().GetSize() == 1);
		BOOST_REQUIRE(tunnels[1].GetInputs()[0].GetPostListeners().GetSize() == 0);
		BOOST_REQUIRE(tunnels[1].GetDestinations().GetSize() == 2);
		BOOST_REQUIRE(tunnels[1].GetDestinations()[0].GetPreListeners().GetSize() == 0);
		BOOST_REQUIRE(tunnels[1].GetDestinations()[0].GetPostListeners().GetSize() == 0);
		BOOST_REQUIRE(tunnels[2].GetFilters().GetSize() == 0);
		BOOST_REQUIRE(tunnels[2].GetInputs().GetSize() == 1);
		BOOST_REQUIRE(tunnels[2].GetInputs()[0].GetPreListeners().GetSize() == 1);
		BOOST_REQUIRE(tunnels[2].GetInputs()[0].GetPostListeners().GetSize() == 0);
		BOOST_REQUIRE(tunnels[2].GetDestinations().GetSize() == 1);
		BOOST_REQUIRE(tunnels[2].GetDestinations()[0].GetPreListeners().GetSize() == 0);
		BOOST_REQUIRE(tunnels[2].GetDestinations()[0].GetPostListeners().GetSize() == 1);

		BOOST_REQUIRE(tunnels[0].GetInputs()[0].IsCombined());
		BOOST_REQUIRE(tunnels[0].GetInputs()[0].IsCombinedAcceptor());
		BOOST_REQUIRE(tunnels[0].GetDestinations()[0].IsCombined());
		BOOST_REQUIRE(!tunnels[0].GetDestinations()[0].IsCombinedAcceptor());
		BOOST_REQUIRE(!tunnels[1].GetInputs()[0].IsCombined());
		BOOST_REQUIRE(tunnels[1].GetInputs()[0].GetReadWriteAcceptor() == tex::Endpoint::ACCEPTOR_READER);
		BOOST_REQUIRE(!tunnels[1].GetDestinations()[1].IsCombined());
		BOOST_REQUIRE(tunnels[1].GetInputs()[1].GetReadWriteAcceptor() == tex::Endpoint::ACCEPTOR_NONE);
		BOOST_REQUIRE(!tunnels[1].GetDestinations()[0].IsCombined());
		BOOST_REQUIRE(tunnels[1].GetDestinations()[0].GetReadWriteAcceptor() == tex::Endpoint::ACCEPTOR_NONE);
		BOOST_REQUIRE(!tunnels[1].GetDestinations()[1].IsCombined());
		BOOST_REQUIRE(tunnels[1].GetDestinations()[1].GetReadWriteAcceptor() == tex::Endpoint::ACCEPTOR_NONE);
		BOOST_REQUIRE(tunnels[2].GetInputs()[0].IsCombined());
		BOOST_REQUIRE(!tunnels[2].GetInputs()[0].IsCombinedAcceptor());
		BOOST_REQUIRE(tunnels[2].GetDestinations()[0].IsCombined());
		BOOST_REQUIRE(!tunnels[2].GetDestinations()[0].IsCombinedAcceptor());

		BOOST_CHECK(tunnels[0].GetName() == L"1-1-1-1-1");
		BOOST_CHECK(tunnels[0].IsEnabled());
		BOOST_CHECK(tunnels[0].GetErrorsTreatment() == tex::Rule::ERRORS_TREATMENT_WARN);
		BOOST_CHECK(tunnels[0].GetUuid() == L"468e23c1-f4a9-4bd8-8609-15c9daa2030a");
		BOOST_CHECK(tunnels[1].GetName() == L"2-2-2-2-2");
		BOOST_CHECK(!tunnels[1].IsEnabled());
		BOOST_CHECK(tunnels[1].GetErrorsTreatment() == tex::Rule::ERRORS_TREATMENT_ERROR);
		BOOST_CHECK(tunnels[1].GetUuid() == L"b03335d0-45f1-496a-8507-89be3d860b44");
		BOOST_CHECK(tunnels[2].GetName() == L"ftp");
		BOOST_CHECK(tunnels[2].IsEnabled());
		BOOST_CHECK(tunnels[2].GetErrorsTreatment() == tex::Rule::ERRORS_TREATMENT_WARN);
		BOOST_CHECK(tunnels[2].GetUuid() == L"d1ae6295-6973-4cb9-80d2-a72ebf349e56");

		BOOST_CHECK(tunnels[0].GetInputs()[0].GetUuid() == L"8c739428-1e9f-4222-b349-751addb0596e");
		BOOST_CHECK(tunnels[1].GetInputs()[0].GetUuid() == L"67061610-c22c-420b-9c65-854416519f74");
		BOOST_CHECK(tunnels[1].GetInputs()[1].GetUuid() == L"62361610-c22c-420b-9c65-854416519f74");
		BOOST_CHECK(tunnels[2].GetInputs()[0].GetUuid() == L"6df638a6-a368-442e-8bd4-f3dfd330bcc4");

		BOOST_CHECK(tunnels[0].GetDestinations()[0].GetUuid() == L"ffa144aa-6257-4bcf-aa61-976b952fd69d");
		BOOST_CHECK(tunnels[1].GetDestinations()[0].GetUuid() == L"7611979d-53fd-4942-9a58-4c624ce40051");
		BOOST_CHECK(tunnels[1].GetDestinations()[1].GetUuid() == L"1611979d-53fd-4942-9a58-4c624ce40051");
		BOOST_CHECK(tunnels[2].GetDestinations()[0].GetUuid() == L"8090688a-0e22-477d-97a2-b89f535f8813");

		BOOST_CHECK(tunnels[0].GetInputs()[0].GetCombinedResourceIdentifier() == L"tcp://*:80?adapter=all");
		BOOST_CHECK(tunnels[1].GetInputs()[0].GetReadResourceIdentifier() == L"tcp://*:1111?adapter=%7B22ECF6FF-F5CA-4C63-847C-EBA3E1490781%7D");
		BOOST_CHECK(tunnels[1].GetInputs()[0].GetWriteResourceIdentifier() == L"udp://*:2222?adapter=all");
		BOOST_CHECK(tunnels[1].GetInputs()[1].GetReadResourceIdentifier() == L"tcp://qqqqqq:1112");
		BOOST_CHECK(tunnels[1].GetInputs()[1].GetWriteResourceIdentifier() == L"udp://gggg:2223?adapter=all");
		BOOST_CHECK(tunnels[2].GetInputs()[0].GetCombinedResourceIdentifier() == L"tcp://*:21?adapter=all");
		
		BOOST_CHECK(tunnels[0].GetDestinations()[0].GetCombinedResourceIdentifier() == L"udp://qqqqq:1234");
		BOOST_CHECK(tunnels[1].GetDestinations()[0].GetReadResourceIdentifier() == L"pipe://ppp");
		BOOST_CHECK(tunnels[1].GetDestinations()[0].GetWriteResourceIdentifier() == L"serial://COM1?baudrate=9600&databits=8&stopbits=1&parity=none&flowcontrol=xon%2Fxoff");
		BOOST_CHECK(tunnels[1].GetDestinations()[1].GetReadResourceIdentifier() == L"pipe://ppp2");
		BOOST_CHECK(tunnels[1].GetDestinations()[1].GetWriteResourceIdentifier() == L"serial://COM2?baudrate=9600&databits=8&stopbits=1&parity=none&flowcontrol=xon%2Fxoff");
		BOOST_CHECK(tunnels[2].GetDestinations()[0].GetCombinedResourceIdentifier() == L"pathfinder://qweqwe:21?proxy=http://asdasd:8080");

		BOOST_CHECK(tunnels[1].GetFilters()[0] == L"DestinationsSorter/Ping");

		BOOST_CHECK(tunnels[1].GetInputs()[0].GetPreListeners()[0].name == L"TrafficLogger/File");
		BOOST_CHECK(tunnels[1].GetInputs()[0].GetPreListeners()[0].param == L"c:\\zzzzz");

		BOOST_CHECK(tunnels[2].GetDestinations()[0].GetPostListeners()[0].name == L"Tunnel/Ftp/Active");
		BOOST_CHECK(tunnels[2].GetDestinations()[0].GetPostListeners()[0].param == L"123");

	}

	BOOST_AUTO_TEST_CASE(ServiceConfiguration_1_0) {

		const tex::String serviceConfigurationXmlVer_1_0
			=	"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
				"<Configuration Version=\"1.0\">"
					"<Rules>C:/xxx/YYY/zzz/Ruuuuules.xml</Rules>"
					"<Log Level=\"error\" MaxSize=\"123456\">C:/zzz/YYY/xxx/TeeeexService.log</Log>"
				"</Configuration>";

		LegacySupporter legacySupporter;
		legacySupporter.SetTestModeToggle(true);
		ServiceConfiguration::SetTestModeToggle(true);
		xml::Document::LoadFromString(serviceConfigurationXmlVer_1_0)
			->Save(ServiceConfiguration::GetConfigurationFilePath());

		const boost::shared_ptr<const ServiceConfiguration> migrated(
			new ServiceConfiguration(
				LegacySupporter().MigrateCurrentServiceConfiguration().Get()));

		BOOST_CHECK(migrated->GetLogPath() == L"C:/zzz/YYY/xxx/TeeeexService.log");
		BOOST_CHECK(migrated->GetLogLevel() == tex::LOG_LEVEL_ERROR);
		BOOST_CHECK(migrated->GetRulesPath() == L"C:/xxx/YYY/zzz/Ruuuuules.xml");
		BOOST_CHECK(migrated->GetMaxLogSize() == 123456);
		BOOST_CHECK(migrated->IsServerStarted() == false);
		BOOST_CHECK(migrated->IsChanged() == true);

	}

	BOOST_AUTO_TEST_CASE(ServiceConfiguration_1_1) {

		const tex::String serviceConfigurationXmlVer_1_1
			=	"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
				"<Configuration Version=\"1.1\">"
					"<Rules>C:/xxx/YYY/zzz/Ruuuuules.xml</Rules>"
					"<Log Level=\"error\" MaxSize=\"123456\">C:/zzz/YYY/xxx/TeeeexService.log</Log>"
					"<ServerState>stopped</ServerState>"
				"</Configuration>";

		LegacySupporter legacySupporter;
		legacySupporter.SetTestModeToggle(true);
		ServiceConfiguration::SetTestModeToggle(true);
		xml::Document::LoadFromString(serviceConfigurationXmlVer_1_1)
			->Save(ServiceConfiguration::GetConfigurationFilePath());

		const boost::shared_ptr<const ServiceConfiguration> migrated(
			new ServiceConfiguration(
				LegacySupporter().MigrateCurrentServiceConfiguration().Get()));

		BOOST_CHECK(migrated->GetLogPath() == L"C:/zzz/YYY/xxx/TeeeexService.log");
		BOOST_CHECK(migrated->GetLogLevel() == tex::LOG_LEVEL_ERROR);
		BOOST_CHECK(migrated->GetRulesPath() == L"C:/xxx/YYY/zzz/Ruuuuules.xml");
		BOOST_CHECK(migrated->GetMaxLogSize() == 123456);
		BOOST_CHECK(migrated->IsServerStarted() == false);
		BOOST_CHECK(migrated->IsChanged() == true);

	}

BOOST_AUTO_TEST_SUITE_END() }

