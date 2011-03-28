/**************************************************************************
 *   Created: 2007/02/09 5:25
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Server.cpp 1129 2011-02-22 17:28:50Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "Server.hpp"
#include "SslCertificatesStorage.hpp"
#include "Log.hpp"
#include "Exceptions.hpp"
#include "ServerWorker.hpp"
#include "ModulesFactory.hpp"
#include "EndpointAddress.hpp"

using namespace std;
using namespace boost;
using namespace TunnelEx;
using namespace TunnelEx::Singletons;

//////////////////////////////////////////////////////////////////////////

//! Server implementation class.
class TunnelEx::Singletons::ServerPolicy::Implementation : private noncopyable {

private:

	typedef ACE_Recursive_Thread_Mutex CtrlMutex;
	typedef ACE_Guard<ACE_Recursive_Thread_Mutex> CtrlLock;

public:

	explicit Implementation(Server::Ref server)
			: m_server(server),
			m_certificatesStorage(0) {
		//...//
	}
	
	~Implementation() {
		//...//
	}

public:

	void Start(const RuleSet &rules, const SslCertificatesStorage &certificatesStorage) {

		const SslCertificatesStorage *const certificatesStoragePtr
			= &certificatesStorage;
		
		CtrlLock lock(m_ctrlMutex);

		Log::Ref log = Log::GetInstance();
		log.AppendDebug("Attempt to start server.");
		if (m_worker.get()) {
			const wchar_t *const message
				= L"The attempt to start server twice has been occurred";
			log.AppendDebug(message);
			throw LogicalException(message);
		}

		auto_ptr<ServerWorker> worker(new ServerWorker(m_server));
		bool rulesOpenResult = true;
		log.AppendDebug(
			"Rule set size: %1% service(s), %2% tunnel(s).",
			rules.GetServices().GetSize(),
			rules.GetTunnels().GetSize());
		worker->Update(rules);
		m_worker = worker;

		if (rulesOpenResult) {
			Log::GetInstance().AppendInfo("Server successfully started.");
		} else {
			Log::GetInstance().AppendWarn("Server started, but some ports or tunnels was not opened.");
		}

		m_certificatesStorage = certificatesStoragePtr;

	}
	
	bool IsStarted() const {
		return m_worker.get() ? true : false;
	}

	void Stop() {
		
		CtrlLock lock(m_ctrlMutex);
		Log::Ref log = Log::GetInstance();

		log.AppendDebug("Attempt to stop server.");
		if (!m_worker.get()) {
			const wchar_t *const message
				= L"The attempt to stop inactive server has been occurred";
			log.AppendDebug(message);
			throw LogicalException(message);
		}
		m_worker.reset();
		m_certificatesStorage = 0;
		
		log.AppendInfo("Server successfully stopped.");

	}

	bool UpdateRule(const ServiceRule &rule) {
		CtrlLock lock(m_ctrlMutex);
		return !m_worker.get() ? false : m_worker->Update(rule);
	}

	bool UpdateRule(const TunnelRule &rule) {
		CtrlLock lock(m_ctrlMutex);
		return !m_worker.get() ? false : m_worker->Update(rule);
	}

	bool DeleteRule(const WString &uuid) {
		CtrlLock lock(m_ctrlMutex);
		return !m_worker.get() ? false : m_worker->DeleteRule(uuid);
	}

	bool IsRuleEnabled(const WString &uuid) const {
		CtrlLock lock(m_ctrlMutex);
		return !m_worker.get() ? false : m_worker->IsRuleEnabled(uuid);
	}

	UniquePtr<EndpointAddress> GetRealOpenedEndpointAddress(
				const WString &ruleUuid,
				const WString &endpointUuid)
			const {
		CtrlLock lock(m_ctrlMutex);
		if (!m_worker.get()) {
			throw LogicalException(L"Could not get real opened endpoint, server does not started");
		}
		return m_worker->GetRealOpenedEndpointAddress(ruleUuid, endpointUuid);
	}

	size_t GetTunnelsNumber() const {
		CtrlLock lock(m_ctrlMutex);
		if (!m_worker.get()) {
			throw LogicalException(L"Could not get tunnels number, server does not started");
		}
		return m_worker->GetTunnelsNumber();
	}
	
	size_t GetOpenedEndpointsNumber() const {
		CtrlLock lock(m_ctrlMutex);
		if (!m_worker.get()) {
			throw LogicalException(L"Could not get opened endpoints number, server does not started");
		}
		return m_worker->GetOpenedEndpointsNumber();
	}

	const SslCertificatesStorage & GetCertificatesStorage() const {
		if (m_certificatesStorage == 0) {
			throw LogicalException(L"SSL certificates storage does not opened");
		}
		return *m_certificatesStorage;
	}

private:

	auto_ptr<ServerWorker> m_worker;
	mutable CtrlMutex m_ctrlMutex;
	Server::Ref m_server;
	const SslCertificatesStorage *m_certificatesStorage;

};

//////////////////////////////////////////////////////////////////////////

Singletons::ServerPolicy::ServerPolicy()
		: m_pimpl(new Implementation(*this)) {
	//...//
}
Singletons::ServerPolicy::~ServerPolicy() throw() {
	delete m_pimpl;
}

void Singletons::ServerPolicy::Start(
			const RuleSet &rules,
			const SslCertificatesStorage &certificatesStorage) {
	m_pimpl->Start(rules, certificatesStorage);
}

bool Singletons::ServerPolicy::IsStarted() const {
	return m_pimpl->IsStarted();
}

void Singletons::ServerPolicy::Stop() {
	m_pimpl->Stop();
}

bool Singletons::ServerPolicy::UpdateRule(const ServiceRule &rule) {
	return m_pimpl->UpdateRule(rule);
}

bool Singletons::ServerPolicy::UpdateRule(const TunnelRule &rule) {
	return m_pimpl->UpdateRule(rule);
}

bool Singletons::ServerPolicy::DeleteRule(const WString &uuid) {
	return m_pimpl->DeleteRule(uuid);
}

bool Singletons::ServerPolicy::IsRuleEnabled(const WString &uuid) const {
	return m_pimpl->IsRuleEnabled(uuid);
}

UniquePtr<EndpointAddress>
Singletons::ServerPolicy::GetRealOpenedEndpointAddress(
			const WString &ruleUuid,
			const WString &endpointUuid)
		const {
	return m_pimpl->GetRealOpenedEndpointAddress(ruleUuid, endpointUuid);
}

size_t Singletons::ServerPolicy::GetTunnelsNumber() const {
	return m_pimpl->GetTunnelsNumber();
}

size_t Singletons::ServerPolicy::GetOpenedEndpointsNumber() const {
	return m_pimpl->GetOpenedEndpointsNumber();
}

const SslCertificatesStorage & Singletons::ServerPolicy::GetCertificatesStorage() const {
	return m_pimpl->GetCertificatesStorage();
}

//////////////////////////////////////////////////////////////////////////

#if TEMPLATES_REQUIRE_SOURCE != 0
#	include "Singleton.cpp"
	//! Only for template instantiation.
	void MakeServerTemplateInstantiation() {
		Server::GetInstance();
	}
#endif // TEMPLATES_REQUIRE_SOURCE
