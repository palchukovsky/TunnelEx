/**************************************************************************
 *   Created: 2010/05/15 16:30
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "Rule.hpp"
#include "EndpointAddress.hpp"
#include "Log.hpp"

using namespace TunnelEx;
namespace fs = boost::filesystem;

//////////////////////////////////////////////////////////////////////////

class Rule::Implementation {

public:

	explicit Implementation(const WString &uuid)
			: m_uuid(uuid),
			m_errorsTreatment(Rule::ERRORS_TREATMENT_ERROR),
			m_isEnabled(true),
			m_isSilent(false) {
		//...//
	}

public:

	WString m_uuid;
	WString m_name;
	Rule::ErrorsTreatment m_errorsTreatment;
	bool m_isEnabled;
	bool m_isSilent;

};

//////////////////////////////////////////////////////////////////////////

Rule::Rule()
		: m_pimpl(new Implementation(Helpers::Uuid().GetAsString().c_str())) {
	//...//
}

Rule::Rule(const WString &uuid)
		: m_pimpl(new Implementation(uuid)) {
	//...//
}

Rule::Rule(const Rule &rhs)
		: m_pimpl(new Implementation(*rhs.m_pimpl)) {
	//...//
}

Rule::~Rule() {
	delete m_pimpl;
}

const Rule & Rule::operator =(const Rule &rhs) {
	Rule newTmp(rhs);
	Swap(newTmp);
	return *this;
}

void Rule::Swap(Rule &rhs) throw() {
	std::swap(m_pimpl, rhs.m_pimpl);
}

const WString & Rule::GetUuid() const {
	return m_pimpl->m_uuid;
}

void Rule::SetUuid(const WString &uuid) {
	m_pimpl->m_uuid = uuid;
}

const WString & Rule::GetName() const {
	return m_pimpl->m_name;
}

void Rule::SetName(const WString &name) {
	m_pimpl->m_name = name;
}

Rule::ErrorsTreatment Rule::GetErrorsTreatment() const {
	return m_pimpl->m_errorsTreatment;
}

void Rule::SetErrorsTreatment(Rule::ErrorsTreatment newTreatment) {
	m_pimpl->m_errorsTreatment = newTreatment;
}

bool Rule::IsEnabled() const {
	return m_pimpl->m_isEnabled;
}

void Rule::Enable(bool newVal) {
	m_pimpl->m_isEnabled = newVal;
}

bool Rule::IsSilent() const {
	return m_pimpl->m_isSilent;
}

void Rule::SetSilent(bool newVal) {
	m_pimpl->m_isSilent = newVal;
}


//////////////////////////////////////////////////////////////////////////

ServiceRuleSet::ServiceRuleSet() {
	//...//
}

ServiceRuleSet::ServiceRuleSet(size_t reserve)
		: Base(reserve) {
	//...//
}

ServiceRuleSet::ServiceRuleSet(const ServiceRuleSet &rhs)
		: Base(rhs) {
	//...//
}

ServiceRuleSet::~ServiceRuleSet() throw() {
	//...//
}

const ServiceRuleSet & ServiceRuleSet::operator =(const ServiceRuleSet &rhs) {
	ServiceRuleSet tmp(rhs);
	Swap(tmp);
	return *this;
}

void ServiceRuleSet::Swap(ServiceRuleSet &rhs) throw() {
	Base::Swap(rhs);
}

//////////////////////////////////////////////////////////////////////////

class ServiceRule::Implementation {

public:

	Implementation() {
		//...//
	}

public:

	ServiceRule::ServiceSet m_serviceSet;

};

//////////////////////////////////////////////////////////////////////////

ServiceRule::ServiceRule()
		: m_pimpl(new Implementation) {
	//...//
}

ServiceRule::ServiceRule(const WString &uuid)
		: Rule(uuid),
		m_pimpl(new Implementation) {
	//...//
}

ServiceRule::ServiceRule(const ServiceRule &rhs)
		: Rule(rhs),
		m_pimpl(new Implementation(*rhs.m_pimpl)) {
	//...//
}

ServiceRule::~ServiceRule() {
	delete m_pimpl;
}

const ServiceRule & ServiceRule::operator =(const ServiceRule &rhs) {
	ServiceRule newTmp(rhs);
	Swap(newTmp);
	return *this;
}

void ServiceRule::Swap(ServiceRule &rhs) throw() {
	Rule::Swap(rhs);
	std::swap(m_pimpl, rhs.m_pimpl);
}

ServiceRule ServiceRule::MakeCopy() const {
	ServiceRule result(*this);
	result.SetUuid(Helpers::Uuid().GetAsString().c_str());
	return result;
}

const ServiceRule::ServiceSet & ServiceRule::GetServices() const {
	return const_cast<ServiceRule *>(this)->GetServices();
}

ServiceRule::ServiceSet & ServiceRule::GetServices() {
	return m_pimpl->m_serviceSet;
}

void ServiceRule::SetServices(const ServiceRule::ServiceSet &services) {
	m_pimpl->m_serviceSet = services;
}

//////////////////////////////////////////////////////////////////////////

TunnelRuleSet::TunnelRuleSet() {
	//...//
}

TunnelRuleSet::TunnelRuleSet(size_t reserve)
		: Base(reserve) {
	//...//
}

TunnelRuleSet::TunnelRuleSet(const TunnelRuleSet &rhs)
		: Base(rhs) {
	//...//
}

TunnelRuleSet::~TunnelRuleSet() throw() {
	//...//
}

const TunnelRuleSet & TunnelRuleSet::operator =(const TunnelRuleSet &rhs) {
	TunnelRuleSet tmp(rhs);
	Swap(tmp);
	return *this;
}

void TunnelRuleSet::Swap(TunnelRuleSet &rhs) throw() {
	Base::Swap(rhs);
}

//////////////////////////////////////////////////////////////////////////

class TunnelRule::Implementation {

public:

	Implementation()
			: m_acceptedConnectionsLimit(0) {
		//...//
	}

public:

	RuleEndpointCollection m_inputs;
	RuleEndpointCollection m_destinations;

	TunnelRule::Filters m_filters;

	unsigned long m_acceptedConnectionsLimit;

};

//////////////////////////////////////////////////////////////////////////

TunnelRule::TunnelRule()
		: m_pimpl(new Implementation) {
	//...//
}

TunnelRule::TunnelRule(const WString &uuid)
		: Rule(uuid),
		m_pimpl(new Implementation) {
	//...//
}

TunnelRule::TunnelRule(const TunnelRule &rhs)
		: Rule(rhs),
		m_pimpl(new Implementation(*rhs.m_pimpl)) {
	//...//
}

TunnelRule::~TunnelRule() {
	delete m_pimpl;
}

const TunnelRule & TunnelRule::operator =(const TunnelRule &rhs) {
	TunnelRule newTmp(rhs);
	Swap(newTmp);
	return *this;
}

void TunnelRule::Swap(TunnelRule &rhs) throw() {
	Rule::Swap(rhs);
	std::swap(m_pimpl, rhs.m_pimpl);
}

const RuleEndpointCollection & TunnelRule::GetInputs() const {
	return m_pimpl->m_inputs;
}

RuleEndpointCollection & TunnelRule::GetInputs() {
	return m_pimpl->m_inputs;
}

const RuleEndpointCollection & TunnelRule::GetDestinations() const {
	return m_pimpl->m_destinations;
}

RuleEndpointCollection & TunnelRule::GetDestinations() {
	return m_pimpl->m_destinations;
}

void TunnelRule::SetInputs(const RuleEndpointCollection &val) {
	m_pimpl->m_inputs = val;
}

void TunnelRule::SetDestinations(const RuleEndpointCollection &val) {
	m_pimpl->m_destinations = val;
}

const TunnelRule::Filters& TunnelRule::GetFilters() const {
	return m_pimpl->m_filters;
}

TunnelRule::Filters& TunnelRule::GetFilters() {
	return m_pimpl->m_filters;
}

void TunnelRule::SetFilters(const TunnelRule::Filters &val) {
	m_pimpl->m_filters = val;
}

unsigned long TunnelRule::GetAcceptedConnectionsLimit() const {
	return m_pimpl->m_acceptedConnectionsLimit;
}

void TunnelRule::SetAcceptedConnectionsLimit(unsigned long newLimit) {
	m_pimpl->m_acceptedConnectionsLimit = newLimit;
}

TunnelRule TunnelRule::MakeCopy() const {

	TunnelRule result(*this);
	result.SetUuid(Helpers::Uuid().GetAsString().c_str());

	struct Util {
		static void MakeCopy(RuleEndpointCollection &collection) {
			const size_t size = collection.GetSize();
			for (size_t i = 0; i < size; ++i) {
				collection[i] = collection[i].MakeCopy();
			}
		}
	};
	Util::MakeCopy(result.m_pimpl->m_inputs);
	Util::MakeCopy(result.m_pimpl->m_destinations);
	
	return result;

}


namespace fs = boost::filesystem;
using namespace TunnelEx;
using namespace TunnelEx::Helpers;
using namespace TunnelEx::Helpers::Xml;

namespace TunnelEx {

	//////////////////////////////////////////////////////////////////////////

	class RuleEntitySetXmlSaver {

	public:

		RuleEntitySetXmlSaver() {
			//...//
		}

	public:

		template<class XmlString>
		void Save(
					const ServiceRuleSet &serviceRuleSet,
					const TunnelRuleSet &tunnelRuleSet,
					XmlString &result)
				const {

			boost::shared_ptr<Document> doc(Document::CreateNew("RuleSet"));
			boost::shared_ptr<Node> root = doc->GetRoot();
			root->SetAttribute("Version", "2.1");

			SaveServiceRuleSet(serviceRuleSet, *root);
			SaveTunnelRuleSet(tunnelRuleSet, *root);
			
			try {
				fs::path schemaFile(GetModuleFilePathA().branch_path());
				schemaFile /= "RuleSet.xsd";
				std::string validateErrors;
				Schema schema(schemaFile.string());
				if (!schema.Validate(*doc, &validateErrors)) {
					WFormat message(
						L"Could not save rule std::set, internal error with XML (\"%1%\").");
					message % ConvertString<WString>(validateErrors.c_str()).GetCStr();
					throw LogicalException(message.str().c_str());
				}
			} catch (const Schema::ParseException& ex) {
				WFormat message(L"Could not load system XML-Schema file: \"%1%\".");
				message % ex.what();
				throw SystemException(message.str().c_str());
			}
			
			doc->Dump(result);

		}

	protected:
		
		void SaveRuleErrorsTreatment(
					const Rule &entity,
					Node &node)
				const {
			const char *treatment;
			switch (entity.GetErrorsTreatment()) {
				case TunnelRule::ERRORS_TREATMENT_INFO:
					treatment = "information";
					break;
				case TunnelRule::ERRORS_TREATMENT_WARN:
					treatment = "warning";
					break;
				default:
					assert(false);
				case TunnelRule::ERRORS_TREATMENT_ERROR:
					treatment = "error";
					break;
			}
			node.SetAttribute("ErrorsTreatment", treatment);
		}

		boost::shared_ptr<Node> CreateEntity(
					Node &list,
					const Rule &source,
					const char *const tagName)
				const {
			boost::shared_ptr<Node> node = list.CreateNewChild(tagName);
			node->SetAttribute("Name", source.GetName());
			node->SetAttribute("IsEnabled", source.IsEnabled() ? "true" : "false");
			SaveRuleErrorsTreatment(source, *node);
			node->SetAttribute("Uuid", source.GetUuid());
			return node;
		}

		void SaveListeners(
					const RuleEndpoint::Listeners &listeners,
					Node &endpointNode,
					const char *tagName)
				const {
			const size_t listenersNumb = listeners.GetSize();
			for (size_t i = 0; i < listenersNumb; ++i) {
				boost::shared_ptr<Node> listenerNode = endpointNode.CreateNewChild(tagName);
				listenerNode->SetAttribute("Name", listeners[i].name);
				listenerNode->SetContent(listeners[i].param);
			}
		}

		void SaveInputEndpoints(
					const RuleEndpointCollection &endpoints,
					Node &ruleNode)
				const {
			boost::shared_ptr<Node> endpointsNode = ruleNode.CreateNewChild("InputSet");
			const size_t size = endpoints.GetSize();
			for (size_t i = 0; i < size; ++i) {
				boost::shared_ptr<Node> endpointNode = endpointsNode->CreateNewChild("Endpoint");
				SaveListeners(endpoints[i].GetPreListeners(), *endpointNode, "PreListener");
				SaveListeners(endpoints[i].GetPostListeners(), *endpointNode, "PostListener");
				if (endpoints[i].IsCombined()) {
					boost::shared_ptr<Node> addressNode = endpointNode->CreateNewChild("CombinedAddress");
					addressNode->SetAttribute(
						"ResourceIdentifier",
						endpoints[i].GetCombinedResourceIdentifier());
					addressNode->SetAttribute(
						"IsAcceptor",
						endpoints[i].IsCombinedAcceptor() ? "true" : "false");
				} else {
					boost::shared_ptr<Node> addressNode = endpointNode->CreateNewChild("SplitAddress");
					addressNode->SetAttribute(
						"ReadResourceIdentifier",
						endpoints[i].GetReadResourceIdentifier());
					addressNode->SetAttribute(
						"WriteResourceIdentifier",
						endpoints[i].GetWriteResourceIdentifier());
					const char *acceptor;
					switch (endpoints[i].GetReadWriteAcceptor()) {
					default:
						assert(false);
					case Endpoint::ACCEPTOR_NONE:
						acceptor = "none";
						break;
					case Endpoint::ACCEPTOR_READER:
						acceptor = "reader";
						break;
					case Endpoint::ACCEPTOR_WRITER:
						acceptor = "writer";
						break;
					}
					addressNode->SetAttribute("Acceptor", acceptor);
				}
				endpointNode->SetAttribute("Uuid", endpoints[i].GetUuid());
			}
		}

		void SaveDestinationEndpoints(
					const RuleEndpointCollection &endpoints,
					Node &node)
				const {
			boost::shared_ptr<Node> endpointsNode = node.CreateNewChild("DestinationSet");
			const size_t size = endpoints.GetSize();
			for (size_t i = 0; i < size; ++i) {
				boost::shared_ptr<Node> endpointNode = endpointsNode->CreateNewChild("Endpoint");
				SaveListeners(endpoints[i].GetPreListeners(), *endpointNode, "PreListener");
				SaveListeners(endpoints[i].GetPostListeners(), *endpointNode, "PostListener");
				if (endpoints[i].IsCombined()) {
					boost::shared_ptr<Node> addressNode = endpointNode->CreateNewChild("CombinedAddress");
					addressNode->SetAttribute(
						"ResourceIdentifier",
						endpoints[i].GetCombinedResourceIdentifier());
				} else {
					boost::shared_ptr<Node> addressNode = endpointNode->CreateNewChild("SplitAddress");
					addressNode->SetAttribute(
						"ReadResourceIdentifier",
						endpoints[i].GetReadResourceIdentifier());
					addressNode->SetAttribute(
						"WriteResourceIdentifier",
						endpoints[i].GetWriteResourceIdentifier());
				}
				endpointNode->SetAttribute("Uuid", endpoints[i].GetUuid());
			}
		}

		void SaveTunnelRuleSet(const TunnelRuleSet &set, Node &root) const {
			for (unsigned int i = 0; i < set.GetSize(); ++i) {
				const TunnelRule &rule = set[i];
				boost::shared_ptr<Node> ruleNode = CreateEntity(root, rule, "TunnelRule");
				{
					boost::shared_ptr<Node> filtersNode
						= ruleNode->CreateNewChild("FilterSet");
					for (	unsigned int filterIndex = 0;
							filterIndex < rule.GetFilters().GetSize();
							++filterIndex) {
						filtersNode->CreateNewChild("Filter")->SetAttribute(
							"Name",
							rule.GetFilters()[filterIndex]);
					}
				}
				SaveInputEndpoints(rule.GetInputs(), *ruleNode);
				SaveDestinationEndpoints(rule.GetDestinations(), *ruleNode);
			}
		}

		void SaveServices(const ServiceRule::ServiceSet &set, Node &root) const {
			for (unsigned int i = 0; i < set.GetSize(); ++i) {
				const ServiceRule::Service &service = set[i];
				boost::shared_ptr<Node> node = root.CreateNewChild("Service");
				node->SetAttribute("Name", service.name);
				node->SetAttribute("Uuid", service.uuid);
				node->SetContent(service.param);
			}
		}

		void SaveServiceRuleSet(const ServiceRuleSet &set, Node &root) const {
			for (unsigned int i = 0; i < set.GetSize(); ++i) {
				const ServiceRule &rule = set[i];
				boost::shared_ptr<Node> ruleNode = CreateEntity(root, rule, "ServiceRule");
				SaveServices(rule.GetServices(), *ruleNode);
			}
		}

	};


	//////////////////////////////////////////////////////////////////////////

	class RuleEntitySetXmlParser : private boost::noncopyable {

	protected:

		Rule::ErrorsTreatment ParseErrorsTreatment(
					boost::shared_ptr<const Node> node)
				const {
			std::string errorsTreatment;
			node->GetAttribute("ErrorsTreatment", errorsTreatment);
			if (errorsTreatment == "information") {
				return Rule::ERRORS_TREATMENT_INFO;
			} else if (errorsTreatment == "warning") {
				return Rule::ERRORS_TREATMENT_WARN;
			} else {
				assert(errorsTreatment == "error");
				return Rule::ERRORS_TREATMENT_ERROR;
			}
		}

		template<class Entity>
		boost::shared_ptr<Entity> ParseEntity(boost::shared_ptr<const Node> node) const {
			WString buffer;
			boost::shared_ptr<Entity> entity(new Entity(node->GetAttribute("Uuid", buffer)));
			entity->SetName(node->GetAttribute("Name", buffer));
			entity->SetErrorsTreatment(ParseErrorsTreatment(node));
			entity->Enable(node->GetAttribute("IsEnabled", buffer) == L"true");
			return entity;
		}

		template<class RuleSet>
		void Parse(
					const Document &doc,
					const std::string &tagName,
					boost::function<boost::shared_ptr<typename RuleSet::ItemType>(boost::shared_ptr<const Node>)> entityParser,
					RuleSet &result)
				const {
			ConstNodeCollection nodes;
			const std::string path = "/RuleSet/" + tagName;
			doc.GetXPath()->Query(path.c_str(), nodes);
			typename RuleSet set(nodes.size());
			foreach (boost::shared_ptr<const Node> &node, nodes) {
				set.Append(*entityParser(node));
			}
			result.Swap(set);
		}

	};

	//////////////////////////////////////////////////////////////////////////

	class TunnelRuleSetXmlParser : private RuleEntitySetXmlParser {

	public:

		TunnelRuleSetXmlParser() {
			//...//
		}

	public:

		void Parse(const Document &doc, TunnelRuleSet &result) const {
			RuleEntitySetXmlParser::Parse(
				doc,
				"TunnelRule",
				boost::bind(&TunnelRuleSetXmlParser::ParseRule, this, _1),
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
			for (	boost::shared_ptr<const Node> node = endpointNode.GetChildElement();
					node;
					node = node->GetNextElement()) {
				node->GetName(wbuffer);
				if (wbuffer == L"CombinedAddress") {
					endpoint.SetCombinedResourceIdentifier(
						node->GetAttribute("ResourceIdentifier", wbuffer),
						node->GetAttribute("IsAcceptor", wbuffer2) == L"true");
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
			for (	boost::shared_ptr<const Node> node = endpointNode.GetChildElement();
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
					boost::shared_ptr<const Node> node,
					RuleEndpointCollection &result)
				const {
			RuleEndpointCollection set;
			for (node = node->GetChildElement(); node; node = node->GetNextElement()) {
				set.Append(ParseInputEndpoint(*node));
			}
			set.Swap(result);
		}

		void ParseDestinationEndpoints(
					boost::shared_ptr<const Node> node,
					RuleEndpointCollection &result)
				const {
			RuleEndpointCollection set;
			for (node = node->GetChildElement(); node; node = node->GetNextElement()) {
				set.Append(ParseDestinationEndpoint(*node));
			}
			set.Swap(result);
		}

		void ParseFilters(
					boost::shared_ptr<const Node> node,
					TunnelRule::Filters &result)
				const {
			TunnelRule::Filters set;
			std::wstring buffer;
			for (node = node->GetChildElement(); node; node = node->GetNextElement()) {
				set.Append(node->GetAttribute("Name", buffer).c_str());
			}
			set.Swap(result);
		}

		boost::shared_ptr<TunnelRule> ParseRule(boost::shared_ptr<const Node> node) const {
			boost::shared_ptr<TunnelRule> rule = ParseEntity<TunnelRule>(node);
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

	class ServiceRuleSetXmlParser : private RuleEntitySetXmlParser {

	public:

		ServiceRuleSetXmlParser() {
			//...//
		}

	public:

		void Parse(const Document &doc, ServiceRuleSet &result) const {
			RuleEntitySetXmlParser::Parse(
				doc,
				"ServiceRule",
				boost::bind(&ServiceRuleSetXmlParser::ParseRule, this, _1),
				result);
		}

	protected:

		boost::shared_ptr<ServiceRule> ParseRule(boost::shared_ptr<const Node> node) const {
			boost::shared_ptr<ServiceRule> rule = ParseEntity<ServiceRule>(node);
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

	//////////////////////////////////////////////////////////////////////////

}

//////////////////////////////////////////////////////////////////////////

class RuleSet::Implementation {

public:

	Implementation() {
		//...//
	}

	explicit Implementation(const WString &xml) {
		
		try {
		
			boost::shared_ptr<Document> doc(Document::LoadFromString(xml));
			fs::path schemaFile(Helpers::GetModuleFilePathA().branch_path());
			schemaFile /= "RuleSet.xsd";
			Schema schema(schemaFile.string());
			std::string validateErrors;

			if (schema.Validate(*doc, &validateErrors)) {
			
				{
					ConstNodeCollection uuids;
					doc->GetXPath()->Query("//*/@Uuid", uuids);
					typedef std::set<std::string> Collection;
					Collection collection;
					std::string buffer;
					const ConstNodeCollection::const_iterator end(uuids.end());
					foreach (boost::shared_ptr<const Node> &node, uuids) {
						if (!collection.insert(node->GetContent(buffer)).second) {
							Log::GetInstance().AppendWarn(
								(Format("Rule set UUID \"%1%\" is not unique.") % buffer).str());
						}
					}
				}

				ServiceRuleSetXmlParser().Parse(*doc, m_serviceRuleSet);
				TunnelRuleSetXmlParser().Parse(*doc, m_tunnelRuleSet);

			} else {
				WFormat message(
					L"Passed XML-string has invalid format and can not be loaded by rule (\"%1%\").");
				message % ConvertString<WString>(validateErrors.c_str()).GetCStr();
				throw XmlDoesNotMatchException(message.str().c_str());
			}

		} catch (const Schema::ParseException &ex) {
			WFormat message(L"Could not load system XML-Schema file: \"%1%\".");
			message % ex.what();
			throw SystemException(message.str().c_str());
		} catch (const Document::ParseException &) {
			const wchar_t *const message
				=	L"Could not parse XML-string with rule std::set,"
						L" std::string has invalid format, invalid text encoding or empty.";
			throw InvalidXmlException(message);
		}

	}

	explicit Implementation(const ServiceRuleSet &serviceSet, const TunnelRuleSet &ruleSet)
			: m_serviceRuleSet(serviceSet),
			m_tunnelRuleSet(ruleSet) {
		//...//
	}

public:

	ServiceRuleSet m_serviceRuleSet;
	TunnelRuleSet m_tunnelRuleSet;

};

//////////////////////////////////////////////////////////////////////////

RuleSet::RuleSet()
		: m_pimpl(new Implementation) {
	//...//
}

RuleSet::RuleSet(const WString &xml)
		: m_pimpl(new Implementation(xml)) {
	//...//
}

RuleSet::RuleSet(
			const ServiceRuleSet &serviceRuleSet,
			const TunnelRuleSet &ruleRuleSet)
		: m_pimpl(new Implementation(serviceRuleSet, ruleRuleSet)) {
	//...//
}

RuleSet::RuleSet(const RuleSet &rhs)
		: m_pimpl(new Implementation(*rhs.m_pimpl)) {
	//...//
}

RuleSet::~RuleSet() throw() {
	//...//
}

const RuleSet & RuleSet::operator =(const RuleSet &rhs) {
	RuleSet tmp(rhs);
	Swap(tmp);
	return *this;
}

void RuleSet::Swap(RuleSet &rhs) throw() {
	std::swap(rhs.m_pimpl, m_pimpl);
}

void RuleSet::GetXml(UString &destinationBuffer) const {
	RuleEntitySetXmlSaver()
		.Save(GetServices(), GetTunnels(), destinationBuffer);
}

void RuleSet::GetXml(WString &destinationBuffer) const {
	RuleEntitySetXmlSaver()
		.Save(GetServices(), GetTunnels(), destinationBuffer);
}

void RuleSet::GetXml(
			const ServiceRuleSet &s,
			const TunnelRuleSet &t,
			UString &b) {
	RuleEntitySetXmlSaver().Save(s, t, b);
}

void RuleSet::GetXml(
			const ServiceRuleSet &s,
			const TunnelRuleSet &t,
			WString &b) {
	RuleEntitySetXmlSaver().Save(s, t, b);
}

ServiceRuleSet & RuleSet::GetServices() {
	return m_pimpl->m_serviceRuleSet;
}

TunnelRuleSet & RuleSet::GetTunnels() {
	return m_pimpl->m_tunnelRuleSet;
}

const ServiceRuleSet & RuleSet::GetServices() const {
	return const_cast<RuleSet *>(this)->GetServices();
}

const TunnelRuleSet & RuleSet::GetTunnels() const {
	return const_cast<RuleSet *>(this)->GetTunnels();
}

size_t RuleSet::GetSize() const {
	return GetTunnels().GetSize() + GetServices().GetSize();
}

void RuleSet::Append(const ServiceRule &rule) {
	GetServices().Append(rule);
}

void RuleSet::Append(const TunnelRule &rule) {
	GetTunnels().Append(rule);
}

//////////////////////////////////////////////////////////////////////////

#if TEMPLATES_REQUIRE_SOURCE != 0
#	include "Collection.cpp"
	namespace {
		//! Only for template instances.
		void MakeTemplateInstantiation() {
			Helpers::MakeCollectionTemplateInstantiation<ServiceRuleSet>();
			Helpers::MakeCollectionTemplateInstantiation<ServiceRule::ServiceSet>();
			Helpers::MakeCollectionTemplateInstantiation<TunnelRuleSet>();
			Helpers::MakeCollectionTemplateInstantiation<TunnelRule::Filters>();
		}
	}
#endif // TEMPLATES_REQUIRE_SOURCE

//////////////////////////////////////////////////////////////////////////
