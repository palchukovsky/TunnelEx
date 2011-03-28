/**************************************************************************
 *   Created: 2007/11/07 23:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.com
 * -------------------------------------------------------------------
 *       $Id: Rule.cpp 990 2010-09-08 01:39:18Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "PipeServer.hpp"
#include "Wait.h"

#include "Modules/Inet/InetEndpointAddress.hpp"

#include <TunnelEx/Rule.hpp>
#include <TunnelEx/EndpointAddress.hpp>
#include <TunnelEx/Server.hpp>
#include <TunnelEx/Log.hpp>
#include <TunnelEx/Exceptions.hpp>
#include <TunnelEx/String.hpp>

namespace ut = boost::unit_test;
namespace tex = TunnelEx;
namespace xml = tex::Helpers::Xml;
using namespace std;
using namespace boost;

namespace Test { BOOST_AUTO_TEST_SUITE(Rule)

	BOOST_AUTO_TEST_CASE(XmlParse) {
		
		const wchar_t *const xml
			=	L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
				L"<RuleSet Version=\"2.1\">"
				L"	<ServiceRule Name=\"First service rule\" Uuid=\"12355577-E705-4805-BAE9-D16D522DB726\" ErrorsTreatment=\"error\" IsEnabled=\"true\">"
				L"		<Service Name=\"Service nameeee\" Uuid=\"43215577-E705-4805-BAE9-D16D522DB726\">1234567</Service>"
				L"	</ServiceRule>"
				L"	<ServiceRule Name=\"2-nd service rule\" Uuid=\"33333337-E705-4805-BAE9-D16D522DB726\" ErrorsTreatment=\"warning\" IsEnabled=\"false\">"
				L"		<Service Name=\"Service name 1\" Uuid=\"43215577-3334-5805-BAE9-D16D522DB726\">qwerty</Service>"
				L"		<Service Name=\"Service name 2\" Uuid=\"41233333-3344-4805-BAE9-D16D522DB726\" />"
				L"	</ServiceRule>"
				L"	<TunnelRule Name=\"First rule\" Uuid=\"AC055577-E705-4805-BAE9-D16D522DB726\" ErrorsTreatment=\"error\" IsEnabled=\"true\">"
				L"		<FilterSet>"
				L"			<Filter Name=\"some filter name\" />"
				L"			<Filter Name=\"second filter name\" />"
				L"		</FilterSet>"
				L"		<InputSet>"
				L"			<Endpoint Uuid=\"AC055577-E705-480a-BAE9-D11D522DB726\">"
				L"				 <CombinedAddress ResourceIdentifier=\"tcp://host-212.213.214.1-from-hosts:755\" IsAcceptor=\"true\" />"
				L"			</Endpoint>"
				L"			<Endpoint Uuid=\"AC055577-E705-480a-BAE9-D12D522DB726\">"
				L"				<PreListener Name=\"SomeListenerNumberOne\">Some Listener Number One Param</PreListener>"
				L"				<PreListener Name=\"SomeListenerNumberOne/2\">Some Listener Number One Param / 2</PreListener>"
				L"				<PostListener Name=\"SomeListenerNumberOne/3\">Some Listener Number One Param / 3</PostListener>"
				L"				<CombinedAddress ResourceIdentifier=\"tcp://212.213.214.2:69\" IsAcceptor=\"false\" />"
				L"			</Endpoint>"
				L"		</InputSet>"
				L"		<DestinationSet>"
				L"			<Endpoint Uuid=\"AC055577-E705-480a-BAE9-D13D522DB726\">"
				L"				<PreListener Name=\"SomeListenerNumberTwo\">Some Listener Number Two Param</PreListener>"
				L"				<CombinedAddress ResourceIdentifier=\"tcp://host-212.213.214.3-from-hosts:80\" />"
				L"			</Endpoint>"
				L"			<Endpoint Uuid=\"AC055577-E705-480a-BAE9-D14D522DB726\">"
				L"				<CombinedAddress ResourceIdentifier=\"tcp://host-212.213.214.6-from-hosts:86\" />"
				L"			</Endpoint>"
				L"		</DestinationSet>"
				L"	</TunnelRule>"
				L"	<TunnelRule Name=\"��� ������� �� ������� ����� ������ ��������� ������ � UTF-8\" Uuid=\"AC055577-E705-480a-BAE9-D16D522DB727\" ErrorsTreatment=\"warning\"  IsEnabled=\"false\">"
				L"		<FilterSet />"
				L"		<InputSet>"
				L"			<Endpoint Uuid=\"AC055577-E705-480a-BAE9-D15D522DB726\">"
				L"				<SplitAddress Acceptor=\"reader\" ReadResourceIdentifier=\"tcp://*:234\" WriteResourceIdentifier=\"tcp://zx:233\" />"
				L"			</Endpoint>"
				L"			<Endpoint Uuid=\"FC055577-E705-4803-BAE9-D15D522DB72F\">"
				L"				<SplitAddress Acceptor=\"none\" ReadResourceIdentifier=\"tcp://ttt:238\" WriteResourceIdentifier=\"tcp://zx:239\" />"
				L"			</Endpoint>"
				L"		</InputSet>"
				L"		<DestinationSet>"
				L"			<Endpoint Uuid=\"AC055577-E705-4809-BAE9-D16D522DB726\">"
				L"				<SplitAddress ReadResourceIdentifier=\"tcp://212.213.214.5:80\" WriteResourceIdentifier=\"tcp://212.213.214.9:81\" />"
				L"			</Endpoint>"
				L"		</DestinationSet>"
				L"	</TunnelRule>"
				L"</RuleSet>";

		const tex::RuleSet p(xml);
		
		// rules number:
		BOOST_REQUIRE(2 == p.GetServices().GetSize());
		BOOST_REQUIRE(2 == p.GetTunnels().GetSize());
		// rules names:
		BOOST_CHECK(tex::WString(L"First rule") == p.GetTunnels()[0].GetName());
		BOOST_CHECK(p.GetTunnels()[0].GetUuid() == L"AC055577-E705-4805-BAE9-D16D522DB726");
		BOOST_CHECK(	p.GetTunnels()[1].GetName()
						== L"��� ������� �� ������� ����� ������ ��������� ������ � UTF-8");
		BOOST_CHECK(p.GetTunnels()[1].GetUuid() == L"AC055577-E705-480a-BAE9-D16D522DB727");
		BOOST_CHECK(p.GetServices()[0].GetName() == L"First service rule");
		BOOST_CHECK(p.GetServices()[0].GetUuid() == L"12355577-E705-4805-BAE9-D16D522DB726");
		BOOST_CHECK(p.GetServices()[1].GetName() == L"2-nd service rule");
		BOOST_CHECK(p.GetServices()[1].GetUuid() == L"33333337-E705-4805-BAE9-D16D522DB726");
		
		BOOST_CHECK(p.GetTunnels()[0].GetErrorsTreatment() == tex::TunnelRule::ERRORS_TREATMENT_ERROR);
		BOOST_CHECK(p.GetTunnels()[1].GetErrorsTreatment() == tex::TunnelRule::ERRORS_TREATMENT_WARN);
		BOOST_CHECK(p.GetServices()[0].GetErrorsTreatment() == tex::TunnelRule::ERRORS_TREATMENT_ERROR);
		BOOST_CHECK(p.GetServices()[1].GetErrorsTreatment() == tex::TunnelRule::ERRORS_TREATMENT_WARN);

		BOOST_CHECK(p.GetTunnels()[0].IsEnabled() == true);
		BOOST_CHECK(p.GetTunnels()[1].IsEnabled() == false);
		BOOST_CHECK(p.GetServices()[0].IsEnabled() == true);
		BOOST_CHECK(p.GetServices()[1].IsEnabled() == false);

		// services
		BOOST_REQUIRE(p.GetServices()[0].GetServices().GetSize() == 1);
		BOOST_CHECK(p.GetServices()[0].GetServices()[0].name == L"Service nameeee");
		BOOST_CHECK(p.GetServices()[0].GetServices()[0].uuid == L"43215577-E705-4805-BAE9-D16D522DB726");
		BOOST_CHECK(p.GetServices()[0].GetServices()[0].param  == L"1234567");
		BOOST_CHECK(p.GetServices()[1].GetServices()[0].name == L"Service name 1");
		BOOST_CHECK(p.GetServices()[1].GetServices()[0].uuid == L"43215577-3334-5805-BAE9-D16D522DB726");
		BOOST_CHECK(p.GetServices()[1].GetServices()[0].param  == L"qwerty");
		BOOST_CHECK(p.GetServices()[1].GetServices()[1].name == L"Service name 2");
		BOOST_CHECK(p.GetServices()[1].GetServices()[1].uuid == L"41233333-3344-4805-BAE9-D16D522DB726");
		BOOST_CHECK(p.GetServices()[1].GetServices()[1].param.IsEmpty());
		
		// input endpoints number:
		BOOST_REQUIRE(2 == p.GetTunnels()[0].GetInputs().GetSize());
		BOOST_REQUIRE(2 == p.GetTunnels()[0].GetInputs()[1].GetPreListeners().GetSize());
		BOOST_REQUIRE(1 == p.GetTunnels()[0].GetInputs()[1].GetPostListeners().GetSize());
		BOOST_REQUIRE(2 == p.GetTunnels()[1].GetInputs().GetSize());

		// input endpoints
		BOOST_REQUIRE(p.GetTunnels()[0].GetInputs()[0].IsCombined());
		BOOST_CHECK(p.GetTunnels()[0].GetInputs()[0].IsCombinedAcceptor());
		BOOST_CHECK(	p.GetTunnels()[0].GetInputs()[0].GetCombinedAddress()->GetResourceIdentifier()
						== L"tcp://host-212.213.214.1-from-hosts:755");
		BOOST_CHECK(	p.GetTunnels()[0].GetInputs()[0].GetUuid()
						== L"AC055577-E705-480a-BAE9-D11D522DB726");
		BOOST_REQUIRE(p.GetTunnels()[0].GetInputs()[1].IsCombined());
		BOOST_CHECK(!p.GetTunnels()[0].GetInputs()[1].IsCombinedAcceptor());
		BOOST_CHECK(	p.GetTunnels()[0].GetInputs()[1].GetCombinedAddress()->GetResourceIdentifier()
						== L"tcp://212.213.214.2:69");
		BOOST_CHECK(	p.GetTunnels()[0].GetInputs()[1].GetUuid()
						== L"AC055577-E705-480a-BAE9-D12D522DB726");
		BOOST_REQUIRE(!p.GetTunnels()[1].GetInputs()[0].IsCombined());
		BOOST_CHECK(	p.GetTunnels()[1].GetInputs()[0].GetReadAddress()->GetResourceIdentifier()
						== L"tcp://*:234");
		BOOST_CHECK(	p.GetTunnels()[1].GetInputs()[0].GetWriteAddress()->GetResourceIdentifier()
						== L"tcp://zx:233");
		BOOST_CHECK(	p.GetTunnels()[1].GetInputs()[0].GetUuid()
						== L"AC055577-E705-480a-BAE9-D15D522DB726");
		BOOST_CHECK(p.GetTunnels()[1].GetInputs()[0].GetReadWriteAcceptor() == tex::Endpoint::ACCEPTOR_READER);
		BOOST_REQUIRE(!p.GetTunnels()[1].GetInputs()[1].IsCombined());
		BOOST_CHECK(	p.GetTunnels()[1].GetInputs()[1].GetReadAddress()->GetResourceIdentifier()
						== L"tcp://ttt:238");
		BOOST_CHECK(	p.GetTunnels()[1].GetInputs()[1].GetWriteAddress()->GetResourceIdentifier()
						== L"tcp://zx:239");
		BOOST_CHECK(	p.GetTunnels()[1].GetInputs()[1].GetUuid()
						== L"FC055577-E705-4803-BAE9-D15D522DB72F");
		BOOST_CHECK(p.GetTunnels()[1].GetInputs()[1].GetReadWriteAcceptor() == tex::Endpoint::ACCEPTOR_NONE);

		// destinations endpoints number:
		BOOST_REQUIRE(2 == p.GetTunnels()[0].GetDestinations().GetSize());
		BOOST_REQUIRE(1 == p.GetTunnels()[0].GetDestinations()[0].GetPreListeners().GetSize());
		BOOST_REQUIRE(1 == p.GetTunnels()[1].GetDestinations().GetSize());
		
		// destination endpoints:
		BOOST_REQUIRE(p.GetTunnels()[0].GetDestinations()[0].IsCombined());
		BOOST_CHECK(	p.GetTunnels()[0].GetDestinations()[0].GetCombinedAddress()->GetResourceIdentifier()
						== L"tcp://host-212.213.214.3-from-hosts:80");
		BOOST_CHECK(	p.GetTunnels()[0].GetDestinations()[0].GetUuid()
						== L"AC055577-E705-480a-BAE9-D13D522DB726");
		BOOST_REQUIRE(p.GetTunnels()[0].GetDestinations()[1].IsCombined());
		BOOST_CHECK(	p.GetTunnels()[0].GetDestinations()[1].GetCombinedAddress()->GetResourceIdentifier()
						== L"tcp://host-212.213.214.6-from-hosts:86");
		BOOST_CHECK(	p.GetTunnels()[0].GetDestinations()[1].GetUuid()
						== L"AC055577-E705-480a-BAE9-D14D522DB726");
		BOOST_REQUIRE(!p.GetTunnels()[1].GetDestinations()[0].IsCombined());
		BOOST_CHECK(	p.GetTunnels()[1].GetDestinations()[0].GetReadAddress()->GetResourceIdentifier()
						== L"tcp://212.213.214.5:80");
		BOOST_CHECK(	p.GetTunnels()[1].GetDestinations()[0].GetWriteAddress()->GetResourceIdentifier()
						== L"tcp://212.213.214.9:81");
		BOOST_CHECK(	p.GetTunnels()[1].GetDestinations()[0].GetUuid()
						== L"AC055577-E705-4809-BAE9-D16D522DB726");

		// filters:
		BOOST_REQUIRE(2 == p.GetTunnels()[0].GetFilters().GetSize());
		BOOST_REQUIRE(0 == p.GetTunnels()[1].GetFilters().GetSize());
		BOOST_CHECK(tex::WString(L"some filter name") == p.GetTunnels()[0].GetFilters()[0]);
		BOOST_CHECK(tex::WString(L"second filter name") == p.GetTunnels()[0].GetFilters()[1]);

		// listeners
		BOOST_CHECK(	p.GetTunnels()[0].GetInputs()[1].GetPreListeners()[0].name
						== L"SomeListenerNumberOne");
		BOOST_CHECK(	p.GetTunnels()[0].GetInputs()[1].GetPreListeners()[1].name
						== L"SomeListenerNumberOne/2");
		BOOST_CHECK(	p.GetTunnels()[0].GetInputs()[1].GetPostListeners()[0].name
						== L"SomeListenerNumberOne/3");
		BOOST_CHECK(	p.GetTunnels()[0].GetInputs()[1].GetPreListeners()[0].param
						== L"Some Listener Number One Param");
		BOOST_CHECK(	p.GetTunnels()[0].GetInputs()[1].GetPreListeners()[1].param
						== L"Some Listener Number One Param / 2");
		BOOST_CHECK(	p.GetTunnels()[0].GetInputs()[1].GetPostListeners()[0].param
						== L"Some Listener Number One Param / 3");
		BOOST_CHECK(	p.GetTunnels()[0].GetDestinations()[0].GetPreListeners()[0].name
						== L"SomeListenerNumberTwo");
		BOOST_CHECK(	p.GetTunnels()[0].GetDestinations()[0].GetPreListeners()[0].param
						== L"Some Listener Number Two Param");

	}

	BOOST_AUTO_TEST_CASE(XmlSave) {
		tex::TunnelRuleSet rules(2);
		{
			tex::TunnelRule rule;
			rule.SetName(L"First rule");
			rule.SetErrorsTreatment(tex::TunnelRule::ERRORS_TREATMENT_INFO);
			rule.Enable(false);
			tex::TunnelRule::Filters filters(2);
			filters.Append(L"first filter name - example");
			filters.Append(L"second filter name - example too");
			rule.SetFilters(filters);
			tex::RuleEndpointCollection inputs(2);
			{
				tex::RuleEndpoint input(L"tcp://host-212.213.214.1-from-hosts:102", true);
				tex::RuleEndpoint::ListenerInfo inListener;
				inListener.name = L"input/listener test";
				inListener.param = L"input/listener parameter";
				input.GetPreListeners().Append(inListener);
				tex::RuleEndpoint::ListenerInfo outListener;
				outListener.name = L"output/listener test";
				outListener.param = L"output/listener parameter";
				input.GetPostListeners().Append(outListener);
				inputs.Append(input);
			}
			inputs.Append(tex::RuleEndpoint(L"tcp://google.com:103", false));
			rule.SetInputs(inputs);
			tex::RuleEndpointCollection destinations(1);
			{
				tex::RuleEndpoint destination(L"tcp://host-212.213.214.3-from-hosts:104", false);
				tex::RuleEndpoint::ListenerInfo inListener;
				inListener.name = L"input/listener test 2";
				inListener.param = L"input/listener parameter 2";
				destination.GetPreListeners().Append(inListener);
				destinations.Append(destination);
			}
			rule.SetDestinations(destinations);
			rules.Append(rule);
		}
		{
			tex::TunnelRule rule;
			rule.SetName(L"��� ���� ��� ������� �� ������� ����� ������ ��������� ������ � UTF-8");
			rule.SetErrorsTreatment(tex::TunnelRule::ERRORS_TREATMENT_WARN);
			tex::RuleEndpointCollection inputs(1);
			inputs.Append(
				tex::RuleEndpoint(
					L"tcp://*:105",
					L"tcp://xxx:106",
					tex::Endpoint::ACCEPTOR_READER));
			inputs.Append(
				tex::RuleEndpoint(
					L"tcp://xxx:107",
					L"tcp://xxx:108",
					tex::Endpoint::ACCEPTOR_NONE));
			rule.SetInputs(inputs);
			tex::RuleEndpointCollection destinations(1);
			destinations.Append(
				tex::RuleEndpoint(
					L"tcp://host-212.213.214.5-from-hosts:106",
					L"tcp://host-212.213.214.5-from-hosts:108",
					tex::Endpoint::ACCEPTOR_NONE));
			destinations.Append(
				tex::RuleEndpoint(
					L"tcp://host-212.213.214.6-from-hosts:116",
					false));
			rule.SetDestinations(destinations);
			rules.Append(rule);
		}
		tex::WString xml;
		tex::RuleSet(tex::ServiceRuleSet(), rules).GetXml(xml);
		{
			BOOST_REQUIRE_NO_THROW(const tex::RuleSet parseTest(xml));
		}
		shared_ptr<const xml::XPath> xpath(
			xml::Document::LoadFromString(xml)->GetXPath());
		xml::ConstNodeCollection  queryResult;
		xpath->Query("/RuleSet/TunnelRule", queryResult);
		BOOST_REQUIRE(2 == queryResult.size());
		tex::String strBuf;
		tex::WString wStrBuf;
		BOOST_REQUIRE_NO_THROW(queryResult[0]->GetAttribute("Name", wStrBuf));
		BOOST_CHECK(wStrBuf == L"First rule");
		BOOST_REQUIRE_NO_THROW(queryResult[1]->GetAttribute("Name", wStrBuf));
		BOOST_CHECK(	wStrBuf
						== L"��� ���� ��� ������� �� ������� ����� ������ ��������� ������ � UTF-8");
		BOOST_REQUIRE_NO_THROW(queryResult[0]->GetAttribute("ErrorsTreatment", wStrBuf));
		BOOST_CHECK(wStrBuf == L"information");
		BOOST_REQUIRE_NO_THROW(queryResult[1]->GetAttribute("ErrorsTreatment", wStrBuf));
		BOOST_CHECK(wStrBuf == L"warning");
		BOOST_CHECK_NO_THROW(queryResult[0]->GetAttribute("IsEnabled", wStrBuf));
		BOOST_CHECK(wStrBuf == L"false");
		BOOST_CHECK_NO_THROW(queryResult[1]->GetAttribute("IsEnabled", wStrBuf));
		BOOST_CHECK(wStrBuf == L"true");
		xpath->Query(
			"/RuleSet/TunnelRule[1]/FilterSet/Filter",
			queryResult);
		BOOST_REQUIRE(2 == queryResult.size());
		BOOST_CHECK(queryResult[0]->GetAttribute("Name", strBuf) == "first filter name - example");
		BOOST_CHECK(queryResult[1]->GetAttribute("Name", strBuf) == "second filter name - example too");
		xpath->Query(
			"/RuleSet/TunnelRule[2]/FilterSet/Filter",
			queryResult);
		BOOST_REQUIRE(0 == queryResult.size());
		xpath->Query(
			"/RuleSet/TunnelRule[1]/InputSet/Endpoint",
			queryResult);
		BOOST_REQUIRE(2 == queryResult.size());
		xpath->Query(
			"/RuleSet/TunnelRule[1]/InputSet/Endpoint[1]/CombinedAddress",
			queryResult);
		BOOST_REQUIRE(1 == queryResult.size());
		BOOST_CHECK(	queryResult[0]->GetAttribute("ResourceIdentifier", strBuf)
						== "tcp://host-212.213.214.1-from-hosts:102");
		BOOST_CHECK(queryResult[0]->GetAttribute("IsAcceptor", strBuf) == "true");

		xpath->Query(
			"/RuleSet/TunnelRule[1]/InputSet/Endpoint[2]/CombinedAddress",
			queryResult);
		BOOST_REQUIRE(1 == queryResult.size());
		BOOST_CHECK(	queryResult[0]->GetAttribute("ResourceIdentifier", strBuf)
						== "tcp://google.com:103");
		BOOST_CHECK(queryResult[0]->GetAttribute("IsAcceptor", strBuf) == "false");

		xpath->Query(
			"/RuleSet/TunnelRule[1]/InputSet/Endpoint/PreListener",
			queryResult);
		BOOST_REQUIRE(1 == queryResult.size());
		BOOST_CHECK(queryResult[0]->GetAttribute("Name", strBuf) == "input/listener test");
		BOOST_CHECK(queryResult[0]->GetContent(strBuf) == "input/listener parameter");
		
		xpath->Query(
			"/RuleSet/TunnelRule[1]/InputSet/Endpoint/PostListener",
			queryResult);
		BOOST_REQUIRE(1 == queryResult.size());
		BOOST_CHECK(queryResult[0]->GetAttribute("Name", strBuf) == "output/listener test");
		BOOST_CHECK(queryResult[0]->GetContent(strBuf) == "output/listener parameter");

		xpath->Query("/RuleSet/TunnelRule[1]/DestinationSet/Endpoint", queryResult);
		BOOST_REQUIRE(1 == queryResult.size());

		xpath->Query(
			"/RuleSet/TunnelRule[1]/DestinationSet/Endpoint[1]/CombinedAddress",
			queryResult);
		BOOST_CHECK(	queryResult[0]->GetAttribute("ResourceIdentifier", strBuf)
						== "tcp://host-212.213.214.3-from-hosts:104");
		BOOST_CHECK(!queryResult[0]->HasAttribute("IsAccepter"));
		
		xpath->Query(
			"/RuleSet/TunnelRule[1]/DestinationSet/Endpoint/PreListener",
			queryResult);
		BOOST_REQUIRE(1 == queryResult.size());
		BOOST_CHECK(queryResult[0]->GetAttribute("Name", strBuf) == "input/listener test 2");
		BOOST_CHECK(queryResult[0]->GetContent(strBuf) == "input/listener parameter 2");
		
		xpath->Query(
			"/RuleSet/TunnelRule[2]/InputSet/Endpoint",
			queryResult);
		BOOST_REQUIRE(2 == queryResult.size());

		xpath->Query(
			"/RuleSet/TunnelRule[2]/InputSet/Endpoint/SplitAddress",
			queryResult);
		BOOST_REQUIRE(2 == queryResult.size());
		BOOST_CHECK(	queryResult[0]->GetAttribute("ReadResourceIdentifier", strBuf)
						== "tcp://*:105");
		BOOST_CHECK(	queryResult[0]->GetAttribute("WriteResourceIdentifier", strBuf)
						== "tcp://xxx:106");
		BOOST_CHECK(queryResult[0]->GetAttribute("Acceptor", strBuf) == "reader");
		BOOST_CHECK(	queryResult[1]->GetAttribute("ReadResourceIdentifier", strBuf)
						== "tcp://xxx:107");
		BOOST_CHECK(	queryResult[1]->GetAttribute("WriteResourceIdentifier", strBuf)
						== "tcp://xxx:108");
		BOOST_CHECK(queryResult[1]->GetAttribute("Acceptor", strBuf) == "none");

		xpath->Query(
			"/RuleSet/TunnelRule[2]/DestinationSet/Endpoint",
			queryResult);
		BOOST_REQUIRE(2 == queryResult.size());

		xpath->Query(
			"/RuleSet/TunnelRule[2]/DestinationSet/Endpoint/SplitAddress",
			queryResult);
		BOOST_REQUIRE(1 == queryResult.size());
		BOOST_CHECK(	queryResult[0]->GetAttribute("ReadResourceIdentifier", strBuf)
						== "tcp://host-212.213.214.5-from-hosts:106");
		BOOST_CHECK(	queryResult[0]->GetAttribute("WriteResourceIdentifier", strBuf)
						== "tcp://host-212.213.214.5-from-hosts:108");
		BOOST_CHECK(!queryResult[0]->HasAttribute("Acceptor"));

		xpath->Query(
			"/RuleSet/TunnelRule[2]/DestinationSet/Endpoint/CombinedAddress",
			queryResult);
		BOOST_REQUIRE(1 == queryResult.size());
		BOOST_CHECK(	queryResult[0]->GetAttribute("ResourceIdentifier", strBuf)
						== "tcp://host-212.213.214.6-from-hosts:116");
		BOOST_CHECK(!queryResult[0]->HasAttribute("Acceptor"));

	}

	BOOST_AUTO_TEST_CASE(Enabling) {
	
		struct RuleAppender : private noncopyable {
			explicit RuleAppender(tex::RuleSet &ruleSet)
					: m_ruleSet(ruleSet) {
				//...//
			}
			void AddRule(
						const bool isEnbaled,
						const wchar_t *const path) {
				tex::TunnelRule rule;
				rule.Enable(isEnbaled);
				tex::RuleEndpointCollection inputs(1);
				inputs.Append(tex::RuleEndpoint(path, true));
				rule.SetInputs(inputs);
				tex::RuleEndpointCollection destinations(1);
				destinations.Append(
					tex::RuleEndpoint(L"pipe://TunnelEx/EnablingTesting/NoExits", false));
				rule.SetDestinations(destinations);
				m_ruleSet.Append(rule);
			}
		private:
			tex::RuleSet &m_ruleSet;
		};
	
		tex::RuleSet rules;
		{
			RuleAppender appender(rules);
			appender.AddRule(
				true,
				L"pipe://TunnelEx/EnablingTesting1");
			appender.AddRule(
				false,
				L"pipe://TunnelEx/EnablingTesting2");
			appender.AddRule(
				true,
				L"pipe://TunnelEx/EnablingTesting3");
		}
		
		{
			tex::SslCertificatesStorage certStorage(L"", 0, 0);
			tex::Server::GetInstance().Start(rules, certStorage);
			BOOST_CHECK(tex::Server::GetInstance().GetOpenedEndpointsNumber() == 2);
			tex::Server::GetInstance().Stop();
		}

	}

	BOOST_AUTO_TEST_CASE(EndpointWithProxy) {
		
		using namespace TunnelEx::Mods::Inet;
		
		TcpEndpointAddress address(
			L"destination_host:987?proxy=http://no_auth_proxy_host:376&proxy=http://asDAsdceed:sdfuweWderWf@auth_ptoxy_host:10&proxy=http://uuu@test:1");
		
		BOOST_CHECK(address.GetHostName() == L"destination_host");
		BOOST_CHECK(address.GetPort() == 987);

		BOOST_REQUIRE(address.GetProxyList().size() == 3);

		vector<TunnelEx::Mods::Inet::Proxy> proxyList(
			address.GetProxyList().begin(), address.GetProxyList().end());
		
		BOOST_CHECK(proxyList[0].host == L"no_auth_proxy_host");
		BOOST_CHECK(proxyList[0].port == 376);
		BOOST_CHECK(proxyList[0].user.empty());
		BOOST_CHECK(proxyList[0].password.empty());

		BOOST_CHECK(proxyList[1].host == L"auth_ptoxy_host");
		BOOST_CHECK(proxyList[1].port == 10);
		BOOST_CHECK(proxyList[1].user == L"asDAsdceed");
		BOOST_CHECK(proxyList[1].password == L"sdfuweWderWf");

		BOOST_CHECK(proxyList[2].host == L"test");
		BOOST_CHECK(proxyList[2].port == 1);
		BOOST_CHECK(proxyList[2].user == L"uuu");
		BOOST_CHECK(proxyList[2].password.empty());

	}

BOOST_AUTO_TEST_SUITE_END() }

