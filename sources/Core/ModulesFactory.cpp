/**************************************************************************
 *   Created: 2007/03/28 9:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "ModulesFactory.hpp"
#include "Connection.hpp"
#include "Service.hpp"
#include "TrafficLogger.hpp"
#include "Rule.hpp"
#include "Log.hpp"
#include "EndpointAddress.hpp"
#include "Locking.hpp"
#include "Dll.hpp"

namespace mi = boost::multi_index;
using namespace TunnelEx;
using namespace TunnelEx::Singletons;

//////////////////////////////////////////////////////////////////////////

namespace {

	template<class T>
	SharedPtr<PreListener> CreateListenerModule(
				TunnelEx::Server::Ref server,
				const RuleEndpoint::ListenerInfo &info,
				const TunnelRule &rule,
				const Connection &currentConnection,
				const Connection &oppositeConnection) {
		return SharedPtr<PreListener>(
			new T(server, info, rule, currentConnection, oppositeConnection));
	}

	void AppendModuleVetsionToLog(const Helpers::Dll &modDll) {
		
		if (!Log::GetInstance().IsDebugRegistrationOn()) {
			return;
		}
		
		typedef const char *(GetTunnelExModuleNameFuncPrototype)(void);
		typedef const char *(GetTunnelExModuleVersionFuncPrototype)(void);
		const char *const getTunnelExModuleNameFuncName = "GetTunnelExModuleName";
		const char *const getTunnelExModuleVersionFuncName = "GetTunnelExModuleVersion";

		try {
			Log::GetInstance().AppendDebug(
				"Loaded module \"%1%\" version %2%.",
				modDll.GetFunction<GetTunnelExModuleNameFuncPrototype>(getTunnelExModuleNameFuncName)(),
				modDll.GetFunction<GetTunnelExModuleVersionFuncPrototype>(getTunnelExModuleVersionFuncName)());
		} catch (const Helpers::Dll::DllFuncException &) {
			Format message("Failed to find module name or version info for %1%.");
			message % modDll.GetFile();
			Log::GetInstance().AppendWarn(message.str());
		}
	
	}

}

//////////////////////////////////////////////////////////////////////////

class ModulesFactoryPolicy::Implementation : private boost::noncopyable {

private:

	typedef SharedPtr<PreListener>(ListenerFabricPrototype)(
			Server::Ref,
			const RuleEndpoint::ListenerInfo &,
			const TunnelRule &,
			const Connection &currentConnection,
			const Connection &oppositeConnection);
	typedef boost::function<ListenerFabricPrototype> ListenerFabric;
	//! \todo: search with hash [2008/06/21 1:27]
	typedef std::map<std::wstring, ListenerFabric> ListenerFabricCollection;

	typedef SharedPtr<Filter> (DestinationPingFilterFabricPrototype)(
		SharedPtr<TunnelRule>, SharedPtr<RecursiveMutex>);
	typedef boost::function<DestinationPingFilterFabricPrototype>
		DestinationPingFilterFabric;

	typedef AutoPtr<EndpointAddress>(
			EndpointAddressFabricPrototype)(
				Server::ConstRef,
				const WString &);
	typedef boost::function<EndpointAddressFabricPrototype>
		EndpointAddressFabricFunc;

	struct EndpointAddressFabric {
		explicit EndpointAddressFabric(
					const wchar_t *proto,
					const EndpointAddressFabricFunc &func)
				: proto(proto),
				func(func) {
			//...//
		}
		std::wstring proto;
		EndpointAddressFabricFunc func;
	};

	typedef AutoPtr<Service>(ServiceFabricPrototype)(
		SharedPtr<const ServiceRule>,
		const ServiceRule::Service &);
	typedef boost::function<ServiceFabricPrototype> ServiceFabricFunc;

	struct ServiceFabric {
		explicit ServiceFabric(const wchar_t *name, const ServiceFabricFunc &func)
				: name(name),
				func(func) {
			//...//
		}
		WString name;
		ServiceFabricFunc func;
	};

	struct ByProto {
		//...//
	};

	struct ByName {
		//...//
	};

	typedef boost::multi_index_container<
			EndpointAddressFabric,
			mi::indexed_by<
				mi::hashed_unique<
					mi::tag<ByProto>,
					mi::member<
						EndpointAddressFabric,
						std::wstring,
						&EndpointAddressFabric::proto> > > >
		EndpointAddressFabrics;
	typedef EndpointAddressFabrics::index<ByProto>::type EndpointAddressFabricByProto;

	typedef boost::multi_index_container<
			ServiceFabric,
			mi::indexed_by<
				mi::hashed_unique<
					mi::tag<ByName>,
					mi::member<
						ServiceFabric,
						WString,
						&ServiceFabric::name>,
					Helpers::StringHasher<WString> > > >
		ServiceFabrics;
	typedef ServiceFabrics::index<ByName>::type ServiceFabricByName;

public:

	Implementation()
			: m_enpointAddrTypeExp(L"([^:/]+):/{2,}([^/].*)") {

		m_listenerFabricCollection.insert(
			std::make_pair<std::wstring, ListenerFabric>(
				L"TrafficLogger/File",
				ListenerFabric(&CreateListenerModule<TrafficLogger>)));

		try {

			m_modInetDll.reset(new Helpers::Dll(TUNNELEX_MODULE_INET_DLL_FILE_NAME));
			AppendModuleVetsionToLog(*m_modInetDll);

			m_endpointAddressFabrics.insert(
				EndpointAddressFabric(
					L"tcp",
					m_modInetDll->GetFunction<EndpointAddressFabricPrototype>("CreateTcpEndpointAddress")));

			m_endpointAddressFabrics.insert(
				EndpointAddressFabric(
					L"udp",
					m_modInetDll->GetFunction<EndpointAddressFabricPrototype>("CreateUdpEndpointAddress")));

			m_listenerFabricCollection.insert(
				std::make_pair(
					L"Tunnel/Ftp/Active",
					m_modInetDll->GetFunction<ListenerFabricPrototype>("CreateActiveFtpListener")));
			m_listenerFabricCollection.insert(
				std::make_pair(
					L"Tunnel/Ftp/Passive",
					m_modInetDll->GetFunction<ListenerFabricPrototype>("CreatePassiveFtpListener")));

			m_destinationPingFilterFabric
				= m_modInetDll->GetFunction<DestinationPingFilterFabricPrototype>("CreateDestinationPingFilter");

		} catch (const ::TunnelEx::DllException &ex) {
			Format message("Could not properly load \"Inet\" module: \"%1%\".");
			message % ConvertString<String>(ex.GetWhat()).GetCStr();
			Log::GetInstance().AppendError(message.str());
		}

		try {
			m_modPathfinderDll.reset(new Helpers::Dll(TUNNELEX_MODULE_PATHFINDER_DLL_FILE_NAME));
			m_endpointAddressFabrics.insert(
				EndpointAddressFabric(
					L"pathfinder",
					m_modPathfinderDll->GetFunction<EndpointAddressFabricPrototype>(
						"CreateEndpointAddress")));
		} catch (const ::TunnelEx::DllException &ex) {
			Format message("Could not properly load \"Pathfinder\" module: \"%1%\".");
			message % ConvertString<String>(ex.GetWhat()).GetCStr();
			Log::GetInstance().AppendError(message.str());
		}

		try {
			m_modPipeDll.reset(new Helpers::Dll( TUNNELEX_MODULE_PIPE_DLL_FILE_NAME));
			m_endpointAddressFabrics.insert(
				EndpointAddressFabric(
					L"pipe",
					m_modPipeDll->GetFunction<EndpointAddressFabricPrototype>(
						"CreateEndpointAddress")));
		} catch (const ::TunnelEx::DllException &ex) {
			Format message("Could not properly load \"Pipe\" module: \"%1%\".");
			message % ConvertString<String>(ex.GetWhat()).GetCStr();
			Log::GetInstance().AppendError(message.str());
		}

		try {
			m_modSerialDll.reset(new Helpers::Dll(TUNNELEX_MODULE_SERIAL_DLL_FILE_NAME));
			m_endpointAddressFabrics.insert(
				EndpointAddressFabric(
					L"serial",
					m_modSerialDll->GetFunction<EndpointAddressFabricPrototype>(
						"CreateEndpointAddress")));
		} catch (const ::TunnelEx::DllException &ex) {
			Format message("Could not properly load \"Serial\" module: \"%1%\".");
			message % ConvertString<String>(ex.GetWhat()).GetCStr();
			Log::GetInstance().AppendError(message.str());
		}

		try {
			m_modUpnpDll.reset(new Helpers::Dll(TUNNELEX_MODULE_UPNP_DLL_FILE_NAME));
			m_endpointAddressFabrics.insert(
				EndpointAddressFabric(
					L"upnp_tcp",
					m_modUpnpDll->GetFunction<EndpointAddressFabricPrototype>(
						"CreateUpnpTcpEndpointAddress")));
			m_endpointAddressFabrics.insert(
				EndpointAddressFabric(
					L"upnp_udp",
					m_modUpnpDll->GetFunction<EndpointAddressFabricPrototype>(
						"CreateUpnpUdpEndpointAddress")));
			m_serviceFabrics.insert(
				ServiceFabric(
					L"Upnpc",
					m_modUpnpDll->GetFunction<ServiceFabricPrototype>(
						"CreateUpnpcService")));
		} catch (const ::TunnelEx::DllException &ex) {
			Format message("Could not properly load \"UPnP\" module: \"%1%\".");
			message % ConvertString<String>(ex.GetWhat()).GetCStr();
			Log::GetInstance().AppendError(message.str());
		}

	}

	void CreateFilters(
				SharedPtr<TunnelRule> rule,
				SharedPtr<RecursiveMutex> ruleChangingMutex,
				std::vector<SharedPtr<Filter> > &filters) {
		const TunnelRule::Filters &filtersNames(rule->GetFilters());
		const size_t filtersNumber = filtersNames.GetSize();
		filters.reserve(filtersNumber);
		for (unsigned int i = 0; i < filtersNumber; ++i) {
			if (m_destinationPingFilterFabric && filtersNames[i] == L"DestinationsSorter/Ping") {
				filters.push_back(m_destinationPingFilterFabric(rule, ruleChangingMutex));
			} else {
				//! @todo: what about this error? whould we throw (and close connection) or just notify? [2008/08/19 14:53]
				String aNameBuffer;
				ConvertString(filtersNames[i], aNameBuffer);
				Log::GetInstance().AppendError(
					(Format("Could not find module \"%1%\".") % aNameBuffer.GetCStr()).str());
			}
		}
	}

	void CreatePreListeners(
				Server::Ref server,
				const TunnelRule &rule,
				const Connection &currentConnection,
				const Connection &oppositeConnection,
				boost::function<void(SharedPtr<PreListener>)> preListenersReceiveCallback) {
		
		const bool isDebugAvailable = Log::GetInstance().IsDebugRegistrationOn();
		const RuleEndpoint::Listeners &listeners
			= currentConnection.GetRuleEndpoint().GetPreListeners();
		const size_t size = listeners.GetSize();
		
		for (unsigned int i = 0; i < size; ++i) {
			
			const RuleEndpoint::ListenerInfo &info = listeners[i];
			const ListenerFabricCollection::const_iterator pos 
				= m_listenerFabricCollection.find(info.name.GetCStr());
			
			if (pos != m_listenerFabricCollection.end()) {
			
				preListenersReceiveCallback(
					pos->second(server, info, rule, currentConnection, oppositeConnection));
				if (isDebugAvailable) {
					Log::GetInstance().AppendDebug(
						"Created listener (pre) %1% for connection %2% from rule %3%.",
						ConvertString<String>(info.name).GetCStr(),
						currentConnection.GetInstanceId(),
						ConvertString<String>(rule.GetUuid()).GetCStr());
				}
			
			} else {
				//! @todo: what about this error? should throw (and close connection) or just notify? [2008/08/19 14:53]
				Format message("Could not find module \"%1%\".");
				message % ConvertString<String>(info.name).GetCStr();
				Log::GetInstance().AppendError(message.str());
			}

		}
	}
	
	void CreatePostListeners(
				Server::Ref,
				const TunnelRule &,
				const Connection &currentConnection,
				const Connection &,
				boost::function<void(SharedPtr<PostListener>)>) {
		const RuleEndpoint::Listeners &listeners
			= currentConnection.GetRuleEndpoint().GetPostListeners();
		const size_t size = listeners.GetSize();
		for (unsigned int i = 0; i < size; ++i) {
			//! @todo: what about this error? should throw (and close connection) or just notify? [2008/08/19 14:53]
			String aNameBuffer;
			ConvertString(listeners[i].name, aNameBuffer);
			Format message("Could not find module \"%1%\".");
			message % aNameBuffer.GetCStr();
			Log::GetInstance().AppendError(message.str());
		}
	}

	AutoPtr<EndpointAddress> CreateEndpointAddress(
				const WString &resourceIdentifier) {
		boost::wcmatch what;
		assert(boost::regex_match(resourceIdentifier.GetCStr(), what, m_enpointAddrTypeExp));
		if (boost::regex_match(resourceIdentifier.GetCStr(), what, m_enpointAddrTypeExp)) {
			EndpointAddressFabricByProto &index
				= m_endpointAddressFabrics.get<ByProto>();
			EndpointAddressFabricByProto::const_iterator fabricPos
				= index.find(what[1]);
			assert(fabricPos != index.end());
			if (fabricPos != index.end()) {
				assert(fabricPos->func);
				return fabricPos->func(Server::GetInstance(), what[2].str().c_str());
			}
		}
		throw InvalidLinkException(
			L"Could not deduce protocol for endpoint address from resource identifier.");
	}


	AutoPtr<Service> CreateService(
				TunnelEx::SharedPtr<const ServiceRule> rule,
				const ServiceRule::Service &service) {

		ServiceFabricByName &index = m_serviceFabrics.get<ByName>();
		const ServiceFabricByName::const_iterator fabricPos = index.find(service.name);
		assert(fabricPos != index.end());
		if (fabricPos == index.end()) {
			WFormat message(L"Could not find service by name \"%1%\".");
			message % service.name.GetCStr();
			throw InvalidLinkException(message.str().c_str());
		}
		
		assert(fabricPos->func);
		return fabricPos->func(rule, service);
	
	}

private:

	ListenerFabricCollection m_listenerFabricCollection;

	boost::wregex m_enpointAddrTypeExp;

	std::auto_ptr<Helpers::Dll> m_modInetDll;
	std::auto_ptr<Helpers::Dll> m_modPathfinderDll;
	std::auto_ptr<Helpers::Dll> m_modPipeDll;
	std::auto_ptr<Helpers::Dll> m_modSerialDll;
	std::auto_ptr<Helpers::Dll> m_modUpnpDll;

	EndpointAddressFabrics m_endpointAddressFabrics;
	ServiceFabrics m_serviceFabrics;
	
	DestinationPingFilterFabric m_destinationPingFilterFabric;

}; 

//////////////////////////////////////////////////////////////////////////

ModulesFactoryPolicy::ModulesFactoryPolicy()
		: m_pimpl(new Implementation) {
	//...//
}

ModulesFactoryPolicy::~ModulesFactoryPolicy() {
	//...//
}

void ModulesFactoryPolicy::CreateFilters(
			SharedPtr<TunnelRule> rule,
			SharedPtr<RecursiveMutex> ruleChangingMutex,
			std::vector<SharedPtr<Filter> > &filters) {
	m_pimpl->CreateFilters(rule, ruleChangingMutex, filters);
}

void ModulesFactoryPolicy::CreatePreListeners(
			Server::Ref server,
			const TunnelRule &rule,
			const Connection &currentConnection,
			const Connection &oppositeConnection,
			boost::function<void(SharedPtr<PreListener>)> preListenersReceiveCallback) {
	m_pimpl->CreatePreListeners(
		server,
		rule,
		currentConnection,
		oppositeConnection,
		preListenersReceiveCallback);
}

void ModulesFactoryPolicy::CreatePostListeners(
			Server::Ref server,
			const TunnelRule &rule,
			const Connection &currentConnection,
			const Connection &oppositeConnection,
			boost::function<void(SharedPtr<PostListener>)> postListenersReceiveCallback) {
	m_pimpl->CreatePostListeners(
		server,
		rule,
		currentConnection,
		oppositeConnection,
		postListenersReceiveCallback);
}

AutoPtr<EndpointAddress> ModulesFactoryPolicy::CreateEndpointAddress(
			const WString &resourceIdentifier) {
	return m_pimpl->CreateEndpointAddress(resourceIdentifier);
}

AutoPtr<Service> ModulesFactoryPolicy::CreateService(
			TunnelEx::SharedPtr<const ServiceRule> rule,
			const ServiceRule::Service &service) {
	return m_pimpl->CreateService(rule, service);
}

//////////////////////////////////////////////////////////////////////////

#if TEMPLATES_REQUIRE_SOURCE != 0
#	include "Singleton.cpp"
	//! Only for template instantiation.
	void MakeModulesFactoryTemplateInstantiation() {
		ModulesFactory::GetInstance();
	}
#endif // TEMPLATES_REQUIRE_SOURCE
