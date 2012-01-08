/**************************************************************************
 *   Created: 2007/03/06 4:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "ServerWorker.hpp"
#include "AcceptHandler.hpp"
#include "Acceptor.hpp"
#include "Connection.hpp"
#include "Tunnel.hpp"
#include "Service.hpp"
#include "ModulesFactory.hpp"
#include "Filter.hpp"
#include "Locking.hpp"
#include "Rule.hpp"
#include "EndpointAddress.hpp"
#include "Filter.hpp"
#include "Log.hpp"
#include "Error.hpp"
#include "Exceptions.hpp"
#include "Licensing.hpp"


namespace mi = boost::multi_index;
using namespace TunnelEx;

////////////////////////////////////////////////////////////////////////////////

namespace {

	//! @todo: hadcored min thread number and tunnel thread opening step, move to options or config
	const auto openingTunnelMinThreadCount = 4;
	//! @todo: hardcored maximum thread count
	const auto openingTunnelMaxThreadCount = 600;
	//! @todo: hadcored sleep time, move to options or config
	const ACE_Time_Value openingTunnelThreadMaxIdleTime(20 * 60);
	
	//! @todo: hadcored min thread number, move to options or config
	const auto proactorMinThreadCount = 8; // see TEX-689 for details
	
	static_assert(openingTunnelMaxThreadCount >= openingTunnelMinThreadCount, "Logic error.");

}

//////////////////////////////////////////////////////////////////////////

class ServerWorker::LicenseException : public LocalException {

public:

	explicit LicenseException(const wchar_t *what) throw()
			: LocalException(what) {
		//...//
	}
	
	LicenseException(const LogicalException &rhs) throw()
			: LocalException(rhs) {
		//...//
	}
	
	virtual ~LicenseException() throw() {
		//...//
	}
	
	LicenseException & operator =(const LicenseException &rhs) throw() {
		LocalException::operator =(rhs);
		return *this;
	}
	
	virtual AutoPtr<LocalException> Clone() const {
		return AutoPtr<LocalException>(new LicenseException(*this));
	}

};

//////////////////////////////////////////////////////////////////////////

template<class Base>
class ServerWorker::ConnectionOpeningExceptionImpl : public Base {
public:
	explicit ConnectionOpeningExceptionImpl(
				SharedPtr<const EndpointAddress> address,
				const LocalException &error,
				const wchar_t *const connectionType)
			: Base(L""),
			m_address(address),
			m_error(error.Clone()),
			m_connectionType(connectionType) {
		//...//
	}
	ConnectionOpeningExceptionImpl(
				const ConnectionOpeningExceptionImpl &rhs)
			: Base(rhs),
			m_address(rhs.m_address),
			m_error(rhs.m_error->Clone()),
			m_connectionType(rhs.m_connectionType) {
		//...//
	}
	ConnectionOpeningExceptionImpl & operator =(
				const ConnectionOpeningExceptionImpl &rhs) {
		DestinationConnectionOpeningException::operator =(rhs);
		m_address = rhs.m_address;
		m_error = rhs.m_error->Clone();
		m_connectionType = rhs.m_connectionType;
		return *this;
	}
	virtual const wchar_t * GetWhat() const throw() {
		try {
			return GetWhatImpl().c_str();
		} catch (...) {
			assert(false);
			return L"Unknown error";
		}
	}
	AutoPtr<LocalException> Clone() const {
		return AutoPtr<LocalException>(
			new ConnectionOpeningExceptionImpl<Base>(*this));
	}
private:
	const std::wstring & GetWhatImpl() const {
		if (m_what.empty()) {
			WFormat what(
				L"Opening new %3% connection to %1%"
				L" is failed with error \"%2%\"");
			what % m_address->GetResourceIdentifier().GetCStr();
			what % m_error->GetWhat();
			what % m_connectionType;
			m_what = what.str();
		}
		return m_what;
	}
private:
	SharedPtr<const EndpointAddress> m_address;
	AutoPtr<LocalException> m_error;
	mutable std::wstring m_what;
	const wchar_t *m_connectionType;
};

//////////////////////////////////////////////////////////////////////////

class ServerWorker::RuleInfo : private boost::noncopyable {
public:
	RuleInfo()
			: acceptedConnectionNumb(0) {
		//...//
	}
public:
	SharedPtr<TunnelRule> rule;
	SharedPtr<RecursiveMutex> mutex;
	std::vector<SharedPtr<Filter> > filters;
	unsigned long long acceptedConnectionNumb;
};

//////////////////////////////////////////////////////////////////////////

namespace {

	template<typename Exception>
	void ReportException(
				TunnelRule::ErrorsTreatment treatment,
				const Exception &ex,
				const char *messageBase = nullptr) {
		String exText;
		ConvertString(ex.GetWhat(), exText);
		std::string message;
		if (!messageBase) {
			message = exText.GetCStr();
		} else {
			Format format(messageBase);
			format % exText.GetCStr();
			message = format.str();
		}
		switch (treatment) {
			case TunnelRule::ERRORS_TREATMENT_INFO:
				Log::GetInstance().AppendInfo(message);
				break;
			case TunnelRule::ERRORS_TREATMENT_WARN:
				Log::GetInstance().AppendWarn(message);
				break;
			default:
				assert(false);
			case TunnelRule::ERRORS_TREATMENT_ERROR:
				Log::GetInstance().AppendError(message);
				break;
		}
	}

}


//////////////////////////////////////////////////////////////////////////

class ServerWorker::Implementation : private boost::noncopyable {

private:

	class TunnelRemover;
	
	struct ByInstance {
		//...//
	};
	struct ByRule {
		//...//
	};
	struct ByUuid {
		//...//
	};
	struct ByEndpoint {
		//...//
	};

	typedef TunnelEx::AcceptHandler<boost::shared_ptr<RuleInfo> > AcceptHandler;
	struct EndpointAcceptorHandler {
		explicit EndpointAcceptorHandler(
					const WString &enpointUuidIn,
					boost::shared_ptr<AcceptHandler> handlerIn)
				: enpointUuid(enpointUuidIn),
				handler(handlerIn) {
			//...//
		}
		WString enpointUuid;
		boost::shared_ptr<AcceptHandler> handler;
	};
	typedef boost::multi_index_container<
			EndpointAcceptorHandler,
			mi::indexed_by<
				mi::hashed_unique<
					mi::tag<ByEndpoint>,
					mi::member<
						EndpointAcceptorHandler,
						WString,
						&EndpointAcceptorHandler::enpointUuid>,
					Helpers::StringHasher<WString> > > >
		EndpointAcceptorHandlers;
	typedef EndpointAcceptorHandlers::index<ByEndpoint>::type
		EndpointAcceptorHandlerByEndpoint;

	struct ActiveRule {
		explicit ActiveRule(WString uuid, bool isTunnel)
				: uuid(uuid),
				isTunnel(isTunnel) {
			//...//
		}
		WString uuid;
		EndpointAcceptorHandlers acceptHandlers;
		bool isTunnel;
	};
	typedef boost::multi_index_container<
			ActiveRule,
			mi::indexed_by<
				mi::hashed_unique<
					mi::tag<ByUuid>,
					mi::member<
						ActiveRule,
						WString,
						&ActiveRule::uuid>,
					Helpers::StringHasher<WString> > > >
		ActiveRules;
	typedef ActiveRules::index<ByUuid>::type RuleByUuid;
	
	struct ActiveTunnel {
		explicit ActiveTunnel(boost::shared_ptr<Tunnel> tunnel)
				: tunnel(tunnel) {
			//...//
		}
		Instance::Id GetInstanceId() const {
			return tunnel->GetInstanceId();
		}
		const WString & GetRuleUuid() const {
			return tunnel->GetRule().GetUuid();
		}
		boost::shared_ptr<Tunnel> tunnel;
	};
	typedef boost::multi_index_container<
			ActiveTunnel,
			mi::indexed_by<
				mi::ordered_unique<
					mi::tag<ByInstance>,
					mi::const_mem_fun<
						ActiveTunnel,
						Instance::Id,
						&ActiveTunnel::GetInstanceId> >,
				mi::hashed_non_unique<
					mi::tag<ByRule>,
					mi::const_mem_fun<
						ActiveTunnel,
						const WString &,
						&ActiveTunnel::GetRuleUuid>,
					Helpers::StringHasher<WString> > > >
		ActiveTunnels;
	typedef ActiveTunnels::index<ByInstance>::type ActiveTunnelByInstance;
	typedef ActiveTunnels::index<ByRule>::type ActiveTunnelByRule;

	typedef boost::multi_index_container<
			TunnelRule,
			mi::indexed_by<
				mi::hashed_unique<
					mi::tag<ByUuid>,
					mi::const_mem_fun<
						Rule,
						const WString &,
						&Rule::GetUuid>,
					Helpers::StringHasher<WString> > > >
		IndexedTunnelRuleSet;
	typedef IndexedTunnelRuleSet::index<ByUuid>::type TunnelRuleByUuid;

	struct ActiveService {
		explicit ActiveService(boost::shared_ptr<Service> service)
				: service(service) {
			//...//
		}
		Instance::Id GetInstanceId() const {
			return service->GetInstanceId();
		}
		const WString & GetRuleUuid() const {
			return service->GetRule().GetUuid();
		}
		boost::shared_ptr<Service> service;
	};
	typedef boost::multi_index_container<
			ActiveService,
			mi::indexed_by<
				mi::ordered_unique<
					mi::tag<ByInstance>,
					mi::const_mem_fun<
						ActiveService,
						Instance::Id,
						&ActiveService::GetInstanceId> >,
				mi::hashed_non_unique<
					mi::tag<ByRule>,
					mi::const_mem_fun<
						ActiveService,
						const WString &,
						&ActiveService::GetRuleUuid>,
					Helpers::StringHasher<WString> > > >
		ActiveServices;
	typedef ActiveServices::index<ByInstance>::type ActiveServiceByInstance;
	typedef ActiveServices::index<ByRule>::type ActiveServiceByRule;

	typedef ACE_RW_Mutex ActiveTunnelsMutex;
	typedef ACE_Read_Guard <ActiveTunnelsMutex> ActiveTunnelsReadLock;
	typedef ACE_Write_Guard<ActiveTunnelsMutex> ActiveTunnelsWriteLock;

	typedef ACE_RW_Mutex RulesMutex;
	typedef ACE_Read_Guard<RulesMutex> RulesReadLock;
	typedef ACE_Write_Guard<RulesMutex> RulesWriteLock;

	typedef ACE_RW_Mutex ActiveServicesMutex;
	typedef ACE_Read_Guard<ActiveServicesMutex> ActiveServicesReadLock;
	typedef ACE_Write_Guard<ActiveServicesMutex> ActiveServicesWriteLock;

	typedef ACE_Thread_Mutex ServerStopMutex;
	typedef ACE_Guard<ServerStopMutex> ServerStopLock;
	typedef ACE_Thread_Condition<ServerStopMutex> ServerStopCondition;

	struct RuleUpdatingState : private boost::noncopyable {

		typedef ACE_Thread_Mutex Mutex;
		typedef ACE_Guard<Mutex> Lock;
		typedef ACE_Thread_Condition<Mutex> Condition;
		
		RuleUpdatingState()
				: newRuleCondition(ruleMutex),
				resultCondition(resultMutex),
				rule(nullptr) {
			//...//
		}
		
		Mutex ruleMutex;
		Condition newRuleCondition;
		Mutex resultMutex;
		Condition resultCondition;
		
		const Rule *rule;
		
		boost::optional<bool> result;
		AutoPtr<LocalException> exception;

	};

	struct TunnelOpeningState : private boost::noncopyable {

		typedef ACE_Thread_Mutex Mutex;
		typedef ACE_Guard<Mutex> Lock;
		typedef ACE_Thread_Condition<Mutex> Condition;

		struct NewConnection {

			NewConnection() {
				//...//
			}

			explicit NewConnection (
						boost::shared_ptr<RuleInfo> ruleInfoIn, 
						AutoPtr<Connection> &connectionIn)
					: ruleInfo(ruleInfoIn),
					connection(connectionIn) {
				assert(bool(ruleInfo) == bool(connection));
			}

			operator bool() const throw() {
				assert(bool(ruleInfo) == bool(connection));
				return bool(connection);
			}

			boost::shared_ptr<RuleInfo> ruleInfo;
			SharedPtr<Connection> connection;

		};

		typedef std::list<NewConnection> NewConnections;
		typedef std::list<boost::shared_ptr<Tunnel>> Tunnels;
		
		TunnelOpeningState()
				: condition(mutex),
				threadsCount(0) {
			//...//
		}
		
		Mutex mutex;
		Condition condition;

		NewConnections newConnections;
		Tunnels tunnels;

		volatile long threadsCount;

	};

	enum ThreadGroup {
		TG_TUNNEL_OPENING,
		TG_PROACTOR,
		TG_REACTOR,
		TG_UPDATING
	};

public:

	explicit Implementation(ServerWorker &myInterface, Server::Ref server) 
			: m_myInterface(myInterface),
			m_server(server),
			//! @todo: from boost::io_service::io_service(), check in new versions.
			m_proactor(new ACE_WIN32_Proactor(std::numeric_limits<size_t>::max()), true),
			m_isServicesThreadLaunched(false),
			m_isRulesCheckThreadLaunched(false),
			m_isDestructionMode(false),
			m_ruleSetLicense(&m_ruleSetLicenseState),
			m_tunnelLicense(&m_tunnelLicenseState),
			m_rwSplitLicense(&m_rwSplitLicenseState),
			m_serverStopCondition(m_serverStopMutex) {
		
		static_assert(
			sizeof(DWORD) >= sizeof(size_t),
			"Wrong max thread number for proactor's CreateIoCompletionPort.");

		Log::GetInstance().AppendDebug("Creating server...");

		{
			const bool isServerOs = IsServerOs();
			if (!Licensing::ExeLicense().IsFeatureAvailable(true)) {
				Log::GetInstance().AppendWarn(
					"Failed to start service. Please activate your product copy."
						" Purchase a License at http://" TUNNELEX_DOMAIN "/order"
						" or get free trial at http://" TUNNELEX_DOMAIN "/order/trial.");
				throw LocalException(L"Failed to start service, License Upgrade required");
			} else if (isServerOs && !Licensing::ServiceStartLicense().IsFeatureAvailable(isServerOs)) {
				Log::GetInstance().AppendWarn(
					"Failed to start service at server operation system."
						" The functionality you have requested requires"
						" a License Upgrade. Please purchase a License that"
						" will enable this feature at http://" TUNNELEX_DOMAIN "/order"
						" or get free trial at http://" TUNNELEX_DOMAIN "/order/trial.");
				throw LocalException(
					L"Failed to start service at server operation system, License Upgrade required");
			}
		}
		m_threadManager.spawn_n(
			openingTunnelMinThreadCount,
			&TunnelOpeningThread,
			this,
			THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED,
			ACE_DEFAULT_THREAD_PRIORITY,
			TG_TUNNEL_OPENING);
		m_threadManager.spawn_n(
			proactorMinThreadCount,
			&ProactorEventLoopThread,
			this,
			THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED,
			ACE_DEFAULT_THREAD_PRIORITY,
			TG_PROACTOR);
		m_threadManager.spawn(
			&ReactorEventLoopThread,
			this,
			THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED,
			0,
			0,
			ACE_DEFAULT_THREAD_PRIORITY,
			TG_REACTOR);
		m_threadManager.spawn(
			&UpdatingThread,
			this,
			THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED,
			0,
			0,
			ACE_DEFAULT_THREAD_PRIORITY,
			TG_UPDATING);
	}

	~Implementation() {

		Log::GetInstance().AppendDebug("Destroying server...");

		verify(Interlocked::CompareExchange(m_isDestructionMode, 1, 0) == 0);
		
		m_tunnelOpeningState.condition.broadcast();
		m_ruleUpdatingState.newRuleCondition.broadcast();

		{
			ServerStopLock stopLock(m_serverStopMutex);
			{
				RulesWriteLock lock(m_rulesMutex);
				m_tunnelRulesToCheck.clear();
				m_activeRules.clear();
			}
			{
				ActiveServicesWriteLock lock(m_activeServicesMutex);
				m_activeServices.clear();
			}
			m_serverStopCondition.broadcast();
		}

		m_reactor.end_reactor_event_loop();
		m_threadManager.wait_grp(TG_REACTOR);
		m_threadManager.wait_grp(TG_TUNNEL_OPENING);
		
		{
			ActiveTunnelsWriteLock lock(m_activeTunnelsMutex);
			m_activeTunnels.clear();
		}
		m_proactor.proactor_end_event_loop();

		m_threadManager.wait();

	}

public:

	//! @todo: check time spending, maybe will more faster to cache this number
	size_t GetOpenedEndpointsNumber() const {
		size_t result = 0;
		RulesReadLock lock(m_rulesMutex);
		foreach (const ActiveRules::value_type &r, m_activeRules) {
			result += r.acceptHandlers.size();
		}
		return result;
	}
	
	size_t GetTunnelsNumber() const {
		ActiveTunnelsReadLock lock(m_activeTunnelsMutex);
		return m_activeTunnels.size();
	}

	AutoPtr<EndpointAddress> GetRealOpenedEndpointAddress(
				const WString &ruleUuid,
				const WString &endpointUuid)
			const {
		RulesReadLock lock(m_rulesMutex);
		const RuleByUuid &ruleIndex = m_activeRules.get<ByUuid>();
		const RuleByUuid::const_iterator ruleInputsPos
			= ruleIndex.find(ruleUuid);
		if (ruleInputsPos == m_activeRules.end()) {
			throw LogicalException(
				(WFormat(L"Failed to find rule %1% in active list.") % ruleUuid.GetCStr()).str().c_str());
		}
		const EndpointAcceptorHandlerByEndpoint &acceptorIndex
			= ruleInputsPos->acceptHandlers.get<ByEndpoint>();
		const EndpointAcceptorHandlerByEndpoint::const_iterator handlerPos
			= acceptorIndex.find(endpointUuid);
		if (handlerPos == acceptorIndex.end()) {
			throw LogicalException(
				(WFormat(L"Rule has not any opened endpoints.") % ruleUuid.GetCStr()).str().c_str());
		}
		return handlerPos->handler->GetAcceptor().GetLocalAddress();
	}

	bool Update(const Rule &rule) {
		
		RuleUpdatingState::Lock resultLock(m_ruleUpdatingState.resultMutex);
		{
			RuleUpdatingState::Lock ruleLock(m_ruleUpdatingState.ruleMutex);
			assert(!m_isDestructionMode);
			m_ruleUpdatingState.rule = &rule;
			m_ruleUpdatingState.newRuleCondition.signal();
		}
		m_ruleUpdatingState.result.reset();
		assert(!m_ruleUpdatingState.exception);
		
		m_ruleUpdatingState.resultCondition.wait();

		if (m_ruleUpdatingState.exception) {
			const AutoPtr<const LocalException> exception(m_ruleUpdatingState.exception);
			const LicenseException *licenseException
				= dynamic_cast<const LicenseException *>(exception.Get());
			throw *(licenseException ? licenseException : exception.Get());
		} else if (!m_ruleUpdatingState.result) {
			throw LocalException(L"Unknown system error occurred at rule updating");
		} else {
			return *m_ruleUpdatingState.result;
		}
	
	}

	bool DeleteRule(const WString &uuid) {
		
		RulesWriteLock lock(m_rulesMutex);
		if (Log::GetInstance().IsDebugRegistrationOn()) {
			Log::GetInstance().AppendDebug(
				"Deleting rule %1%...",
				ConvertString<String>(uuid).GetCStr());
		}

		bool wasDeleted = false;
		bool wasDeletedFromActive = false;
		ActiveRules activeRules(m_activeRules);
		bool isTunnel = false;

		{
			const RuleByUuid &index = activeRules.get<ByUuid>();
			const RuleByUuid::iterator pos(index.find(uuid));
			if (pos == index.end()) {
				Log::GetInstance().AppendDebug(
					"Failed to find rule %1% in active list.",
					ConvertString<String>(uuid).GetCStr());
			} else {
				isTunnel = pos->isTunnel;
				activeRules.erase(pos);
				if (Log::GetInstance().IsDebugRegistrationOn()) {
					Log::GetInstance().AppendDebug(
						"The %1% rule %2% has been removed from active list.",
						isTunnel ? "tunnel" : "service",
						ConvertString<String>(uuid).GetCStr());
				}
				wasDeleted = true;
				wasDeletedFromActive = true;
			}
		}

		std::auto_ptr<ActiveServicesWriteLock> activeServicesLock;
		std::auto_ptr<ActiveServices> activeServices;
		boost::shared_ptr<Service> service;
		if (wasDeletedFromActive && !isTunnel) {
			activeServicesLock.reset(new ActiveServicesWriteLock(m_activeServicesMutex));
			activeServices.reset(new ActiveServices(m_activeServices));
			const ActiveServiceByRule::iterator pos
				= activeServices->get<ByRule>().find(uuid);
			assert(pos != activeServices->get<ByRule>().end());
			if (pos != activeServices->get<ByRule>().end()) {
				service = pos->service;
				activeServices->get<ByRule>().erase(pos);
			}
		}

		std::auto_ptr<IndexedTunnelRuleSet> tunnelRulesToCheck;
		if (	m_tunnelRulesToCheck.get<ByUuid>().find(uuid)
				!= m_tunnelRulesToCheck.get<ByUuid>().end()) {
			assert(isTunnel);
			tunnelRulesToCheck.reset(new IndexedTunnelRuleSet(m_tunnelRulesToCheck));
			const TunnelRuleByUuid::iterator pos(
				tunnelRulesToCheck->get<ByUuid>().find(uuid));
			assert(pos != tunnelRulesToCheck->get<ByUuid>().end());
			tunnelRulesToCheck->erase(pos);
			if (Log::GetInstance().IsDebugRegistrationOn()) {
				Log::GetInstance().AppendDebug(
					"The rule %1% has been removed from checking list.",
					ConvertString<String>(uuid).GetCStr());
			}
			wasDeleted = true;
		}

		if (wasDeletedFromActive) {
			swap(activeRules, m_activeRules);
			if (activeServices.get()) {
				swap(*activeServices, m_activeServices);
				activeServicesLock.reset();
				assert(service);
				if (service) {
					service->Stop();
					service.reset();
				}
			}
		}

		if (tunnelRulesToCheck.get()) {
			swap(*tunnelRulesToCheck, m_tunnelRulesToCheck);
		}

		return wasDeleted;

	}

	bool IsRuleEnabled(const WString &uuid) const {
		RulesReadLock lock(m_rulesMutex);
		const RuleByUuid &index = m_activeRules.get<ByUuid>();
		return index.find(uuid) != index.end();
	}

	void OpenTunnel(boost::shared_ptr<RuleInfo> ruleInfo, Acceptor &acceptor) {
		if (!acceptor.TryToAttach()) {
			Log::GetInstance().AppendDebug(
				"Incoming connection detected, initializing tunnel...");
			AutoPtr<Connection> inConnection = acceptor.Accept();
			if (!inConnection->IsOneWay()) {
				const unsigned long limit
					= ruleInfo->rule->GetAcceptedConnectionsLimit();
				if (limit > 0 && ++ruleInfo->acceptedConnectionNumb >= limit) {
					if (Log::GetInstance().IsDebugRegistrationOn()) {
						Log::GetInstance().AppendDebug(
							"Rule %1% will be deleted as max connection number has been exceed for it.",
							ConvertString<String>(ruleInfo->rule->GetUuid()).GetCStr());
					}
					DeleteRule(ruleInfo->rule->GetUuid());
				}
				OpenTunnel(ruleInfo, inConnection);
			} else if (Log::GetInstance().IsDebugRegistrationOn()) {
				const AutoPtr<const EndpointAddress> remoteAddress(
					inConnection->GetRemoteAddress());
				Log::GetInstance().AppendDebug(
					"One-way connection %1% -> %2% is completed.",
					remoteAddress
						?	ConvertString<String>(remoteAddress->GetResourceIdentifier()).GetCStr()
						:	"<unknown>",
					ConvertString<String>(
							inConnection->GetRuleEndpointAddress()->GetResourceIdentifier())
						.GetCStr());
			}
		}
	}

	void OpenTunnel(
				boost::shared_ptr<RuleInfo> ruleInfo,
				AutoPtr<Connection> inConnection) {
		size_t newConnectionsInQueue;
		size_t tunnelsToSwitchInQueue;
		{
			TunnelOpeningState::Lock lock(m_tunnelOpeningState.mutex);
			newConnectionsInQueue = m_tunnelOpeningState.newConnections.size();
			tunnelsToSwitchInQueue = m_tunnelOpeningState.tunnels.size();
			m_tunnelOpeningState.newConnections.push_back(
				TunnelOpeningState::NewConnection(ruleInfo, inConnection));
			m_tunnelOpeningState.condition.signal();
		}
		StartTunnelOpeningThread(newConnectionsInQueue, tunnelsToSwitchInQueue);
	}

	SharedPtr<Connection> CreateConnection(
				const RuleEndpoint &endpoint,
				SharedPtr<const EndpointAddress> address,
				const wchar_t *const type)
			const {
		try {
			AutoPtr<Connection> result
				= address->CreateLocalConnection(endpoint, address);
			return SharedPtr<Connection>(result);
		} catch (const TunnelEx::ConnectionOpeningException &ex) {
			typedef ConnectionOpeningExceptionImpl<
					SourceConnectionOpeningException>
				ExceptionImpl;
			throw ExceptionImpl(address, ex, type);
		}
	}

	void CloseTunnel(Instance::Id tunnelId) {

		if (m_isDestructionMode) {
			return;
		}

		boost::shared_ptr<Tunnel> tunnel;
		{
			ActiveTunnelsWriteLock lock(m_activeTunnelsMutex);
			ActiveTunnelByInstance &index
				= m_activeTunnels.get<ByInstance>();
			const ActiveTunnelByInstance::const_iterator pos
				= index.find(tunnelId);
			if (pos == index.end()) {
				return;
			}
			tunnel = pos->tunnel;
			index.erase(pos);
		}

		if (!tunnel->IsDead()) {
			if (tunnel->IsSetupFailed()) {
				// tunnel object will be closed if no destinations will be opened,
				// even if all other connections already closed.
				SwitchTunnel(tunnel);
			} else if (tunnel->IsStatic()) {
				TunnelRule rule(tunnel->GetRule());
				tunnel.reset();
				{
					if (Log::GetInstance().IsDebugRegistrationOn()) {
						Log::GetInstance().AppendDebug(
							"Deleting rule %1%...",
							ConvertString<String>(rule.GetUuid()).GetCStr());
					}
					RulesWriteLock lock(m_rulesMutex);
					m_activeRules.get<ByUuid>().erase(rule.GetUuid());
				}
				Log::GetInstance().AppendDebug(
					"Trying to reopen rule %1% for static tunnel...",
					ConvertString<String>(rule.GetUuid()).GetCStr());
				Update(rule);
			} else {
				tunnel->MarkAsDead();
			}
		}
		
		if (!tunnel->IsDead()) {
			return;
		}

		size_t newConnectionsInQueue;
		size_t tunnelsToSwitchInQueue;
		{
			TunnelOpeningState::Lock lock(m_tunnelOpeningState.mutex);
			newConnectionsInQueue = m_tunnelOpeningState.newConnections.size();
			tunnelsToSwitchInQueue = m_tunnelOpeningState.tunnels.size();
			m_tunnelOpeningState.tunnels.push_back(tunnel);
			m_tunnelOpeningState.condition.signal();
		}
		StartTunnelOpeningThread(newConnectionsInQueue, tunnelsToSwitchInQueue);

	}

	ACE_Proactor & GetProactor() {
		return m_proactor;
	}

	ACE_Reactor & GetReactor() {
		return m_reactor;
	}

	Server::Ref GetServer() {
		return m_server;
	}

private:

	void OpenRule(
				const ServiceRule &sourceRule,
				ActiveServices &services)
			const {
		SharedPtr<const ServiceRule> rule(new ServiceRule(sourceRule));
		ActiveServices servicesTmp(services);
		const size_t servicesNumb = rule->GetServices().GetSize();
		assert(servicesNumb > 0);
		for (size_t i = 0; i < servicesNumb; ++i) {
			const ServiceRule::Service &s = rule->GetServices()[i];
			AutoPtr<Service> servicePtr
				= ModulesFactory::GetInstance().CreateService(rule, s);
			if (rule->IsEnabled()) {
				if (!servicePtr->IsStarted()) {
					servicePtr->Start();
				}
				boost::shared_ptr<Service> service(servicePtr.Get());
				servicePtr.Release();
				servicesTmp.insert(ActiveService(service));
			} else {
				servicePtr->Stop();
			}
		}
		if (servicesTmp.size() != services.size()) {
			assert(servicesTmp.size() > services.size());
			servicesTmp.swap(services);
		}
	}

	void OpenRule(
				const TunnelRule &rule,
				ActiveRule &activeRule,
				ActiveTunnels &tunnels,
				std::vector<boost::shared_ptr<Tunnel> > &newTunnels,
				IndexedTunnelRuleSet &ruleToCheck)
			const {
	
		const boost::shared_ptr<RuleInfo> ruleInfo(new RuleInfo);
		ruleInfo->rule.Reset(new TunnelRule(rule));
		ruleInfo->mutex.Reset(new RecursiveMutex);
		ModulesFactory::GetInstance().CreateFilters(
			ruleInfo->rule,
			ruleInfo->mutex,
			ruleInfo->filters);

		RecursiveLock lock(*ruleInfo->mutex); // lock rule for acceptors and accept handlers
		const RuleEndpointCollection &inputs = ruleInfo->rule->GetInputs();
		const size_t inputsNumb = inputs.GetSize();
		for (unsigned int inputI = 0; inputI < inputsNumb; ++inputI) {
			const RuleEndpoint &endpoint = inputs[inputI];
			SharedPtr<const EndpointAddress> acceptorAddress;
			SharedPtr<const EndpointAddress> readerAddress;
			SharedPtr<const EndpointAddress> writerAddress;
			if (!endpoint.IsCombined()) {
				readerAddress = endpoint.GetReadAddress();
				writerAddress = endpoint.GetWriteAddress();
				if (endpoint.GetReadWriteAcceptor() != Endpoint::ACCEPTOR_NONE) {
					acceptorAddress = endpoint.GetReadWriteAcceptor() == Endpoint::ACCEPTOR_READER
						?	readerAddress
						:	writerAddress;
					assert(
						acceptorAddress == readerAddress
						|| endpoint.GetReadWriteAcceptor() == Endpoint::ACCEPTOR_WRITER);
				}
			} else {
				readerAddress = writerAddress = endpoint.GetCombinedAddress();
				if (endpoint.IsCombinedAcceptor()) {
					acceptorAddress = readerAddress;
				}
			}
			if (acceptorAddress && !acceptorAddress->IsHasMultiClientsType()) {
				Format message(
					"Failed to open endpoint for tunnel entrance %1%: endpoint with such type could not be accessible.");
				message
					% ConvertString<String>(acceptorAddress->GetResourceIdentifier())
					.GetCStr();
				Log::GetInstance().AppendError(message.str());
				return;
			}
			Log::GetInstance().AppendDebug(
				"Number of currently open tunnels: %1%.",
				tunnels.size());
			if (acceptorAddress) {
				try {
					const boost::shared_ptr<AcceptHandler> handler(
						AcceptHandler::CreateInstance(
							m_myInterface,
							ruleInfo,
							endpoint,
							acceptorAddress));
					const EndpointAcceptorHandler endpointAcceptorHandler(
						endpoint.GetUuid(),
						handler);
					assert(
						activeRule.acceptHandlers.get<ByEndpoint>().find(endpointAcceptorHandler.enpointUuid)
						== activeRule.acceptHandlers.get<ByEndpoint>().end());
					activeRule.acceptHandlers.insert(endpointAcceptorHandler);
				} catch (const TunnelEx::ConnectionException &ex) {
					Format message(
						"Failed to open endpoint %1% for incoming connections (error: \"%2%\").");
					message
						% ConvertString<String>(acceptorAddress->GetResourceIdentifier()).GetCStr()
						% ConvertString<String>(ex.GetWhat()).GetCStr();
					Log::GetInstance().AppendError(message.str());
				}
			} else if (!m_tunnelLicense.IsFeatureAvailable(tunnels.size() + 1)) {
				Format message(
					"Failed to open new connection: too many connections - %1%."
						" The functionality you have requested requires"
						" a License Upgrade. Please purchase a License that"
						" will enable this feature at http://" TUNNELEX_DOMAIN "/order"
						" or get free trial at http://" TUNNELEX_DOMAIN "/order/trial.");
				message % tunnels.size();
				Log::GetInstance().AppendWarn(message.str().c_str());
				throw LocalException(
					L"Failed to open new connection, License Upgrade required");
			} else {
				try {
					SharedPtr<Connection> reader
						= CreateConnection(
							endpoint,
							readerAddress,
							readerAddress == writerAddress ? L"combined" : L"read");
					SharedPtr<Connection> writer = readerAddress == writerAddress
						?	reader
						:	CreateConnection(endpoint, writerAddress, L"write");
					boost::shared_ptr<Tunnel> tunnel(
						new Tunnel(true, m_myInterface, ruleInfo->rule, reader, writer));
					assert(
						tunnels.get<ByInstance>().find(tunnel->GetInstanceId())
						== tunnels.get<ByInstance>().end());
					newTunnels.push_back(tunnel);
					tunnels.insert(ActiveTunnel(tunnel));
				} catch (const TunnelEx::ConnectionException &ex) {
					ReportException(
						ruleInfo->rule->GetErrorsTreatment(),
						ex,
						"Failed to open endpoint for tunnel entrance: %1%.");
#					ifdef DEV_VER
						if (ruleToCheck.find(ruleInfo->rule->GetUuid().GetCStr()) != ruleToCheck.end()) {
							Format message(
								"Issue TEX-634: rule %1% already added to checking list.");
							message % ConvertString<String>(ruleInfo->rule->GetUuid()).GetCStr();
							Log::GetInstance().AppendWarn(message.str());
						}
#					endif
					ruleToCheck.insert(*ruleInfo->rule);
				}
			}
		}
		
	}

	bool UpdateImplementation(const TunnelRule &rule) {
		
		if (Log::GetInstance().IsDebugRegistrationOn()) {
			Log::GetInstance().AppendDebug(
				"Updating tunnel rule %1%...",
				ConvertString<String>(rule.GetUuid()).GetCStr());
		}

		RulesWriteLock lock(m_rulesMutex);

		{
			const TunnelRuleByUuid &index = m_tunnelRulesToCheck.get<ByUuid>();
			const TunnelRuleByUuid::iterator pos(index.find(rule.GetUuid()));
			if (pos != index.end()) {
				if (Log::GetInstance().IsDebugRegistrationOn()) {
					Format message("Removing tunnel rule %1% from checking list before updating...");
					message % ConvertString<String>(rule.GetUuid()).GetCStr();
					Log::GetInstance().AppendDebug(message.str());
				}
				m_tunnelRulesToCheck.erase(pos);
			}
		}

		// I have to remove exists acceptors before open new.
		const RuleByUuid &rulesIndex = m_activeRules.get<ByUuid>();
		const RuleByUuid::iterator oldRuleAcceptorsPos
			= rulesIndex.find(rule.GetUuid());
		const bool isNewRule = oldRuleAcceptorsPos == rulesIndex.end();
		if (!isNewRule) {
			m_activeRules.erase(oldRuleAcceptorsPos);
		} else if (Log::GetInstance().IsDebugRegistrationOn()) {
			Format message(
				"Tunnel rule %1% is not found in active list, new rule will be inserted.");
			message % ConvertString<String>(rule.GetUuid()).GetCStr();
			Log::GetInstance().AppendDebug(message.str());
		}
		{
			ActiveTunnelByRule &index = m_activeTunnels.get<ByRule>();
			if (index.find(rule.GetUuid()) != index.end()) {
				boost::shared_ptr<Tunnel> tunnel;
				{
					ActiveTunnelsWriteLock lock(m_activeTunnelsMutex);
					const ActiveTunnelByRule::const_iterator pos
						= index.find(rule.GetUuid());
					if (pos != index.end()) {
						tunnel = pos->tunnel;
						index.erase(pos);
					}
				}
				if (tunnel) {
					tunnel->MarkAsDead();
				}
			}
		}

		size_t openedTunnelsNumb = 0;
		ActiveRule newRule(rule.GetUuid(), true);
		const size_t allowedActiveRulesCheckNumber = m_activeRules.size() + 1;
		std::vector<boost::shared_ptr<Tunnel>> newTunnels;
		if (	rule.IsEnabled()
				&&	m_ruleSetLicense.IsFeatureAvailable(allowedActiveRulesCheckNumber)) {
			// locking, so Close method now will be wait until new tunnels
			// will be added to the collection.
			ActiveTunnelsWriteLock lock(m_activeTunnelsMutex);
			ActiveTunnels tmpActiveTunnels(m_activeTunnels);
			IndexedTunnelRuleSet tmpRulesToCheck(m_tunnelRulesToCheck);
			OpenRule(rule, newRule, tmpActiveTunnels, newTunnels, tmpRulesToCheck);
			openedTunnelsNumb = tmpActiveTunnels.size() - m_activeTunnels.size();
			assert(
				m_activeRules.get<ByUuid>().find(newRule.uuid)
				== m_activeRules.get<ByUuid>().end());
			m_activeRules.insert(newRule);
			// tunnels updating after rule insertion for exceptional safety
			tmpActiveTunnels.swap(m_activeTunnels);
			m_tunnelRulesToCheck.swap(tmpRulesToCheck);
			if (m_tunnelRulesToCheck.size() > 0 && !m_isRulesCheckThreadLaunched) {
				m_isRulesCheckThreadLaunched = true;
				m_threadManager.spawn(
					&RulesCheckThread,
					this,
					THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED,
					0,
					0,
					ACE_DEFAULT_THREAD_PRIORITY,
					TG_UPDATING);
			}
			if (Log::GetInstance().IsDebugRegistrationOn()) {
				Format message(
					!isNewRule
						?	"The tunnel rule %1% has been updated."
						:	"The new tunnel rule %1% has been inserted.");
				message % ConvertString<String>(rule.GetUuid()).GetCStr();
			}
		} else if (rule.IsEnabled()) {
			Format message(
				"Failed to activate one or more rules: too many rules activated - %1%."
					" The functionality you have requested requires"
					" a License Upgrade. Please purchase a License that"
					" will enable this feature at http://" TUNNELEX_DOMAIN "/order"
					" or get free trial at http://" TUNNELEX_DOMAIN "/order/trial.");
			message % m_activeRules.size();
			Log::GetInstance().AppendWarn(message.str());
			throw LicenseException(
				L"Failed to activate one or more rules, License Upgrade required");
		}
		foreach (const boost::shared_ptr<Tunnel> &tunnel, newTunnels) {
			try {
				tunnel->StartSetup();
			} catch (...) {
				tunnel->MarkAsDead();
				CloseTunnel(tunnel->GetInstanceId());
				throw;
			}
		}
		return
			!rule.IsEnabled()
			|| !m_ruleSetLicense.IsFeatureValueAvailable(allowedActiveRulesCheckNumber)
			|| rule.GetInputs().GetSize() == newRule.acceptHandlers.size() + openedTunnelsNumb;

	}

	bool UpdateImplementation(const ServiceRule &rule) {

		if (Log::GetInstance().IsDebugRegistrationOn()) {
			Log::GetInstance().AppendDebug(
				"Updating service rule %1%...",
				ConvertString<String>(rule.GetUuid()).GetCStr());
		}

		if (rule.GetServices().GetSize() == 0) {
			Log::GetInstance().AppendDebug(
				"Service rule %1% is empty.",
				ConvertString<String>(rule.GetUuid()).GetCStr());
			return false;
		}

		RulesWriteLock rulesLock(m_rulesMutex);
		ActiveServicesWriteLock servicesLock(m_activeServicesMutex);

		{
			ActiveServiceByRule &index = m_activeServices.get<ByRule>();
			if (Log::GetInstance().IsDebugRegistrationOn()) {
				if (index.find(rule.GetUuid()) != index.end()) {
					Format message("Stopping services for rule %1%...");
					message % ConvertString<String>(rule.GetUuid()).GetCStr();
					Log::GetInstance().AppendDebug(message.str());
				} else {
					Format message(
						"Service rule %1% is not found in active list, new rule will be inserted.");
					message % ConvertString<String>(rule.GetUuid()).GetCStr();
					Log::GetInstance().AppendDebug(message.str());
				}
			}
			index.erase(rule.GetUuid());
		}

		const size_t allowedActiveRulesCheckNumber = m_activeRules.size() + 1;
		if (	rule.IsEnabled()
				&&	m_ruleSetLicense.IsFeatureAvailable(allowedActiveRulesCheckNumber)) {
			ActiveRules activeRules(m_activeRules);
			if (!m_isServicesThreadLaunched) {
				// Will be completed automatically at error in OpenRule.
				m_isServicesThreadLaunched = true;
				m_threadManager.spawn(
					&ServiceThread,
					this,
					THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED,
					0,
					0,
					ACE_DEFAULT_THREAD_PRIORITY,
					TG_UPDATING);
			}
			activeRules.insert(ActiveRule(rule.GetUuid(), false));
			const size_t oldActiveServicesCount = m_activeServices.size();
			OpenRule(rule, m_activeServices);
			if (oldActiveServicesCount != m_activeServices.size()) {
				assert(oldActiveServicesCount < m_activeServices.size());
				swap(activeRules, m_activeRules);
			}
		} else if (rule.IsEnabled()) {
			Format message(
				"Failed to activate one or more rules: too many rules activated - %1%."
					" The functionality you have requested requires"
					" a License Upgrade. Please purchase a License that"
					" will enable this feature at http://" TUNNELEX_DOMAIN "/order"
					" or get free trial at http://" TUNNELEX_DOMAIN "/order/trial.");
			message % m_activeRules.size();
			Log::GetInstance().AppendWarn(message.str());
			throw LicenseException(
				L"Failed to activate one or more rules, License Upgrade required");
		}

		return true;

	}

	static ACE_THR_FUNC_RETURN UpdatingThread(void *param) {

		Log::GetInstance().AppendDebug("Started updating thread.");
	
		Implementation &instance = *static_cast<Implementation *>(param);
		typedef RuleUpdatingState::Lock Lock;
		RuleUpdatingState &state = instance.m_ruleUpdatingState;

		while (!instance.m_isDestructionMode) {

			Lock ruleLock(state.ruleMutex);
	
			if (state.rule) {
				Lock resultLock(state.resultMutex);
				assert(!state.exception);
				assert(!state.result);
				try {
					if (dynamic_cast<const TunnelRule *>(state.rule)) {
						state.result = instance.UpdateImplementation(
							*boost::polymorphic_downcast<const TunnelRule *>(
								instance.m_ruleUpdatingState.rule));
					} else {
						assert(dynamic_cast<const ServiceRule *>(state.rule));
						state.result = instance.UpdateImplementation(
							*boost::polymorphic_downcast<const ServiceRule *>(
								instance.m_ruleUpdatingState.rule));
					}
				} catch (const TunnelEx::LocalException &ex) {
					instance.m_ruleUpdatingState.exception = ex.Clone();
				} catch (const std::exception &ex) {
					instance.m_ruleUpdatingState.exception.Reset(
						new SystemException(ConvertString<WString>(ex.what()).GetCStr()));
				} catch (...) {
					Format message(
						"Unknown system error occurred: %1%:%2%."
							" Please contact product support to resolve this issue."
							" %3% %4%");
					message
						% __FILE__ % __LINE__
						% TUNNELEX_NAME % TUNNELEX_BUILD_IDENTITY;
					Log::GetInstance().AppendFatalError(message.str());
					assert(false);
				}
				state.resultCondition.broadcast();
			}
			
			state.newRuleCondition.wait();

		}

		Log::GetInstance().AppendDebug("Updating thread completed.");
		return 0;

	}

	static ACE_THR_FUNC_RETURN ServiceThread(void *param) {
		
		Log::GetInstance().AppendDebug("Started services thread.");
		Implementation &instance = *static_cast<Implementation *>(param);
		std::auto_ptr<ServerStopLock> lock(
			new ServerStopLock(instance.m_serverStopMutex));

		for ( ; !instance.m_isDestructionMode; ) {

			//! @todo: hardcoded sleep time, move to options or config
			const ACE_Time_Value waitUntilTime
				= ACE_OS::gettimeofday() + ACE_Time_Value(60);
			lock.reset(new ServerStopLock(instance.m_serverStopMutex));
			const int waitResult
				= instance.m_serverStopCondition.wait(&waitUntilTime);
			if (waitResult != -1 || instance.m_isDestructionMode) {
				break;
			}
			assert(errno == ETIME);

			try {
			
				ActiveServicesReadLock lock(instance.m_activeServicesMutex);
				if (!instance.m_activeServices.empty())  {
					foreach (ActiveServices::value_type svc, instance.m_activeServices) {
						try {
							svc.service->DoWork();
						} catch (const TunnelEx::LocalException &ex) {
							Log::GetInstance().AppendFatalError(
								ConvertString<String>(ex.GetWhat()).GetCStr());
						} catch (const std::exception &ex) {
							Format message("Error (std) occurred in service %1%: %2%.");
							message
								% ConvertString<String>(svc.service->GetService().name).GetCStr()
								% ex.what();
							Log::GetInstance().AppendSystemError(message.str().c_str());
						} catch (...) {
							Format message(
								"Unknown system error occurred: %1%:%2%."
									" Please contact product support to resolve this issue."
									" %3% %4%");
							message
								% __FILE__ % __LINE__
								% TUNNELEX_NAME % TUNNELEX_BUILD_IDENTITY;
							Log::GetInstance().AppendFatalError(message.str());
							assert(false);
						}
					}
					continue;
				}
			
			} catch (const TunnelEx::LocalException &ex) {
				Format message("Error occurred in services thread: %1%.");
				message % ConvertString<String>(ex.GetWhat()).GetCStr();
				Log::GetInstance().AppendFatalError(message.str().c_str());
				continue;
			} catch (const std::exception &ex) {
				Format message("Error occurred in services thread: %1%.");
				message % ex.what();
				Log::GetInstance().AppendSystemError(message.str().c_str());
				continue;
			} catch (...) {
				Format message(
					"Unknown system error occurred: %1%:%2%."
						" Please contact product support to resolve this issue."
						" %3% %4%");
				message
					% __FILE__ % __LINE__
					% TUNNELEX_NAME % TUNNELEX_BUILD_IDENTITY;
				Log::GetInstance().AppendFatalError(message.str());
				assert(false);
				continue;
			}
			
			ActiveServicesReadLock lock(instance.m_activeServicesMutex);
			if (instance.m_activeServices.empty()) {
				instance.m_isServicesThreadLaunched = false;
				break;
			}
		
		}
	
		Log::GetInstance().AppendDebug("Services thread completed.");
		return 0;
	
	}

	static ACE_THR_FUNC_RETURN RulesCheckThread(void *param) {
		Log::GetInstance().AppendDebug("Started rules checking thread.");
		Implementation &instance = *static_cast<Implementation *>(param);
		std::auto_ptr<ServerStopLock> lock(
			new ServerStopLock(instance.m_serverStopMutex));
		for ( ; !instance.m_isDestructionMode; ) {
			//! @todo: hardcoded sleep time, move to options or config
			const ACE_Time_Value waitUntilTime
				= ACE_OS::gettimeofday() + ACE_Time_Value(60);
			lock.reset(new ServerStopLock(instance.m_serverStopMutex));
			const int waitResult
				= instance.m_serverStopCondition.wait(&waitUntilTime);
			if (waitResult != -1 || instance.m_isDestructionMode) {
				break;
			}
			assert(errno == ETIME);
			try {
				if (!instance.CheckTunnelRules()) {
					RulesWriteLock lock(instance.m_rulesMutex);
					if (instance.m_tunnelRulesToCheck.empty()) {
						instance.m_isRulesCheckThreadLaunched = false;
						break;
					}
				}
			} catch (const TunnelEx::LocalException &ex) {
				Format message("Error occurred in rule checking thread: %1%.");
				message % ConvertString<String>(ex.GetWhat()).GetCStr();
				Log::GetInstance().AppendFatalError(message.str().c_str());
			} catch (const std::exception &ex) {
				Format message("Error occurred in rule checking thread: %1%.");
				message % ex.what();
				Log::GetInstance().AppendSystemError(message.str().c_str());
			} catch (...) {
				Format message(
					"Unknown system error occurred: %1%:%2%."
						" Please contact product support to resolve this issue."
						" %3% %4%");
				message
					% __FILE__ % __LINE__
					% TUNNELEX_NAME % TUNNELEX_BUILD_IDENTITY;
				Log::GetInstance().AppendFatalError(message.str());
				assert(false);
			}
		}
		Log::GetInstance().AppendDebug("Rules checking thread completed.");
		return 0;
	}

	bool CheckTunnelRules() {
		IndexedTunnelRuleSet rulesToCheck;
		{
			RulesWriteLock lock(m_rulesMutex);
			if (m_tunnelRulesToCheck.size() == 0) {
				return false;
			}
			m_tunnelRulesToCheck.swap(rulesToCheck);
		}
		assert(rulesToCheck.size() > 0);
		foreach (const TunnelRule &rule, rulesToCheck) {
			try {
				Update(rule);
			} catch (const LicenseException &ex) {
				Log::GetInstance().AppendDebug(
					ConvertString<String>(ex.GetWhat()).GetCStr());
			} catch (...) {
				// resets all rules for checking
				throw;
			}
		}
		return m_tunnelRulesToCheck.size() > 0;
	}

	void SwitchTunnel(boost::shared_ptr<Tunnel> tunnel) {
		size_t newConnectionsInQueue;
		size_t tunnelsToSwitchInQueue;
		{
			TunnelOpeningState::Lock conditionLock(m_tunnelOpeningState.mutex);
			newConnectionsInQueue = m_tunnelOpeningState.newConnections.size();
			tunnelsToSwitchInQueue = m_tunnelOpeningState.tunnels.size();
			m_tunnelOpeningState.tunnels.push_back(tunnel);
			m_tunnelOpeningState.condition.signal();
		}
		StartTunnelOpeningThread(newConnectionsInQueue, tunnelsToSwitchInQueue);
	}

	void OpenTunnelImplementation(
				boost::shared_ptr<RuleInfo> ruleInfo,
				SharedPtr<Connection> inConnection) {
		boost::shared_ptr<Tunnel> tunnel;
		{
			SharedPtr<Connection> reader;
			SharedPtr<Connection> writer;
			const RuleEndpoint &sourceEndpoint = inConnection->GetRuleEndpoint();
			if (sourceEndpoint.IsCombined()) {
				assert(sourceEndpoint.IsCombinedAcceptor());
				reader = writer = inConnection;
			} else if (!m_rwSplitLicense.IsFeatureAvailable(true)) {
				Log::GetInstance().AppendWarn(
					"Failed to open connection with I/O channels separation."
						" The functionality you have requested requires"
						" a License Upgrade. Please purchase a License that"
						" will enable this feature at http://" TUNNELEX_DOMAIN "/order"
						" or get free trial at http://" TUNNELEX_DOMAIN "/order/trial.");
				throw LocalException(L"Failed to open connection, License Upgrade required");
			} else {
				assert(sourceEndpoint.GetReadWriteAcceptor() != Endpoint::ACCEPTOR_NONE);
				switch (sourceEndpoint.GetReadWriteAcceptor()) {
					case Endpoint::ACCEPTOR_READER:
						reader = inConnection;
						writer = CreateConnection(
							sourceEndpoint,
							sourceEndpoint.GetWriteAddress(),
							L"write");
						break;
					case Endpoint::ACCEPTOR_WRITER:
						writer = inConnection;
						reader = CreateConnection(
							sourceEndpoint,
							sourceEndpoint.GetReadAddress(),
							L"read");
						break;
					default:
						assert(false);
				}
			}
			assert(reader && writer);
			tunnel.reset(
				new Tunnel(false, m_myInterface, ruleInfo->rule, reader, writer));
		}
		OpenTunnelImplementation(tunnel);
	}

	void SwitchTunnelImplementation(boost::shared_ptr<Tunnel> tunnel) {

		for ( ; ; ) {
		
			boost::optional<SharedPtr<Connection> > reader;
			boost::optional<SharedPtr<Connection> > writer;
			
			bool reopenRead = false;
			bool reopenWrite = false;
			const RuleEndpoint &sourceEndpoint
				= const_cast<const Tunnel *>(tunnel.get())->GetIncomingReadConnection().GetRuleEndpoint();
			if (sourceEndpoint.IsCombined()) {
				if (const_cast<const Tunnel *>(tunnel.get())->GetIncomingReadConnection().IsSetupFailed()) {
					if (	!const_cast<const Tunnel *>(tunnel.get())
								->GetIncomingReadConnection()
								.GetRuleEndpointAddress()
								->IsReadyToRecreateLocalConnection()) {
						return;
					}
					reopenRead = true;
				}
			} else {
				const Endpoint::Acceptor acceptor = sourceEndpoint.GetReadWriteAcceptor();
				if (	acceptor == Endpoint::ACCEPTOR_NONE
						|| acceptor == Endpoint::ACCEPTOR_WRITER) {
					if (	const_cast<const Tunnel *>(tunnel.get())
								->GetIncomingReadConnection()
								.IsSetupFailed()) {
						if (const_cast<const Tunnel *>(tunnel.get())
								->GetIncomingReadConnection()
								.GetRuleEndpointAddress()
								->IsReadyToRecreateLocalConnection()) {
							return;
						}
						reopenRead = true;
					}
				}
				if (	acceptor == Endpoint::ACCEPTOR_NONE
						|| acceptor == Endpoint::ACCEPTOR_READER) {
					if (	const_cast<const Tunnel *>(tunnel.get())
								->GetIncomingWriteConnection().IsSetupFailed()) {
						if (!const_cast<const Tunnel *>(tunnel.get())
								->GetIncomingWriteConnection()
								.GetRuleEndpointAddress()
								->IsReadyToRecreateLocalConnection()) {
							return;
						}
						reopenWrite = true;
					}
				}
			}

			if (reopenRead) {
				if (sourceEndpoint.IsCombined()) {
					Log::GetInstance().AppendDebug(
						"Reopening incoming connection (read/write) for tunnel %1%...",
						tunnel->GetInstanceId());
					reader = writer = CreateConnection(
						sourceEndpoint,
						const_cast<const Tunnel *>(tunnel.get())
							->GetIncomingReadConnection()
							.GetRuleEndpointAddress(),
						L"combined");
				} else {
					Log::GetInstance().AppendDebug(
						"Reopening connection (read) for tunnel %1%...",
						tunnel->GetInstanceId());
					reader = CreateConnection(
						sourceEndpoint,
						const_cast<const Tunnel *>(tunnel.get())
							->GetIncomingReadConnection()
							.GetRuleEndpointAddress(),
						L"read");
				}
			}
			if (reopenWrite) {
				Log::GetInstance().AppendDebug(
					"Reopening connection (write) for tunnel %1%...",
					tunnel->GetInstanceId());
				writer = CreateConnection(
					sourceEndpoint,
					const_cast<const Tunnel *>(tunnel.get())
						->GetIncomingWriteConnection()
						.GetRuleEndpointAddress(),
					L"write");
			}

			if (!tunnel->Switch(reader, writer)) {
				break;
			} else if (!OpenTunnelImplementation(tunnel))  {
				continue;
			} else {
				break;
			}

		}

	}

	bool OpenTunnelImplementation(boost::shared_ptr<Tunnel> tunnel) {
		Log::GetInstance().AppendDebug(
			"Number of currently open tunnels: %1%.",
			m_activeTunnels.size() + 1);
		{
			ActiveTunnelsWriteLock lock(m_activeTunnelsMutex);
			if (tunnel->IsSetupFailed()) {
				Log::GetInstance().AppendDebug(
					"Closing tunnel %1% - some connections setup was canceled, while opening another.",
					tunnel->GetInstanceId());
				return false;
			}
			if (!m_tunnelLicense.IsFeatureAvailable(m_activeTunnels.size() + 1)) {
				Format message(
					"Failed to open new connection: too many connections - %1%."
						" The functionality you have requested requires"
						" a License Upgrade. Please purchase a License that"
						" will enable this feature at http://" TUNNELEX_DOMAIN "/order"
						" or get free trial at http://" TUNNELEX_DOMAIN "/order/trial.");
				message % m_activeTunnels.size();
				Log::GetInstance().AppendWarn(message.str());
				throw LocalException(
					L"Failed to open new connection, License Upgrade required");
			}
			assert(
				m_activeTunnels.get<ByInstance>().find(tunnel->GetInstanceId())
				== m_activeTunnels.get<ByInstance>().end());
			m_activeTunnels.insert(ActiveTunnel(tunnel));
		}
		try {
			tunnel->StartSetup();
		} catch (...) {
			tunnel->MarkAsDead();
			CloseTunnel(tunnel->GetInstanceId());
			throw;
		}
		return true;
	}

	void StartTunnelOpeningThread(
				size_t newConnectionsInQueue,
				size_t tunnelsToSwitchInQueue) {
		
		const long queue = newConnectionsInQueue + tunnelsToSwitchInQueue;
		if (queue <= m_tunnelOpeningState.threadsCount) {
			return;
		}
		auto threadsToStart
			= queue - m_tunnelOpeningState.threadsCount + openingTunnelMinThreadCount;
		
		Log::GetInstance().AppendDebug(
			"Starting %1% thread(s) for tunnel opening (already started: %2%).",
			threadsToStart,
			m_tunnelOpeningState.threadsCount);
		
		if (m_tunnelOpeningState.threadsCount + threadsToStart > openingTunnelMaxThreadCount) {
			Format message(
				"Failed to start new tunnel opening thread."
					" Maximum number of threads exceeded."
					" Already active %1% threads."
					" Trying to start %2% threads."
					" Please contact product support to resolve this issue.");
			message % m_tunnelOpeningState.threadsCount;
			message % threadsToStart;
			Log::GetInstance().AppendWarn(message.str());
			threadsToStart = openingTunnelMaxThreadCount - m_tunnelOpeningState.threadsCount;
			assert(threadsToStart > 0);
		}
		
		m_threadManager.spawn_n(
			threadsToStart,
			&TunnelOpeningThread,
			this,
			THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED,
			ACE_DEFAULT_THREAD_PRIORITY,
			TG_TUNNEL_OPENING);
	
	}

	static ACE_THR_FUNC_RETURN TunnelOpeningThread(void *param) {

		Implementation &instance = *static_cast<Implementation *>(param);
		TunnelOpeningState &state = instance.m_tunnelOpeningState;
		Interlocked::Increment(state.threadsCount);
		Log::GetInstance().AppendDebug("Started tunnel opening thread.");

		const class Cleanup : private boost::noncopyable {
		public:
			Cleanup(volatile long &threadsCountRef)
					: m_threadsCount(threadsCountRef) {
				//...//
			}
			~Cleanup() {
				Interlocked::Decrement(m_threadsCount);
			}
		private:
			volatile long &m_threadsCount;
		} cleanup(state.threadsCount);

		for ( ; ; ) {

			TunnelOpeningState::NewConnection newConnection;
			boost::shared_ptr<Tunnel> tunnel;
			size_t newConnectionsInQueue = 0;
			size_t tunnelsToSwitchInQueue = 0;
			const char *logMessageSubject = nullptr;

			for (bool isTimeout = false; ; isTimeout = true) {

				TunnelOpeningState::Lock conditionLock(state.mutex);
				if (instance.m_isDestructionMode) {
					Log::GetInstance().AppendDebug(
						"Closing tunnel opening thread, because a destruction mode has activated...");
					return 0;
				}

				if (state.newConnections.empty() && state.tunnels.empty()) {

					if (isTimeout && state.threadsCount > openingTunnelMinThreadCount) {
						Log::GetInstance().AppendDebug(
							"Too many threads (%1%) for tunnel opening, closing one...",
							state.threadsCount);
						return 0;
					}

					if (Log::GetInstance().IsDebugRegistrationOn()) {
						Log::GetInstance().AppendDebug(
							"No connections or tunnels in queue, will wait %1% minutes...",
							long(floor(openingTunnelThreadMaxIdleTime.sec() / 60.0)));
					}
					const ACE_Time_Value waitUntilTime
						= ACE_OS::gettimeofday() + openingTunnelThreadMaxIdleTime;
					
					if (state.condition.wait(&waitUntilTime) == -1) {
						assert(errno == ETIME);
						if (errno != ETIME) {
							const Error error(errno);
							Format message(
								"Failed to set tunnel opening condition, system error: %1% (%2%).");
							message % error.GetStringA();
							message % error.GetErrorNo();
							Log::GetInstance().AppendSystemError(message.str());
							return 1;
						}
					}
					
				}

				newConnectionsInQueue = state.newConnections.size();
				tunnelsToSwitchInQueue = state.tunnels.size();
				
				if (newConnectionsInQueue > 0) {
					newConnection = *state.newConnections.begin();
					state.newConnections.pop_front();
					--newConnectionsInQueue;
					logMessageSubject = Log::GetInstance().IsDebugRegistrationOn()
						?	"connection is taken to open tunnel"
						:	nullptr;
				} else if (tunnelsToSwitchInQueue > 0) {
					tunnel = *state.tunnels.begin();
					state.tunnels.pop_front();
					--tunnelsToSwitchInQueue;
					logMessageSubject = Log::GetInstance().IsDebugRegistrationOn()
						?	"tunnel is taken to switch"
						:	nullptr;
				} else {
					continue;
				}
			
				break;
			
			}

			if (logMessageSubject) {
				Log::GetInstance().AppendDebug(
					"One %1%, another connections/tunnels %2%/%3% in queue.",
					logMessageSubject,
					newConnectionsInQueue,
					tunnelsToSwitchInQueue);
			}

			instance.StartTunnelOpeningThread(newConnectionsInQueue, tunnelsToSwitchInQueue);

			try {
				if (newConnection) {
					try {
						instance.OpenTunnelImplementation(
							newConnection.ruleInfo,
							newConnection.connection);
					} catch	(const TunnelEx::DestinationConnectionOpeningException &ex) {
						ReportException(newConnection.ruleInfo->rule->GetErrorsTreatment(), ex);
					}
				} else if (tunnel && !tunnel->IsDead()) {
					try {
						instance.SwitchTunnelImplementation(tunnel);
					} catch (const TunnelEx::DestinationConnectionOpeningException &ex) {
						ReportException(tunnel->GetRule().GetErrorsTreatment(), ex);
					}
				}
			} catch (const TunnelEx::LocalException &ex) {
				Format message("Failed to open tunnel: %1%.");
				message % ConvertString<String>(ex.GetWhat()).GetCStr();
				Log::GetInstance().AppendError(message.str().c_str());
			} catch (const std::exception &ex) {
				Format message("Failed to open tunnel: %1%.");
				message % ex.what();
				Log::GetInstance().AppendSystemError(message.str().c_str());
			} catch (...) {
				Format message(
					"Unknown system error occurred: %1%:%2%."
						" Please contact product support to resolve this issue."
						" %3% %4%");
				message
					% __FILE__ % __LINE__
					% TUNNELEX_NAME % TUNNELEX_BUILD_IDENTITY;
				Log::GetInstance().AppendFatalError(message.str());
				assert(false);
			}

		}

	}

	static ACE_THR_FUNC_RETURN ProactorEventLoopThread(void *param) {
		Log::GetInstance().AppendDebug("Started proactor events thread.");
		Implementation &instance = *static_cast<Implementation *>(param);
		for ( ; ; ) {
			try {
				ACE_Proactor &proactor = instance.m_proactor;
				proactor.proactor_run_event_loop();
				assert(static_cast<Implementation *>(param)->m_activeTunnels.empty());
				break;
			} catch (const TunnelEx::LocalException &ex) {
				Log::GetInstance().AppendFatalError(ConvertString<String>(ex.GetWhat()).GetCStr());
			} catch (const std::exception &ex) {
				Log::GetInstance().AppendFatalError(ex.what());
			} catch (...) {
				Format message(
					"Unknown system error occurred: %1%:%2%."
						" Please contact product support to resolve this issue."
						" %3% %4%");
				message
					% __FILE__ % __LINE__
					% TUNNELEX_NAME % TUNNELEX_BUILD_IDENTITY;
				Log::GetInstance().AppendFatalError(message.str());
				assert(false);
			}
		}
		Log::GetInstance().AppendDebug("Proactor events thread completed.");
		return NULL;
	}

	static ACE_THR_FUNC_RETURN ReactorEventLoopThread(void *param) {
		Log::GetInstance().AppendDebug("Started reactor events thread.");
		for ( ; ; ) {
			try {
				ACE_Reactor &reactor = static_cast<Implementation *>(param)->m_reactor;
				reactor.owner(ACE_Thread::self());
				reactor.run_reactor_event_loop();
				break;
			} catch (const TunnelEx::LocalException &ex) {
				Log::GetInstance().AppendFatalError(ConvertString<String>(ex.GetWhat()).GetCStr());
			} catch (const std::exception &ex) {
				Log::GetInstance().AppendFatalError(ex.what());
			} catch (...) {
				Format message(
					"Unknown system error occurred: %1%:%2%."
						" Please contact product support to resolve this issue."
						" %3% %4%");
				message
					% __FILE__ % __LINE__
					% TUNNELEX_NAME % TUNNELEX_BUILD_IDENTITY;
				Log::GetInstance().AppendFatalError(message.str());
				assert(false);
			}
		}
		Log::GetInstance().AppendDebug("Reactor events thread completed.");
		return NULL;
	}

	inline static bool IsServerOs() {
		OSVERSIONINFOEX verInfo;
		ZeroMemory(&verInfo, sizeof(OSVERSIONINFOEX));
		verInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
		if (!GetVersionEx(reinterpret_cast<OSVERSIONINFO *>(&verInfo))) {
			const Error error(GetLastError());
			WFormat message(L"Failed to get system info: %1% (%2%)");
			message % error.GetStringW() % error.GetErrorNo();
			throw SystemException(message.str().c_str());
		}
		return verInfo.wProductType != VER_NT_WORKSTATION;
	}

private:

	ServerWorker &m_myInterface;
	Server::Ref m_server;
	
	ACE_Reactor m_reactor;
	ACE_Proactor m_proactor;
	
	ActiveRules m_activeRules;
	
	mutable ActiveTunnelsMutex m_activeTunnelsMutex;
	ActiveTunnels m_activeTunnels;
	bool m_isServicesThreadLaunched;

	IndexedTunnelRuleSet m_tunnelRulesToCheck;
	bool m_isRulesCheckThreadLaunched;
	
	mutable RulesMutex m_rulesMutex;

	mutable ActiveServicesMutex m_activeServicesMutex;
	ActiveServices m_activeServices;
	
	ACE_Thread_Manager m_threadManager;
	volatile long m_isDestructionMode;

	RuleUpdatingState m_ruleUpdatingState;
	TunnelOpeningState m_tunnelOpeningState;

	Licensing::FsLocalStorageState m_ruleSetLicenseState;
	Licensing::RuleSetLicense m_ruleSetLicense;
	Licensing::FsLocalStorageState m_tunnelLicenseState;
	Licensing::TunnelLicense m_tunnelLicense;
	Licensing::FsLocalStorageState m_rwSplitLicenseState;
	Licensing::EndpointIoSeparationLicense m_rwSplitLicense;

	ServerStopMutex m_serverStopMutex;
	ServerStopCondition m_serverStopCondition;

};

//////////////////////////////////////////////////////////////////////////

ServerWorker::ServerWorker(Server::Ref server) {
	m_pimpl = new Implementation(*this, server);
}

ServerWorker::~ServerWorker() {
	delete m_pimpl;
}

size_t ServerWorker::GetOpenedEndpointsNumber() const {
	return m_pimpl->GetOpenedEndpointsNumber();
}

size_t ServerWorker::GetTunnelsNumber() const {
	return m_pimpl->GetTunnelsNumber();
}

AutoPtr<EndpointAddress> ServerWorker::GetRealOpenedEndpointAddress(
			const WString &ruleUuid,
			const WString &endpointUuid)
		const {
	return m_pimpl->GetRealOpenedEndpointAddress(ruleUuid, endpointUuid);
}

bool ServerWorker::Update(const ServiceRule &rule) {
	return m_pimpl->Update(rule);
}

bool ServerWorker::Update(const TunnelRule &rule) {
	return m_pimpl->Update(rule);
}

bool ServerWorker::Update(const RuleSet &rules) {
	try {
		bool result = true;
		{
			const size_t rulesNumb = rules.GetServices().GetSize();
			for (size_t i = 0; i < rulesNumb; ++i) {
				if (!Update(rules.GetServices()[i])) {
					result = false;
				}
			}
		}
		{
			const size_t rulesNumb = rules.GetTunnels().GetSize();
			for (size_t i = 0; i < rulesNumb; ++i) {
				if (!Update(rules.GetTunnels()[i])) {
					result = false;
				}
			}
		}
		return result;
	} catch (const LicenseException &ex) {
		Log::GetInstance().AppendError(
			ConvertString<String>(ex.GetWhat()).GetCStr());
		return false;
	}
}

bool ServerWorker::IsRuleEnabled(const WString &uuid) const {
	return m_pimpl->IsRuleEnabled(uuid);
}

bool ServerWorker::DeleteRule(const WString &ruleUuid) {
	return m_pimpl->DeleteRule(ruleUuid);
}

void ServerWorker::OpenTunnel(
			boost::shared_ptr<RuleInfo> ruleInfo,
			Acceptor &incomingConnectionAcceptor) {
	m_pimpl->OpenTunnel(ruleInfo, incomingConnectionAcceptor);
}

ACE_Proactor & ServerWorker::GetProactor() {
	return m_pimpl->GetProactor();
}

ACE_Reactor & ServerWorker::GetReactor() {
	return m_pimpl->GetReactor();
}

void ServerWorker::CloseTunnel(Instance::Id tunnelId) {
	m_pimpl->CloseTunnel(tunnelId);
}

Server::Ref ServerWorker::GetServer() {
	return m_pimpl->GetServer();
}

//////////////////////////////////////////////////////////////////////////
