/**************************************************************************
 *   Created: 2008/01/20 16:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "DestinationPingFilter.hpp"
#include "InetEndpointAddress.hpp"

#include "Core/Endpoint.hpp"
#include "Core/Rule.hpp"
#include "Core/Log.hpp"
#include "Core/Error.hpp"

using namespace TunnelEx;
using namespace TunnelEx::Mods::Inet;

//////////////////////////////////////////////////////////////////////////

DestinationPingFilter::Thread *DestinationPingFilter::m_thread = 0;
unsigned int DestinationPingFilter::m_objectCount = 0;

//////////////////////////////////////////////////////////////////////////

class DestinationPingFilter::PingEndpoint {

public:

	PingEndpoint()
			: m_endpoint(),
			m_pingTime(0) {
		assert(false);
	}

	explicit PingEndpoint(const RuleEndpoint &endpoint)
			: m_endpoint(endpoint),
			m_pingTime(0) {
		//...//
	}

private:

	const PingEndpoint & operator =(const PingEndpoint &);

public:

	bool operator >(const PingEndpoint &rhs) const {
		return m_pingTime > rhs.m_pingTime;
	}
	bool operator <(const PingEndpoint &rhs) const {
		return m_pingTime < rhs.m_pingTime;
	}

	const RuleEndpoint & GetEndpoint() const {
		return m_endpoint;
	}

	void Ping() {
		ACE_Ping_Socket ping(ACE_Addr::sap_any);
		Ping(ping);
		ping.close();
	}

	void Ping(ACE_Ping_Socket &ping) {
		typedef std::vector<ACE_INET_Addr> Addrs;
		Addrs addrs;
		if (m_endpoint.IsCombined()) {
			assert(m_endpoint.CheckCombinedAddressType<InetEndpointAddress>());
			addrs.push_back(
				m_endpoint.GetCombinedTypedAddress<InetEndpointAddress>().GetAceInetAddr());
		} else {
			assert(
				m_endpoint.CheckReadAddressType<InetEndpointAddress>()
				|| m_endpoint.CheckWriteAddressType<InetEndpointAddress>());
			if (m_endpoint.CheckReadAddressType<InetEndpointAddress>()) {
				addrs.push_back(
					m_endpoint.GetReadTypedAddress<InetEndpointAddress>().GetAceInetAddr());
			}
			if (m_endpoint.CheckWriteAddressType<InetEndpointAddress>()) {
				addrs.push_back(
					m_endpoint.GetWriteTypedAddress<InetEndpointAddress>().GetAceInetAddr());
			}
		}
		PingTime commonPingTime = 0;
		foreach (ACE_INET_Addr &addr, addrs) {
			const ACE_Time_Value before(ACE_OS::gettimeofday());
			//! \todo: hardcode [2008/01/24 3:50]
			ACE_Time_Value timeout(10);
			if (!ping.make_echo_check(addr, 0, &timeout)) {
				commonPingTime += (ACE_OS::gettimeofday() - before).msec();
				if (Log::GetInstance().IsDebugRegistrationOn()) {
					Log::GetInstance().AppendDebug(
						"Ping time for address from the endpoint \"%1%\" is %2% msec.",
						ConvertString<String>(m_endpoint.GetUuid()).GetCStr(),
						m_pingTime);
				}
			} else {
				m_pingTime = std::numeric_limits<PingTime>::max();
				if (errno == ETIME) {
					if (Log::GetInstance().IsDebugRegistrationOn()) {
						Log::GetInstance().AppendDebug(
							"Timeout of exceeded for ping address from the endpoint \"%1%\".",
							ConvertString<String>(m_endpoint.GetUuid()).GetCStr());
					}
				} else {
					const Error error(errno);
					std::ostringstream oss;
					String errorStr;
					oss << "Ping failed with the system error \""
						<< ConvertString(error.GetString(), errorStr).GetCStr()
						<< " (" << error.GetErrorNo() << "), endpoint \""
						<< ConvertString<String>(m_endpoint.GetUuid()).GetCStr() << ".";
					Log::GetInstance().AppendSystemError(oss.str());
				}
				return;
			}
		}
		m_pingTime = commonPingTime / addrs.size();
	}

private:

	bool operator ==(const PingEndpoint &rhs) const;
	bool operator !=(const PingEndpoint &rhs) const;

private:

	const RuleEndpoint m_endpoint;

	typedef unsigned long PingTime;
	PingTime m_pingTime;

};

//////////////////////////////////////////////////////////////////////////

class DestinationPingFilter::Thread : private boost::noncopyable {

public:

	Thread()
			: m_stopEvent(new StopEvent(m_reactor)) {
		m_threadManager.spawn(
			&Thread::ThreadMain,
			this,
			THR_SCOPE_PROCESS | THR_DETACHED);
		Log::GetInstance().AppendDebug("Thread for destinations pinging started.");
	}

	~Thread() {
		m_reactor.notify(m_stopEvent);
		m_threadManager.wait();
		Log::GetInstance().AppendDebug("Thread for destinations pinging stopped.");
	}

public:

	void Register(
				DestinationPingFilter &handler,
				DestinationPingFilter::Endpoints &endpoints) {
		Lock lock(m_endpointCollectionsMutex);
		m_endpointCollections[&endpoints] = &handler;
	}

	void Unregister(DestinationPingFilter::Endpoints &endpoints) {
		Lock lock(m_endpointCollectionsMutex);
		const EndpointCollections::iterator pos
			= m_endpointCollections.find(&endpoints);
		assert(pos != m_endpointCollections.end());
		if (pos != m_endpointCollections.end()) {
			m_endpointCollections.erase(pos);
		}
	}

	void Ping() {
		Lock lock(m_endpointCollectionsMutex);
		const EndpointCollections::iterator end = m_endpointCollections.end();
		ACE_Ping_Socket ping(ACE_Addr::sap_any);
		for (	EndpointCollections::iterator i = m_endpointCollections.begin();
				i != end;
				++i) {
			Ping(*i->first, *i->second, ping);
		}
		ping.close();
	}

private:

	typedef std::map<
			DestinationPingFilter::Endpoints *,
			DestinationPingFilter *>
		EndpointCollections;
	EndpointCollections m_endpointCollections;

	typedef ACE_Guard<ACE_Thread_Mutex> Lock;
	typedef ACE_Thread_Mutex Mutex;
	Mutex m_endpointCollectionsMutex;

	ACE_Reactor m_reactor;
	
	class StopEvent : public ACE_Event_Handler {
	public:
		explicit StopEvent(ACE_Reactor &reactor)
				: ACE_Event_Handler(&reactor, ACE_Event_Handler::LO_PRIORITY) {
			//...//
		}
	private:
		virtual ~StopEvent() {
			//...//
		}
	protected:
		virtual int handle_exception(ACE_HANDLE) {
			reactor()->end_reactor_event_loop();
			//! Trigger call to handle_close() method.
			return -1; 
		}
		virtual int handle_close(ACE_HANDLE, ACE_Reactor_Mask) {
			delete this;
			return 0;
		}
	};
	StopEvent *m_stopEvent;

	ACE_Thread_Manager m_threadManager;

protected:

	void Ping(
				DestinationPingFilter::Endpoints &endpoints,
				DestinationPingFilter &filter,
				ACE_Ping_Socket &ping) {
		bool orderChanged = false;
		const DestinationPingFilter::Endpoints::const_iterator end = endpoints.end();
		const DestinationPingFilter::Endpoints::iterator begin = endpoints.begin();
		for (	DestinationPingFilter::Endpoints::iterator i = begin;
				i != end;
				++i) {
			i->Ping(ping);
			if (!orderChanged) {
				DestinationPingFilter::Endpoints::const_iterator tmp = i;
				if (i != begin && *--tmp > *i) {
					orderChanged = true;
				} else {
					tmp = i;
					orderChanged = ++tmp != end && *i > *tmp;
				}
			}
		}
		if (orderChanged) {
			filter.ScheduleRuleChange();
		}
	}

private:

	static ACE_THR_FUNC_RETURN ThreadMain(void *param) {
		try {
			static_cast<Thread *>(param)->WorkWithConnection();
		} catch (const TunnelEx::LocalException &ex) {
			Log::GetInstance().AppendFatalError(
				ConvertString<String>(ex.GetWhat()).GetCStr());
		} catch (const std::exception &ex) {
			Log::GetInstance().AppendFatalError(ex.what());
			throw;
		} catch (...) {
			Log::GetInstance().AppendSystemError(
				"Unknown system error occurred in the pinging thread.");
			throw;
		}
		return NULL;
	}

	void WorkWithConnection() {
		m_reactor.owner(ACE_OS::thr_self());
		new TimeoutEvent(m_reactor, *this);
		m_reactor.run_reactor_event_loop();
	}

private:

	class TimeoutEvent : public ACE_Event_Handler {
	public:
		TimeoutEvent(ACE_Reactor &reactor, Thread &thread)
				: m_thread(thread) {
			//! \todo: hardcode [2008/01/24 4:29]
			reactor.schedule_timer(this, NULL, ACE_Time_Value(15));
		}
		~TimeoutEvent() {
			//...//
		}
	private:
		TimeoutEvent(const TimeoutEvent &);
		const TimeoutEvent & operator =(const TimeoutEvent &);
	private:
		Thread &m_thread;
	public:
		virtual int handle_close(ACE_HANDLE = ACE_INVALID_HANDLE, ACE_Reactor_Mask = 0) {
			delete this;
			return 0;
		}
		virtual int handle_timeout(const ACE_Time_Value &, const void * = 0) {
			m_thread.Ping();
			//! \todo: hardcode, first timeout too [2008/01/24 1:17]
			reactor()->schedule_timer(this, NULL, ACE_Time_Value(60 * 5));
			return 0;
		}
	};

};

//////////////////////////////////////////////////////////////////////////

DestinationPingFilter::DestinationPingFilter(
			SharedPtr<TunnelRule> rule,
			SharedPtr<RecursiveMutex> mutex)
		: Filter(rule, mutex) {
	AutoPtr<RecursiveLock> lock(LockRule());
	const RuleEndpointCollection &endpoints = GetRule().GetDestinations();
	const size_t size = endpoints.GetSize();
	for (size_t i = 0; i < size; ++i) {
		if (endpoints[i].IsCombined()) {
			if (!endpoints[i].CheckCombinedAddressType<InetEndpointAddress>()) {
				continue;
			}
		} else if (
				!endpoints[i].CheckReadAddressType<InetEndpointAddress>()
				&& !endpoints[i].CheckWriteAddressType<InetEndpointAddress>()) {
			continue;
		}
		const PingEndpoint endpoint(endpoints[i]);
		m_endpointUuids.insert(
			make_pair(
				endpoint.GetEndpoint().GetUuid(),
				m_endpoints.insert(m_endpoints.end(), endpoint)));
	}
	m_isActive = m_endpointUuids.size() > 0;
	if (!m_isActive) {
		Log::GetInstance().AppendDebug(
			"Destinations pinging not started, rule %1%, filter %2%.",
			ConvertString<String>(GetRule().GetUuid()).GetCStr(),
			GetInstanceId());
		return;
	}
	
	if (!m_objectCount++) {
		// Do not use locking here, construction and destructing always in one thread.
		assert(!m_thread);
		m_thread = new Thread;
	}
	m_thread->Register(*this, m_endpoints);
	if (Log::GetInstance().IsDebugRegistrationOn()) {
		Log::GetInstance().AppendDebug(
			"Destinations pinging started, rule %1%, filter %2%.",
			ConvertString<String>(GetRule().GetUuid()).GetCStr(),
			GetInstanceId());
	}
}

DestinationPingFilter::~DestinationPingFilter() {
	if (m_isActive) {
		m_thread->Unregister(m_endpoints);
		if (!--m_objectCount) {
			assert(m_thread);
			delete m_thread;
			m_thread = 0;
		}
	}
	Log::GetInstance().AppendDebug(
		"Destinations pinging stopped, filter %1%.", GetInstanceId());
}

void DestinationPingFilter::ChangeRule(TunnelRule &rule) {

	Log::Ref log = Log::GetInstance();

	RuleEndpointCollection& endpoints = rule.GetDestinations();
	const size_t size = endpoints.GetSize();

	EndpointUuids newEndpointUuids(m_endpointUuids);
	Endpoints newEndpoints(m_endpoints);
	
	// searching for new endpoints in rule
	typedef std::set<WString> RuleEndpointUuids;
	RuleEndpointUuids ruleEndpointUuids;
	for (unsigned int i = 0; i < size; ++i) {
		const RuleEndpoint& endpoint(endpoints[i]);
		const WString uuid = endpoint.GetUuid();
		if (newEndpointUuids.find(uuid) == newEndpointUuids.end()) {
			PingEndpoint pingEndpoint(endpoint);
			pingEndpoint.Ping();
			newEndpointUuids.insert(
				make_pair(
					uuid,
					newEndpoints.insert(newEndpoints.end(), pingEndpoint)));
			log.AppendDebug(
				"New endpoint %1% has been added to the destination pinging list.",
				uuid.GetCStr());
		}
		ruleEndpointUuids.insert(uuid);
	}
	
	if (endpoints.GetSize() != newEndpoints.size()) {
		// searching for old endpoints in sorting cache
		const EndpointUuids::const_iterator end = newEndpointUuids.end();
		const RuleEndpointUuids::const_iterator endInRuleEndpointUuids = ruleEndpointUuids.end();
		for (EndpointUuids::iterator i = newEndpointUuids.begin(); i != end; ) {
			const WString& uuid = i->first;
			if (ruleEndpointUuids.find(uuid) == endInRuleEndpointUuids) {
				newEndpoints.erase(i->second);
				i = newEndpointUuids.erase(i++);
				if (log.IsDebugRegistrationOn()) {
					log.AppendDebug(
						"Endpoint %1% has been removed from the destination pinging list.",
						ConvertString<String>(uuid).GetCStr());
				}
			} else {
				++i;
			}
		}
	}
	
	newEndpoints.sort();
	
	// inserting sorted endpoints in the rule
	bool orderChanged = false;
	const Endpoints::const_iterator end = newEndpoints.end();
	Endpoints::const_iterator endpointIter = newEndpoints.begin();
	for (unsigned int i = 0; i < size && endpointIter != end; ) {
		RuleEndpoint& ruleEndpoint(endpoints[i]);
		const RuleEndpoint& pingEndpoint(endpointIter->GetEndpoint());
		if (ruleEndpoint.GetUuid() != pingEndpoint.GetUuid()) {
			ruleEndpoint = pingEndpoint;
			orderChanged = true;
		}
		++i;
		++endpointIter;
	}

	if (orderChanged && log.IsDebugRegistrationOn()) {
		std::vector<std::wstring> endpointsOrder;
		endpointsOrder.reserve(size);
		for (unsigned int i = 0; i < size ; ++i) {
			endpointsOrder.push_back(endpoints[i].GetUuid().GetCStr());
		}
		const WString message(
				(WFormat(L"Destinations order has been changed, new order: %1%.")
					% boost::join(endpointsOrder, ", "))
			.str().c_str());
		log.AppendDebug(ConvertString<String>(message).GetCStr());
	}

}

//////////////////////////////////////////////////////////////////////////

namespace TunnelEx { namespace Mods { namespace Inet {

	SharedPtr<Filter> CreateDestinationPingFilter(
				SharedPtr<TunnelRule> rule,
				SharedPtr<RecursiveMutex> ruleChangeMutex) {
		return SharedPtr<Filter>(new DestinationPingFilter(rule, ruleChangeMutex));
	}

} } }

//////////////////////////////////////////////////////////////////////////