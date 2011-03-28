/**************************************************************************
 *   Created: 2008/06/06 23:13
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Rules.cpp 1017 2010-09-27 01:24:30Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "Rules.h"
#include "ServiceControl/Configuration.hpp"

#include <TunnelEx/Rule.hpp>
#include <TunnelEx/Log.hpp>

using namespace std;
using namespace boost;
using namespace TunnelEx;
using namespace TunnelEx::Helpers::Xml;

//////////////////////////////////////////////////////////////////////////

class RulesXmlParserForVer1_0 : private noncopyable {

public:

	RulesXmlParserForVer1_0()
			: m_addressExpressions(L"(([^*:]+)|(\\*)):(\\d+)") {
		//...//
	}

	void Parse(
				ServiceRuleSet &rulesCollection,
				const Document &)
			const {
		ServiceRuleSet().Swap(rulesCollection);
	}

	void Parse(
				TunnelRuleSet &rulesCollection,
				const Document &doc)
			const {
		for (	shared_ptr<const Node> ruleNode(doc.GetRoot()->GetChildElement());
				ruleNode;
				ruleNode = ruleNode->GetNextElement()) {
			try {
				rulesCollection.Append(*ParseRule(ruleNode));
			} catch (const ::std::logic_error &ex) {
				string message;
				if (ruleNode->HasAttribute("Uuid")) {
					string buffer;
					message
						= (Format(
								"Legacy version supports (rules migration): "
									"rule with UUID \"%1%\" could not be migrated"
									"with error \"%2%\".")
							% ruleNode->GetAttribute("Uuid", buffer)
							% ex.what()
						).str();
					
				} else {
					message
						= (Format(
								"Legacy version supports (rules migration): "
									"some rule with unknown UUID could not be migrated "
									"with error \"%1%\".")
							% ex.what()
						).str();
				}
				Log::GetInstance().AppendError(message);
			}
		}
	}


private:

	wregex m_addressExpressions;

protected:

	RuleEndpoint ParseEndpoint(shared_ptr<const Node> endpointNode, bool isInput) const {
		wstring address;
		wsmatch what;
		regex_match(
			endpointNode->GetAttribute("Address", address),
			what,
			m_addressExpressions);
		unsigned short port;
		try {
			port = lexical_cast<unsigned short>(what[4].str());
		} catch (const bad_lexical_cast&) {
			port = numeric_limits<unsigned short>::max();
			if (Log::GetInstance().IsWarnsRegistrationOn()) {
				Format message(	"Legacy version supports (rules migration):"
								" Network port value truncated from \"%1%\" to \"%2%\".");
				message
					% ConvertString<String>(what[4].str().c_str()).GetCStr()
					% port;
				Log::GetInstance().AppendWarn(message.str());
			}
		}
		WString buffer;
		WFormat endpointAddress(L"tcp://%1%:%2%");
		endpointAddress % (!what[2].matched ? L"*" : what[1].str());
		endpointAddress % port;
		RuleEndpoint endpoint(
			endpointAddress.str().c_str(),
			isInput,
			&endpointNode->GetAttribute("Uuid", buffer));
		for (	shared_ptr<const Node> node = endpointNode->GetChildElement();
				node.get();
				node = node->GetNextElement()) {
			RuleEndpoint::ListenerInfo listener;
			listener.name = node->GetAttribute("Name", buffer);
			listener.param = node->GetContent(buffer);
			if (node->GetAttribute("Stream", buffer) == L"in") {
				if (listener.name == L"Forwarder/Ftp/Active") {
					listener.name = L"Tunnel/Ftp/Passive";
				} else if (listener.name == L"Forwarder/Ftp/Passive") {
					listener.name = L"Tunnel/Ftp/Active";
				}
				endpoint.GetPreListeners().Append(listener);
			} else {
				endpoint.GetPostListeners().Append(listener);
			}
		}
		return endpoint;
	}

	void ParseEnpoints(
				shared_ptr<const Node> node,
				RuleEndpointCollection &result,
				bool isInput)
			const {
		RuleEndpointCollection tmp;
		for (node = node->GetChildElement(); node; node = node->GetNextElement()) {
			tmp.Append(ParseEndpoint(node, isInput));
		}
		tmp.Swap(result);
	}

	void ParseFilters(shared_ptr<const Node> node, TunnelRule::Filters &result) const {
		TunnelRule::Filters tmp;
		wstring buffer;
		for (node = node->GetChildElement(); node; node = node->GetNextElement()) {
			tmp.Append(node->GetAttribute("Name", buffer).c_str());
		}
		tmp.Swap(result);
	}

	shared_ptr<TunnelRule> ParseRule(shared_ptr<const Node> node) const {
		WString buffer;
		shared_ptr<TunnelRule> rule(new TunnelRule(node->GetAttribute("Uuid", buffer)));
		rule->SetName(node->GetAttribute("Name", buffer));
		node = node->GetChildElement();
		{
			TunnelRule::Filters filters;
			ParseFilters(node, filters);
			rule->SetFilters(filters);
		}
		node = node->GetNextElement();
		RuleEndpointCollection endpoints;
		ParseEnpoints(node, endpoints ,true);
		rule->SetInputs(endpoints);
		node = node->GetNextElement();
		ParseEnpoints(node, endpoints, false);
		rule->SetDestinations(endpoints);
		return rule;
	}

};

//////////////////////////////////////////////////////////////////////////

class RulesXmlParserForVer1_1 : private noncopyable {

public:

	void Parse(
				ServiceRuleSet &rulesCollection,
				const Document &)
			const {
		ServiceRuleSet().Swap(rulesCollection);
	}

	void Parse(
				TunnelRuleSet &rulesCollection,
				const Document &doc)
			const {
		for (	shared_ptr<const Node> ruleNode = doc.GetRoot()->GetChildElement();
				ruleNode;
				ruleNode = ruleNode->GetNextElement()) {
			rulesCollection.Append(*ParseRule(ruleNode));
		}
	}

protected:

	RuleEndpoint ParseEndpoint(
				shared_ptr<const Node> endpointNode,
				bool isInput)
			const {
		WString wbuffer;
		WString wbuffer2;
		endpointNode->GetAttribute("ResourceIdentifier", wbuffer);
		RuleEndpoint endpoint(
			wbuffer,
			isInput && !boost::istarts_with(wbuffer.GetCStr(), L"udp://"),
			&endpointNode->GetAttribute("Uuid", wbuffer2));
		for (	shared_ptr<const Node> node = endpointNode->GetChildElement();
				node;
				node = node->GetNextElement()) {
			RuleEndpoint::ListenerInfo listener;
			listener.name = node->GetAttribute("Name", wbuffer);
			if (listener.name == L"Forwarder/Ftp/Active") {
				listener.name = L"Tunnel/Ftp/Active";
			} else if (listener.name == L"Forwarder/Ftp/Passive") {
				listener.name = L"Tunnel/Ftp/Passive";
			}
			listener.param = node->GetContent(wbuffer);
			if (node->GetName(wbuffer) == L"PreListener") {
				endpoint.GetPreListeners().Append(listener);
			} else { // L"PostListener"
				endpoint.GetPostListeners().Append(listener);
			}
		}
		return endpoint;
	}
	
	void ParseEnpoints(
				shared_ptr<const Node> node,
				RuleEndpointCollection &result,
				bool isInput)
			const {
		RuleEndpointCollection tmp;
		for (node = node->GetChildElement(); node; node = node->GetNextElement()) {
			tmp.Append(ParseEndpoint(node, isInput));
		}
		tmp.Swap(result);
	}

	void ParseFilters(
				shared_ptr<const Node> node,
				TunnelRule::Filters &result)
			const {
		TunnelRule::Filters tmp;
		wstring buffer;
		for (node = node->GetChildElement(); node; node = node->GetNextElement()) {
			tmp.Append(node->GetAttribute("Name", buffer).c_str());
		}
		tmp.Swap(result);
	}

	shared_ptr<TunnelRule> ParseRule(shared_ptr<const Node> node) const {
		WString buffer;
		shared_ptr<TunnelRule> rule(new TunnelRule(node->GetAttribute("Uuid", buffer)));
		rule->SetName(node->GetAttribute("Name", buffer));
		node = node->GetChildElement();
		{
			TunnelRule::Filters filters;
			ParseFilters(node, filters);
			rule->SetFilters(filters);
		}
		node = node->GetNextElement();
		RuleEndpointCollection endpoints;
		ParseEnpoints(node, endpoints, true);
		rule->SetInputs(endpoints);
		node = node->GetNextElement();
		ParseEnpoints(node, endpoints, true);
		rule->SetDestinations(endpoints);
		return rule;
	}

};

//////////////////////////////////////////////////////////////////////////

class RulesXmlParserForVer1_2 : private noncopyable {

public:

	void Parse(
				ServiceRuleSet &rulesCollection,
				const Document &)
			const {
		ServiceRuleSet().Swap(rulesCollection);
	}

	void Parse(
				TunnelRuleSet &rulesCollection,
				const Document &doc)
			const {
		for (	shared_ptr<const Node> ruleNode = doc.GetRoot()->GetChildElement();
				ruleNode;
				ruleNode = ruleNode->GetNextElement()) {
			rulesCollection.Append(*ParseRule(ruleNode));
		}
	}

protected:

	RuleEndpoint ParseEndpoint(
				shared_ptr<const Node> endpointNode,
				bool isInput)
			const {
		WString wbuffer;
		WString wbuffer2;
		endpointNode->GetAttribute("ResourceIdentifier", wbuffer);
		RuleEndpoint endpoint(
			wbuffer,
			isInput && !istarts_with(wbuffer.GetCStr(), L"udp://"),
			&endpointNode->GetAttribute("Uuid", wbuffer2));
		for (	shared_ptr<const Node> node = endpointNode->GetChildElement();
				node;
				node = node->GetNextElement()) {
			RuleEndpoint::ListenerInfo listener;
			listener.name = node->GetAttribute("Name", wbuffer);
			if (listener.name == L"Forwarder/Ftp/Active") {
				listener.name = L"Tunnel/Ftp/Active";
			} else if (listener.name == L"Forwarder/Ftp/Passive") {
				listener.name = L"Tunnel/Ftp/Passive";
			}
			listener.param = node->GetContent(wbuffer);
			if (node->GetName(wbuffer) == L"PreListener") {
				endpoint.GetPreListeners().Append(listener);
			} else { // L"PostListener"
				endpoint.GetPostListeners().Append(listener);
			}
		}
		return endpoint;
	}
	
	void ParseEnpoints(
				shared_ptr<const Node> node,
				RuleEndpointCollection &result,
				bool isInput)
			const {
		RuleEndpointCollection tmp;
		for (	node = node->GetChildElement();
				node;
				node = node->GetNextElement()) {
			tmp.Append(ParseEndpoint(node, isInput));
		}
		tmp.Swap(result);
	}

	void ParseFilters(
				shared_ptr<const Node> node,
				TunnelRule::Filters &result)
			const {
		TunnelRule::Filters tmp;
		wstring buffer;
		for (	node = node->GetChildElement();
				node;
				node = node->GetNextElement()) {
			tmp.Append(node->GetAttribute("Name", buffer).c_str());
		}
		tmp.Swap(result);
	}

	TunnelRule::ErrorsTreatment ParseRuleErrorsTreatment(
				shared_ptr<const Node> node)
			const {
		string errorsTreatment;
		node->GetAttribute("ErrorsTreatment", errorsTreatment);
		if (errorsTreatment == "information") {
			return TunnelRule::ERRORS_TREATMENT_INFO;
		} else if (errorsTreatment == "warning") {
			return TunnelRule::ERRORS_TREATMENT_WARN;
		} else {
			BOOST_ASSERT(errorsTreatment == "error");
			return TunnelRule::ERRORS_TREATMENT_ERROR;
		}
	}

	shared_ptr<TunnelRule> ParseRule(shared_ptr<const Node> node) const {
		WString buffer;
		shared_ptr<TunnelRule> rule(new TunnelRule(node->GetAttribute("Uuid", buffer)));
		rule->SetName(node->GetAttribute("Name", buffer));
		rule->SetErrorsTreatment(ParseRuleErrorsTreatment(node));
		rule->Enable(node->GetAttribute("IsEnabled", buffer) == L"true");
		node = node->GetChildElement();
		{
			TunnelRule::Filters filters;
			ParseFilters(node, filters);
			rule->SetFilters(filters);
		}
		node = node->GetNextElement();
		RuleEndpointCollection endpoints;
		ParseEnpoints(node, endpoints, true);
		rule->SetInputs(endpoints);
		node = node->GetNextElement();
		ParseEnpoints(node, endpoints, false);
		rule->SetDestinations(endpoints);
		return rule;
	}

};

//////////////////////////////////////////////////////////////////////////

class RulesXmlParserForVer1_3 : private noncopyable {

public:

	RulesXmlParserForVer1_3() {
		//...//
	}

	void Parse(
				ServiceRuleSet &rulesCollection,
				const Document &)
			const {
		ServiceRuleSet().Swap(rulesCollection);
	}

	void Parse(
				TunnelRuleSet &rulesCollection,
				const Document &doc)
			const {
		for (	shared_ptr<const Node> ruleNode = doc.GetRoot()->GetChildElement();
				ruleNode;
				ruleNode = ruleNode->GetNextElement()) {
			rulesCollection.Append(*ParseRule(ruleNode));
		}
	}

protected:

	RuleEndpoint::ListenerInfo ParseListiner(const Node &node) const {
		RuleEndpoint::ListenerInfo result;
		WString wbuffer;
		result.name = node.GetAttribute("Name", wbuffer);
		result.param = node.GetContent(wbuffer);
		return result;
	}

	RuleEndpoint ParseInputEndpoint(const Node &endpointNode) const {
		WString wbuffer;
		WString wbuffer2;
		RuleEndpoint endpoint(&endpointNode.GetAttribute("Uuid", wbuffer2));
		for (	shared_ptr<const Node> node = endpointNode.GetChildElement();
				node;
				node = node->GetNextElement()) {
			node->GetName(wbuffer);
			if (wbuffer == L"CombinedAddress") {
				endpoint.SetCombinedResourceIdentifier(
					node->GetAttribute("ResourceIdentifier", wbuffer),
					node->GetAttribute("IsAcceptor", wbuffer) == L"true");
			} else if (wbuffer == L"SplitAddress") {
				node->GetAttribute("Acceptor", wbuffer);
				Endpoint::Acceptor acceptor = Endpoint::ACCEPTOR_NONE; // wbuffer == L"none"
				if (wbuffer == L"reader") {
					acceptor = Endpoint::ACCEPTOR_READER;
				} else if (wbuffer == L"writer") {
					acceptor = Endpoint::ACCEPTOR_WRITER;
				}
				endpoint.SetReadWriteResourceIdentifiers(
					node->GetAttribute("ReadResourceIdentifier", wbuffer),
					node->GetAttribute("WriteResourceIdentifier", wbuffer2),
					acceptor);
			} else if (wbuffer == L"PreListener") {
				endpoint.GetPreListeners().Append(ParseListiner(*node));
			} else { // L"PostListener"
				endpoint.GetPostListeners().Append(ParseListiner(*node));
			}
		}
		return endpoint;
	}

	RuleEndpoint ParseDestinationEndpoint(const Node &endpointNode) const {
		WString wbuffer;
		WString wbuffer2;
		RuleEndpoint endpoint(&endpointNode.GetAttribute("Uuid", wbuffer2));
		for (	shared_ptr<const Node> node = endpointNode.GetChildElement();
				node;
				node = node->GetNextElement()) {
			node->GetName(wbuffer);
			if (wbuffer == L"CombinedAddress") {
				endpoint.SetCombinedResourceIdentifier(
					node->GetAttribute("ResourceIdentifier", wbuffer),
					false);
			} else if (wbuffer == L"SplitAddress") {
				endpoint.SetReadWriteResourceIdentifiers(
					node->GetAttribute("ReadResourceIdentifier", wbuffer),
					node->GetAttribute("WriteResourceIdentifier", wbuffer2),
					Endpoint::ACCEPTOR_NONE);
			} else if (wbuffer == L"PreListener") {
				endpoint.GetPreListeners().Append(ParseListiner(*node));
			} else { // L"PostListener"
				endpoint.GetPostListeners().Append(ParseListiner(*node));
			}
		}
		return endpoint;
	}
	
	void ParseInputEnpoints(
				shared_ptr<const Node> node,
				RuleEndpointCollection &result)
			const {
		RuleEndpointCollection tmp;
		for (node = node->GetChildElement(); node; node = node->GetNextElement()) {
			tmp.Append(ParseInputEndpoint(*node));
		}
		tmp.Swap(result);
	}

	void ParseDestinationEndpoints(
				shared_ptr<const Node> node,
				RuleEndpointCollection &result)
			const {
		RuleEndpointCollection tmp;
		for (node = node->GetChildElement(); node; node = node->GetNextElement()) {
			tmp.Append(ParseDestinationEndpoint(*node));
		}
		tmp.Swap(result);
	}

	void ParseFilters(
				shared_ptr<const Node> node,
				TunnelRule::Filters &result)
			const {
		TunnelRule::Filters tmp;
		wstring buffer;
		for (node = node->GetChildElement(); node; node = node->GetNextElement()) {
			tmp.Append(node->GetAttribute("Name", buffer).c_str());
		}
		tmp.Swap(result);
	}

	Rule::ErrorsTreatment ParseRuleErrorsTreatment(
				shared_ptr<const Node> node)
			const {
		string errorsTreatment;
		node->GetAttribute("ErrorsTreatment", errorsTreatment);
		if (errorsTreatment == "information") {
			return Rule::ERRORS_TREATMENT_INFO;
		} else if (errorsTreatment == "warning") {
			return Rule::ERRORS_TREATMENT_WARN;
		} else {
			BOOST_ASSERT(errorsTreatment == "error");
			return Rule::ERRORS_TREATMENT_ERROR;
		}
	}

	shared_ptr<TunnelRule> ParseRule(shared_ptr<const Node> node) const {
		WString buffer;
		shared_ptr<TunnelRule> rule(new TunnelRule(node->GetAttribute("Uuid", buffer)));
		rule->SetName(node->GetAttribute("Name", buffer));
		rule->SetErrorsTreatment(ParseRuleErrorsTreatment(node));
		rule->Enable(node->GetAttribute("IsEnabled", buffer) == L"true");
		node = node->GetChildElement();
		{
			TunnelRule::Filters filters;
			ParseFilters(node, filters);
			rule->SetFilters(filters);
		}
		node = node->GetNextElement();
		RuleEndpointCollection endpoints;
		ParseInputEnpoints(node, endpoints);
		rule->SetInputs(endpoints);
		node = node->GetNextElement();
		ParseDestinationEndpoints(node, endpoints);
		rule->SetDestinations(endpoints);
		return rule;
	}

};


//////////////////////////////////////////////////////////////////////////

class RulesXmlParserForVer2_0 : private noncopyable {

public:

	class RuleEntitySetXmlParser : private noncopyable {

	protected:

		Rule::ErrorsTreatment ParseErrorsTreatment(
					shared_ptr<const Node> node)
				const {
			string errorsTreatment;
			node->GetAttribute("ErrorsTreatment", errorsTreatment);
			if (errorsTreatment == "information") {
				return Rule::ERRORS_TREATMENT_INFO;
			} else if (errorsTreatment == "warning") {
				return Rule::ERRORS_TREATMENT_WARN;
			} else {
				BOOST_ASSERT(errorsTreatment == "error");
				return Rule::ERRORS_TREATMENT_ERROR;
			}
		}

		template<class Entity>
		shared_ptr<Entity> ParseEntity(shared_ptr<const Node> node) const {
			WString buffer;
			shared_ptr<Entity> entity(new Entity(node->GetAttribute("Uuid", buffer)));
			entity->SetName(node->GetAttribute("Name", buffer));
			entity->SetErrorsTreatment(ParseErrorsTreatment(node));
			entity->Enable(node->GetAttribute("IsEnabled", buffer) == L"true");
			return entity;
		}

		template<class RuleSet>
		void Parse(
					const Document &doc,
					const string &tagName,
					function<shared_ptr<typename RuleSet::ItemType>(shared_ptr<const Node>)> entityParser,
					RuleSet &result)
				const {
			ConstNodeCollection nodes;
			const string path = "/RuleSet/" + tagName;
			doc.GetXPath()->Query(path.c_str(), nodes);
			typename RuleSet set(nodes.size());
			foreach (shared_ptr<const Node> &node, nodes) {
				set.Append(*entityParser(node));
			}
			result.Swap(set);
		}

	};

private:

	class TunnelRuleSetXmlParser : private RuleEntitySetXmlParser {

	public:

		void Parse(const Document &doc, TunnelRuleSet &result) const {
			RuleEntitySetXmlParser::Parse(
				doc,
				"TunnelRule",
				bind(&TunnelRuleSetXmlParser::ParseRule, this, _1),
				result);
		}

	protected:

		RuleEndpoint::ListenerInfo ParseListiner(const Node &node) const {
			RuleEndpoint::ListenerInfo result;
			WString wbuffer;
			result.name = node.GetAttribute("Name", wbuffer);
			result.param = node.GetContent(wbuffer);
			return result;
		}

		RuleEndpoint ParseInputEndpoint(const Node &endpointNode) const {
			WString wbuffer;
			WString wbuffer2;
			RuleEndpoint endpoint(&endpointNode.GetAttribute("Uuid", wbuffer2));
			for (	shared_ptr<const Node> node = endpointNode.GetChildElement();
					node;
					node = node->GetNextElement()) {
				node->GetName(wbuffer);
				if (wbuffer == L"CombinedAddress") {
					const bool isUdp = istarts_with(
						node->GetAttribute("ResourceIdentifier", wbuffer).GetCStr(),
						L"udp:");
					endpoint.SetCombinedResourceIdentifier(
						node->GetAttribute("ResourceIdentifier", wbuffer),
						isUdp || node->GetAttribute("IsAcceptor", wbuffer2) == L"true");
				} else if (wbuffer == L"SplitAddress") {
					node->GetAttribute("Acceptor", wbuffer);
					Endpoint::Acceptor acceptor = Endpoint::ACCEPTOR_NONE; // wbuffer == L"none"
					if (	wbuffer == L"reader"
							|| istarts_with(node->GetAttribute("ReadResourceIdentifier", wbuffer2).GetCStr(), L"udp:")) {
						acceptor = Endpoint::ACCEPTOR_READER;
					} else if (wbuffer == L"writer") {
						acceptor = Endpoint::ACCEPTOR_WRITER;
					}
					endpoint.SetReadWriteResourceIdentifiers(
						node->GetAttribute("ReadResourceIdentifier", wbuffer),
						node->GetAttribute("WriteResourceIdentifier", wbuffer2),
						acceptor);
				} else if (wbuffer == L"PreListener") {
					endpoint.GetPreListeners().Append(ParseListiner(*node));
				} else { // L"PostListener"
					endpoint.GetPostListeners().Append(ParseListiner(*node));
				}
			}
			return endpoint;
		}

		RuleEndpoint ParseDestinationEndpoint(const Node &endpointNode) const {
			WString wbuffer;
			WString wbuffer2;
			RuleEndpoint endpoint(&endpointNode.GetAttribute("Uuid", wbuffer2));
			for (	shared_ptr<const Node> node = endpointNode.GetChildElement();
					node;
					node = node->GetNextElement()) {
				node->GetName(wbuffer);
				if (wbuffer == L"CombinedAddress") {
					endpoint.SetCombinedResourceIdentifier(
						node->GetAttribute("ResourceIdentifier", wbuffer),
						false);
				} else if (wbuffer == L"SplitAddress") {
					endpoint.SetReadWriteResourceIdentifiers(
						node->GetAttribute("ReadResourceIdentifier", wbuffer),
						node->GetAttribute("WriteResourceIdentifier", wbuffer2),
						Endpoint::ACCEPTOR_NONE);
				} else if (wbuffer == L"PreListener") {
					endpoint.GetPreListeners().Append(ParseListiner(*node));
				} else { // L"PostListener"
					endpoint.GetPostListeners().Append(ParseListiner(*node));
				}
			}
			return endpoint;
		}

		void ParseInputEnpoints(
					shared_ptr<const Node> node,
					RuleEndpointCollection &result)
				const {
			RuleEndpointCollection set;
			for (node = node->GetChildElement(); node; node = node->GetNextElement()) {
				set.Append(ParseInputEndpoint(*node));
			}
			set.Swap(result);
		}

		void ParseDestinationEndpoints(
					shared_ptr<const Node> node,
					RuleEndpointCollection &result)
				const {
			RuleEndpointCollection set;
			for (node = node->GetChildElement(); node; node = node->GetNextElement()) {
				set.Append(ParseDestinationEndpoint(*node));
			}
			set.Swap(result);
		}

		void ParseFilters(
					shared_ptr<const Node> node,
					TunnelRule::Filters &result)
				const {
			TunnelRule::Filters set;
			wstring buffer;
			for (node = node->GetChildElement(); node; node = node->GetNextElement()) {
				set.Append(node->GetAttribute("Name", buffer).c_str());
			}
			set.Swap(result);
		}

		shared_ptr<TunnelRule> ParseRule(shared_ptr<const Node> node) const {
			shared_ptr<TunnelRule> rule = ParseEntity<TunnelRule>(node);
			node = node->GetChildElement();
			{
				TunnelRule::Filters filters;
				ParseFilters(node, filters);
				rule->SetFilters(filters);
			}
			node = node->GetNextElement();
			RuleEndpointCollection endpoints;
			ParseInputEnpoints(node, endpoints);
			rule->SetInputs(endpoints);
			node = node->GetNextElement();
			ParseDestinationEndpoints(node, endpoints);
			rule->SetDestinations(endpoints);
			return rule;
		}

	};

public:

	class ServiceRuleSetXmlParser : private RuleEntitySetXmlParser {

	public:

		void Parse(const Document &doc, ServiceRuleSet &result) const {
			RuleEntitySetXmlParser::Parse(
				doc,
				"ServiceRule",
				bind(&ServiceRuleSetXmlParser::ParseRule, this, _1),
				result);
		}

	protected:

		shared_ptr<ServiceRule> ParseRule(shared_ptr<const Node> node) const {
			shared_ptr<ServiceRule> rule = ParseEntity<ServiceRule>(node);
			ServiceRule::ServiceSet services;
			for (node = node->GetChildElement(); node; node = node->GetNextElement()) {
				services.Append(ParseService(*node));
			}
			rule->SetServices(services);
			return rule;
		}

		ServiceRule::Service ParseService(const Node &node) const {
			ServiceRule::Service result;
			node.GetAttribute("Uuid", result.uuid);
			node.GetAttribute("Name", result.name);
			node.GetContent(result.param);
			return result;
		}

	};

public:

	void Parse(ServiceRuleSet &rulesCollection, const Document &doc) const {
		ServiceRuleSetXmlParser().Parse(doc, rulesCollection);
	}

	void Parse(TunnelRuleSet &rulesCollection, const Document &doc) const {
		TunnelRuleSetXmlParser().Parse(doc, rulesCollection);
	}

};


//////////////////////////////////////////////////////////////////////////

//! Migrates rules XML in format X to  current. Throws an exception on error.
/** @param	source	the DOM-document, rules in format 1.0.
 *	@return		DOM-document, rules in current format.
 */
template<class Migrator>
shared_ptr<Document> MigrateRulesFrom(const Document &source) {
	ServiceRuleSet serviceRuleSet;
	TunnelRuleSet tunnelRuleSet;
	try {
		Migrator migrator;
		migrator.Parse(serviceRuleSet, source);
		migrator.Parse(tunnelRuleSet, source);
	} catch (const ::TunnelEx::Helpers::Xml::Exception &ex) {
		Format message(
			"Legacy version supports: "
				"rules migration could not be completed with error \"%1%\".");
		message % ex.what();
		Log::GetInstance().AppendError(message.str());
		throw LocalException(L"Could not migrate rules.");
	}
	UString xml;
	RuleSet parser(serviceRuleSet, tunnelRuleSet);
	parser.GetXml(xml);
	return shared_ptr<Document>(Document::LoadFromString(xml));
}

//////////////////////////////////////////////////////////////////////////

bool MigrateCurrentRuleSet(RuleSet &result) {

	Log::GetInstance().AppendInfo("Starting rule set migration...");
	Log::GetInstance().AppendInfo("Opening current rule set file...");

	wifstream rulesFile;
	try {
		rulesFile.open(
			ServiceConfiguration().GetRulesPath().c_str(),
			ios::binary | ios::in);
	} catch (const ServiceConfiguration::ConfigurationException &) {
		Log::GetInstance().AppendFatalError(
			"Could not get rule set file path (service configuration getting error).");
		RuleSet().Swap(result);
		return true;
	}

	if (!rulesFile) {
		Log::GetInstance().AppendInfo("File not found, migration stopped.");
		RuleSet().Swap(result);
		return true;
	}

	WString rulesXml;
	{
		wostringstream rulesXmlStream;
		rulesXmlStream << rulesFile.rdbuf();
		rulesXml = rulesXmlStream.str().c_str();
	}

	try {
		RuleSet ruleSet(rulesXml);
		Log::GetInstance().AppendInfo("Migration is not required, migration stopped.");
		ruleSet.Swap(result);
		return false;
	} catch (const TunnelEx::LocalException &) {
		//...//
	}

	MigrateRuleSet(rulesXml, result);
	Log::GetInstance().AppendInfo("Rule set migration finished.");
	return true;

}

void MigrateRuleSet(const WString &rulesXml, RuleSet &result) {

	shared_ptr<Document> oldDoc;
	try {
		oldDoc = Document::LoadFromString(rulesXml);
	} catch (const Document::ParseException &) {
		Log::GetInstance().AppendWarn("File has invalid format, migration stopped.");
		RuleSet().Swap(result);
	}

	RuleSet ruleSet;
	try {
		shared_ptr<const XPath> xpath(oldDoc->GetXPath());
		ConstNodeCollection  queryResult;
		xpath->Query("/RuleSet", queryResult);
		shared_ptr<Document> newDoc;
		WString buffer;
		if (queryResult.size() == 1 && queryResult[0]->HasAttribute("Version")) {
			if (queryResult[0]->GetAttribute("Version", buffer) == L"1.2") {
				newDoc = MigrateRulesFrom<RulesXmlParserForVer1_2>(*oldDoc);
			} else if (queryResult[0]->GetAttribute("Version", buffer) == L"1.3") {
				newDoc = MigrateRulesFrom<RulesXmlParserForVer1_3>(*oldDoc);
			} else if (queryResult[0]->GetAttribute("Version", buffer) == L"2.0") {
				newDoc = MigrateRulesFrom<RulesXmlParserForVer2_0>(*oldDoc);
			}
			// for 2.1 use RulesXmlParserForVer2_0 sub-classes
			// (instead tunnel endpoint parse) - data format not changed.
		} else if (!queryResult.size()) {
			xpath->Query("/RuleCollection", queryResult);
			if (queryResult.size() == 1 && queryResult[0]->HasAttribute("Version")) {
				if (queryResult[0]->GetAttribute("Version", buffer) == L"1.0") {
					newDoc = MigrateRulesFrom<RulesXmlParserForVer1_0>(*oldDoc);
				} else if (queryResult[0]->GetAttribute("Version", buffer) == L"1.1") {
					newDoc = MigrateRulesFrom<RulesXmlParserForVer1_1>(*oldDoc);
				}
			}
		}
		if (newDoc) {
			newDoc->Dump(buffer);
			RuleSet(buffer).Swap(ruleSet);
		} else {
			Log::GetInstance().AppendWarn("Unknown version, migration failed...");
		}
	} catch (const TunnelEx::LocalException &) {
		Log::GetInstance().AppendWarn("Migration failed...");
	}

	ruleSet.Swap(result);

}
