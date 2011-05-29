/**************************************************************************
 *   Created: 2007/11/07 23:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.com
 **************************************************************************/

#include "Prec.h"

#include "TestUtils/PipeServer.hpp"
#include "TestUtils/Wait.h"

#include "Modules/Inet/InetEndpointAddress.hpp"

#include "Core/Rule.hpp"
#include "Core/EndpointAddress.hpp"
#include "Core/Server.hpp"
#include "Core/Log.hpp"
#include "Core/Exceptions.hpp"
#include "Core/String.hpp"

namespace tex = TunnelEx;
namespace xml = tex::Helpers::Xml;

namespace Test {

	TEST(Rule, XmlParse) {
		
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
		ASSERT_TRUE(2 == p.GetServices().GetSize());
		ASSERT_TRUE(2 == p.GetTunnels().GetSize());
		// rules names:
		EXPECT_TRUE(tex::WString(L"First rule") == p.GetTunnels()[0].GetName());
		EXPECT_TRUE(p.GetTunnels()[0].GetUuid() == L"AC055577-E705-4805-BAE9-D16D522DB726");
		EXPECT_TRUE(	p.GetTunnels()[1].GetName()
						== L"��� ������� �� ������� ����� ������ ��������� ������ � UTF-8");
		EXPECT_TRUE(p.GetTunnels()[1].GetUuid() == L"AC055577-E705-480a-BAE9-D16D522DB727");
		EXPECT_TRUE(p.GetServices()[0].GetName() == L"First service rule");
		EXPECT_TRUE(p.GetServices()[0].GetUuid() == L"12355577-E705-4805-BAE9-D16D522DB726");
		EXPECT_TRUE(p.GetServices()[1].GetName() == L"2-nd service rule");
		EXPECT_TRUE(p.GetServices()[1].GetUuid() == L"33333337-E705-4805-BAE9-D16D522DB726");
		
		EXPECT_TRUE(p.GetTunnels()[0].GetErrorsTreatment() == tex::TunnelRule::ERRORS_TREATMENT_ERROR);
		EXPECT_TRUE(p.GetTunnels()[1].GetErrorsTreatment() == tex::TunnelRule::ERRORS_TREATMENT_WARN);
		EXPECT_TRUE(p.GetServices()[0].GetErrorsTreatment() == tex::TunnelRule::ERRORS_TREATMENT_ERROR);
		EXPECT_TRUE(p.GetServices()[1].GetErrorsTreatment() == tex::TunnelRule::ERRORS_TREATMENT_WARN);

		EXPECT_TRUE(p.GetTunnels()[0].IsEnabled() == true);
		EXPECT_TRUE(p.GetTunnels()[1].IsEnabled() == false);
		EXPECT_TRUE(p.GetServices()[0].IsEnabled() == true);
		EXPECT_TRUE(p.GetServices()[1].IsEnabled() == false);

		// services
		ASSERT_TRUE(p.GetServices()[0].GetServices().GetSize() == 1);
		EXPECT_TRUE(p.GetServices()[0].GetServices()[0].name == L"Service nameeee");
		EXPECT_TRUE(p.GetServices()[0].GetServices()[0].uuid == L"43215577-E705-4805-BAE9-D16D522DB726");
		EXPECT_TRUE(p.GetServices()[0].GetServices()[0].param  == L"1234567");
		EXPECT_TRUE(p.GetServices()[1].GetServices()[0].name == L"Service name 1");
		EXPECT_TRUE(p.GetServices()[1].GetServices()[0].uuid == L"43215577-3334-5805-BAE9-D16D522DB726");
		EXPECT_TRUE(p.GetServices()[1].GetServices()[0].param  == L"qwerty");
		EXPECT_TRUE(p.GetServices()[1].GetServices()[1].name == L"Service name 2");
		EXPECT_TRUE(p.GetServices()[1].GetServices()[1].uuid == L"41233333-3344-4805-BAE9-D16D522DB726");
		EXPECT_TRUE(p.GetServices()[1].GetServices()[1].param.IsEmpty());
		
		// input endpoints number:
		ASSERT_TRUE(2 == p.GetTunnels()[0].GetInputs().GetSize());
		ASSERT_TRUE(2 == p.GetTunnels()[0].GetInputs()[1].GetPreListeners().GetSize());
		ASSERT_TRUE(1 == p.GetTunnels()[0].GetInputs()[1].GetPostListeners().GetSize());
		ASSERT_TRUE(2 == p.GetTunnels()[1].GetInputs().GetSize());

		// input endpoints
		ASSERT_TRUE(p.GetTunnels()[0].GetInputs()[0].IsCombined());
		EXPECT_TRUE(p.GetTunnels()[0].GetInputs()[0].IsCombinedAcceptor());
		EXPECT_TRUE(	p.GetTunnels()[0].GetInputs()[0].GetCombinedAddress()->GetResourceIdentifier()
						== L"tcp://host-212.213.214.1-from-hosts:755");
		EXPECT_TRUE(	p.GetTunnels()[0].GetInputs()[0].GetUuid()
						== L"AC055577-E705-480a-BAE9-D11D522DB726");
		ASSERT_TRUE(p.GetTunnels()[0].GetInputs()[1].IsCombined());
		EXPECT_FALSE(p.GetTunnels()[0].GetInputs()[1].IsCombinedAcceptor());
		EXPECT_TRUE(	p.GetTunnels()[0].GetInputs()[1].GetCombinedAddress()->GetResourceIdentifier()
						== L"tcp://212.213.214.2:69");
		EXPECT_TRUE(	p.GetTunnels()[0].GetInputs()[1].GetUuid()
						== L"AC055577-E705-480a-BAE9-D12D522DB726");
		ASSERT_TRUE(!p.GetTunnels()[1].GetInputs()[0].IsCombined());
		EXPECT_TRUE(	p.GetTunnels()[1].GetInputs()[0].GetReadAddress()->GetResourceIdentifier()
						== L"tcp://*:234");
		EXPECT_TRUE(	p.GetTunnels()[1].GetInputs()[0].GetWriteAddress()->GetResourceIdentifier()
						== L"tcp://zx:233");
		EXPECT_TRUE(	p.GetTunnels()[1].GetInputs()[0].GetUuid()
						== L"AC055577-E705-480a-BAE9-D15D522DB726");
		EXPECT_TRUE(p.GetTunnels()[1].GetInputs()[0].GetReadWriteAcceptor() == tex::Endpoint::ACCEPTOR_READER);
		ASSERT_TRUE(!p.GetTunnels()[1].GetInputs()[1].IsCombined());
		EXPECT_TRUE(	p.GetTunnels()[1].GetInputs()[1].GetReadAddress()->GetResourceIdentifier()
						== L"tcp://ttt:238");
		EXPECT_TRUE(	p.GetTunnels()[1].GetInputs()[1].GetWriteAddress()->GetResourceIdentifier()
						== L"tcp://zx:239");
		EXPECT_TRUE(	p.GetTunnels()[1].GetInputs()[1].GetUuid()
						== L"FC055577-E705-4803-BAE9-D15D522DB72F");
		EXPECT_TRUE(p.GetTunnels()[1].GetInputs()[1].GetReadWriteAcceptor() == tex::Endpoint::ACCEPTOR_NONE);

		// destinations endpoints number:
		ASSERT_TRUE(2 == p.GetTunnels()[0].GetDestinations().GetSize());
		ASSERT_TRUE(1 == p.GetTunnels()[0].GetDestinations()[0].GetPreListeners().GetSize());
		ASSERT_TRUE(1 == p.GetTunnels()[1].GetDestinations().GetSize());
		
		// destination endpoints:
		ASSERT_TRUE(p.GetTunnels()[0].GetDestinations()[0].IsCombined());
		EXPECT_TRUE(	p.GetTunnels()[0].GetDestinations()[0].GetCombinedAddress()->GetResourceIdentifier()
						== L"tcp://host-212.213.214.3-from-hosts:80");
		EXPECT_TRUE(	p.GetTunnels()[0].GetDestinations()[0].GetUuid()
						== L"AC055577-E705-480a-BAE9-D13D522DB726");
		ASSERT_TRUE(p.GetTunnels()[0].GetDestinations()[1].IsCombined());
		EXPECT_TRUE(	p.GetTunnels()[0].GetDestinations()[1].GetCombinedAddress()->GetResourceIdentifier()
						== L"tcp://host-212.213.214.6-from-hosts:86");
		EXPECT_TRUE(	p.GetTunnels()[0].GetDestinations()[1].GetUuid()
						== L"AC055577-E705-480a-BAE9-D14D522DB726");
		ASSERT_TRUE(!p.GetTunnels()[1].GetDestinations()[0].IsCombined());
		EXPECT_TRUE(	p.GetTunnels()[1].GetDestinations()[0].GetReadAddress()->GetResourceIdentifier()
						== L"tcp://212.213.214.5:80");
		EXPECT_TRUE(	p.GetTunnels()[1].GetDestinations()[0].GetWriteAddress()->GetResourceIdentifier()
						== L"tcp://212.213.214.9:81");
		EXPECT_TRUE(	p.GetTunnels()[1].GetDestinations()[0].GetUuid()
						== L"AC055577-E705-4809-BAE9-D16D522DB726");

		// filters:
		ASSERT_TRUE(2 == p.GetTunnels()[0].GetFilters().GetSize());
		ASSERT_TRUE(0 == p.GetTunnels()[1].GetFilters().GetSize());
		EXPECT_TRUE(tex::WString(L"some filter name") == p.GetTunnels()[0].GetFilters()[0]);
		EXPECT_TRUE(tex::WString(L"second filter name") == p.GetTunnels()[0].GetFilters()[1]);

		// listeners
		EXPECT_TRUE(	p.GetTunnels()[0].GetInputs()[1].GetPreListeners()[0].name
						== L"SomeListenerNumberOne");
		EXPECT_TRUE(	p.GetTunnels()[0].GetInputs()[1].GetPreListeners()[1].name
						== L"SomeListenerNumberOne/2");
		EXPECT_TRUE(	p.GetTunnels()[0].GetInputs()[1].GetPostListeners()[0].name
						== L"SomeListenerNumberOne/3");
		EXPECT_TRUE(	p.GetTunnels()[0].GetInputs()[1].GetPreListeners()[0].param
						== L"Some Listener Number One Param");
		EXPECT_TRUE(	p.GetTunnels()[0].GetInputs()[1].GetPreListeners()[1].param
						== L"Some Listener Number One Param / 2");
		EXPECT_TRUE(	p.GetTunnels()[0].GetInputs()[1].GetPostListeners()[0].param
						== L"Some Listener Number One Param / 3");
		EXPECT_TRUE(	p.GetTunnels()[0].GetDestinations()[0].GetPreListeners()[0].name
						== L"SomeListenerNumberTwo");
		EXPECT_TRUE(	p.GetTunnels()[0].GetDestinations()[0].GetPreListeners()[0].param
						== L"Some Listener Number Two Param");

	}

	TEST(Rule, XmlSave) {
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
			ASSERT_NO_THROW(const tex::RuleSet parseTest(xml));
		}
		boost::shared_ptr<const xml::XPath> xpath(
			xml::Document::LoadFromString(xml)->GetXPath());
		xml::ConstNodeCollection  queryResult;
		xpath->Query("/RuleSet/TunnelRule", queryResult);
		ASSERT_TRUE(2 == queryResult.size());
		tex::String strBuf;
		tex::WString wStrBuf;
		ASSERT_NO_THROW(queryResult[0]->GetAttribute("Name", wStrBuf));
		EXPECT_TRUE(wStrBuf == L"First rule");
		ASSERT_NO_THROW(queryResult[1]->GetAttribute("Name", wStrBuf));
		EXPECT_TRUE(	wStrBuf
						== L"��� ���� ��� ������� �� ������� ����� ������ ��������� ������ � UTF-8");
		ASSERT_NO_THROW(queryResult[0]->GetAttribute("ErrorsTreatment", wStrBuf));
		EXPECT_TRUE(wStrBuf == L"information");
		ASSERT_NO_THROW(queryResult[1]->GetAttribute("ErrorsTreatment", wStrBuf));
		EXPECT_TRUE(wStrBuf == L"warning");
		EXPECT_NO_THROW(queryResult[0]->GetAttribute("IsEnabled", wStrBuf));
		EXPECT_TRUE(wStrBuf == L"false");
		EXPECT_NO_THROW(queryResult[1]->GetAttribute("IsEnabled", wStrBuf));
		EXPECT_TRUE(wStrBuf == L"true");
		xpath->Query(
			"/RuleSet/TunnelRule[1]/FilterSet/Filter",
			queryResult);
		ASSERT_TRUE(2 == queryResult.size());
		EXPECT_TRUE(queryResult[0]->GetAttribute("Name", strBuf) == "first filter name - example");
		EXPECT_TRUE(queryResult[1]->GetAttribute("Name", strBuf) == "second filter name - example too");
		xpath->Query(
			"/RuleSet/TunnelRule[2]/FilterSet/Filter",
			queryResult);
		ASSERT_TRUE(0 == queryResult.size());
		xpath->Query(
			"/RuleSet/TunnelRule[1]/InputSet/Endpoint",
			queryResult);
		ASSERT_TRUE(2 == queryResult.size());
		xpath->Query(
			"/RuleSet/TunnelRule[1]/InputSet/Endpoint[1]/CombinedAddress",
			queryResult);
		ASSERT_TRUE(1 == queryResult.size());
		EXPECT_TRUE(	queryResult[0]->GetAttribute("ResourceIdentifier", strBuf)
						== "tcp://host-212.213.214.1-from-hosts:102");
		EXPECT_TRUE(queryResult[0]->GetAttribute("IsAcceptor", strBuf) == "true");

		xpath->Query(
			"/RuleSet/TunnelRule[1]/InputSet/Endpoint[2]/CombinedAddress",
			queryResult);
		ASSERT_TRUE(1 == queryResult.size());
		EXPECT_TRUE(	queryResult[0]->GetAttribute("ResourceIdentifier", strBuf)
						== "tcp://google.com:103");
		EXPECT_TRUE(queryResult[0]->GetAttribute("IsAcceptor", strBuf) == "false");

		xpath->Query(
			"/RuleSet/TunnelRule[1]/InputSet/Endpoint/PreListener",
			queryResult);
		ASSERT_TRUE(1 == queryResult.size());
		EXPECT_TRUE(queryResult[0]->GetAttribute("Name", strBuf) == "input/listener test");
		EXPECT_TRUE(queryResult[0]->GetContent(strBuf) == "input/listener parameter");
		
		xpath->Query(
			"/RuleSet/TunnelRule[1]/InputSet/Endpoint/PostListener",
			queryResult);
		ASSERT_TRUE(1 == queryResult.size());
		EXPECT_TRUE(queryResult[0]->GetAttribute("Name", strBuf) == "output/listener test");
		EXPECT_TRUE(queryResult[0]->GetContent(strBuf) == "output/listener parameter");

		xpath->Query("/RuleSet/TunnelRule[1]/DestinationSet/Endpoint", queryResult);
		ASSERT_TRUE(1 == queryResult.size());

		xpath->Query(
			"/RuleSet/TunnelRule[1]/DestinationSet/Endpoint[1]/CombinedAddress",
			queryResult);
		EXPECT_TRUE(	queryResult[0]->GetAttribute("ResourceIdentifier", strBuf)
						== "tcp://host-212.213.214.3-from-hosts:104");
		EXPECT_FALSE(queryResult[0]->HasAttribute("IsAccepter"));
		
		xpath->Query(
			"/RuleSet/TunnelRule[1]/DestinationSet/Endpoint/PreListener",
			queryResult);
		ASSERT_TRUE(1 == queryResult.size());
		EXPECT_TRUE(queryResult[0]->GetAttribute("Name", strBuf) == "input/listener test 2");
		EXPECT_TRUE(queryResult[0]->GetContent(strBuf) == "input/listener parameter 2");
		
		xpath->Query(
			"/RuleSet/TunnelRule[2]/InputSet/Endpoint",
			queryResult);
		ASSERT_TRUE(2 == queryResult.size());

		xpath->Query(
			"/RuleSet/TunnelRule[2]/InputSet/Endpoint/SplitAddress",
			queryResult);
		ASSERT_TRUE(2 == queryResult.size());
		EXPECT_TRUE(	queryResult[0]->GetAttribute("ReadResourceIdentifier", strBuf)
						== "tcp://*:105");
		EXPECT_TRUE(	queryResult[0]->GetAttribute("WriteResourceIdentifier", strBuf)
						== "tcp://xxx:106");
		EXPECT_TRUE(queryResult[0]->GetAttribute("Acceptor", strBuf) == "reader");
		EXPECT_TRUE(	queryResult[1]->GetAttribute("ReadResourceIdentifier", strBuf)
						== "tcp://xxx:107");
		EXPECT_TRUE(	queryResult[1]->GetAttribute("WriteResourceIdentifier", strBuf)
						== "tcp://xxx:108");
		EXPECT_TRUE(queryResult[1]->GetAttribute("Acceptor", strBuf) == "none");

		xpath->Query(
			"/RuleSet/TunnelRule[2]/DestinationSet/Endpoint",
			queryResult);
		ASSERT_TRUE(2 == queryResult.size());

		xpath->Query(
			"/RuleSet/TunnelRule[2]/DestinationSet/Endpoint/SplitAddress",
			queryResult);
		ASSERT_TRUE(1 == queryResult.size());
		EXPECT_TRUE(	queryResult[0]->GetAttribute("ReadResourceIdentifier", strBuf)
						== "tcp://host-212.213.214.5-from-hosts:106");
		EXPECT_TRUE(	queryResult[0]->GetAttribute("WriteResourceIdentifier", strBuf)
						== "tcp://host-212.213.214.5-from-hosts:108");
		EXPECT_FALSE(queryResult[0]->HasAttribute("Acceptor"));

		xpath->Query(
			"/RuleSet/TunnelRule[2]/DestinationSet/Endpoint/CombinedAddress",
			queryResult);
		ASSERT_TRUE(1 == queryResult.size());
		EXPECT_TRUE(	queryResult[0]->GetAttribute("ResourceIdentifier", strBuf)
						== "tcp://host-212.213.214.6-from-hosts:116");
		EXPECT_FALSE(queryResult[0]->HasAttribute("Acceptor"));

	}

	// Will work after testing mode for licensing
	/* BOOST_AUTO_TEST_CASE(Enabling) {
	
		struct RuleAppender : private boost::noncopyable {
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
			EXPECT_TRUE(tex::Server::GetInstance().GetOpenedEndpointsNumber() == 2);
			tex::Server::GetInstance().Stop();
		}

	} */

	TEST(Rule, EndpointWithProxy) {
		
		using namespace TunnelEx::Mods::Inet;
		
		TcpEndpointAddress address(
			L"destination_host:987?proxy=http://no_auth_proxy_host:376&proxy=http://asDAsdceed:sdfuweWderWf@auth_ptoxy_host:10&proxy=http://uuu@test:1");
		
		EXPECT_TRUE(address.GetHostName() == L"destination_host");
		EXPECT_TRUE(address.GetPort() == 987);

		ASSERT_TRUE(address.GetProxyList().size() == 3);

		std::vector<TunnelEx::Mods::Inet::Proxy> proxyList(
			address.GetProxyList().begin(), address.GetProxyList().end());
		
		EXPECT_TRUE(proxyList[0].host == L"no_auth_proxy_host");
		EXPECT_TRUE(proxyList[0].port == 376);
		EXPECT_TRUE(proxyList[0].user.empty());
		EXPECT_TRUE(proxyList[0].password.empty());

		EXPECT_TRUE(proxyList[1].host == L"auth_ptoxy_host");
		EXPECT_TRUE(proxyList[1].port == 10);
		EXPECT_TRUE(proxyList[1].user == L"asDAsdceed");
		EXPECT_TRUE(proxyList[1].password == L"sdfuweWderWf");

		EXPECT_TRUE(proxyList[2].host == L"test");
		EXPECT_TRUE(proxyList[2].port == 1);
		EXPECT_TRUE(proxyList[2].user == L"uuu");
		EXPECT_TRUE(proxyList[2].password.empty());

	}

}

