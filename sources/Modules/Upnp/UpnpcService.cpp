/**************************************************************************
 *   Created: 2010/05/25 3:33
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: UpnpcService.cpp 1079 2010-12-01 05:19:27Z palchukovsky $
 **************************************************************************/

#include "Prec.h"
#include "UpnpcService.hpp"
#include "EndpointResourceIdentifierParsers.hpp"
#include "Core/Log.hpp"

using namespace TunnelEx;
using namespace TunnelEx::Helpers;
using namespace TunnelEx::Mods::Upnp;

//////////////////////////////////////////////////////////////////////////


class UpnpcService::Implementation {

public:

	Implementation()
			: m_proto(Client::PROTO_TCP),
			m_isForceMode(false),
			m_isPersistent(false),
			m_nextCheckTime(ACE_Time_Value::zero) {
		//...//
	}

public:

	ACE_Time_Value GetNextCheckTime() const {
		// see Client::Implementation::Cache::IsActual 
		return ACE_OS::gettimeofday() + ACE_Time_Value(60 * 5);
	}

public:

	Client::Proto m_proto;
	std::string m_externalPort;
	std::string m_destinationHost;
	std::string m_destinationPort;
	bool m_isForceMode;
	bool m_isPersistent;
	std::string m_id;

	Client m_client;

	ACE_Time_Value m_nextCheckTime;

};

//////////////////////////////////////////////////////////////////////////

UpnpcService::UpnpcService(
			SharedPtr<const ServiceRule> rule,
			const ServiceRule::Service &service)
		: Service(rule, service) {
	
	BOOST_ASSERT(service.name == L"Upnpc");
	
	unsigned short externalPort = 0;
	std::wstring destinationHost;
	unsigned short destinationPort = 0;
	std::auto_ptr<Implementation> pimpl(new Implementation);
	ParseParam(
		GetService().param,
		pimpl->m_proto,
		externalPort,
		destinationHost,
		destinationPort,
		pimpl->m_isForceMode,
		pimpl->m_isPersistent);
	pimpl->m_externalPort = boost::lexical_cast<std::string>(externalPort);
	pimpl->m_destinationHost = ConvertString<String>(destinationHost.c_str()).GetCStr();
	pimpl->m_destinationPort = boost::lexical_cast<std::string>(destinationPort);
	pimpl->m_id = ConvertString<String>(GetService().uuid).GetCStr();

	SetStarted(pimpl->m_client.CheckMapping(
		pimpl->m_externalPort,
		pimpl->m_destinationHost,
		pimpl->m_destinationPort,
		pimpl->m_proto,
		pimpl->m_id));

	if (IsStarted()) {
		pimpl->m_nextCheckTime = pimpl->GetNextCheckTime();
	}

	m_pimpl = pimpl.release();

	Log::GetInstance().AppendDebug(
		"Upnpc service %1%: created, %2%, %3%.",
		m_pimpl->m_id,
		IsStarted() ? "started" : "stopped",
		m_pimpl->m_isPersistent ? "persistent" : "not persistent");

}
 
UpnpcService::~UpnpcService() throw() {
	if (!m_pimpl->m_isPersistent) {
		Stop();
	}
	delete m_pimpl;
}

namespace {

	std::wstring GetHumanReadable(
				Client::Proto proto,
				const std::wstring &host,
				unsigned short port) {
		WFormat result(L"> %1%://%2%:%3%");
		result
			% (proto == Client::PROTO_TCP ? L"tcp" : L"udp")
			% host
			% port;
		return result.str();
	}

}

std::wstring UpnpcService::GetHumanReadableExternalPort(
			const WString &param,
			const std::wstring &externalIp) {
	Client::Proto proto = Client::PROTO_TCP;
	unsigned short externalPort = 0;
	std::wstring destinationHost;
	unsigned short destinationPort = 0;
	bool isForceMode = false;
	bool isPersistent = false;
	ParseParam(
		param,
		proto,
		externalPort,
		destinationHost,
		destinationPort,
		isForceMode,
		isPersistent);
	return GetHumanReadable(proto, externalIp, externalPort);
}

std::wstring UpnpcService::GetHumanReadableDestination(const WString &param) {
	Client::Proto proto = Client::PROTO_TCP;
	unsigned short externalPort = 0;
	std::wstring destinationHost;
	unsigned short destinationPort = 0;
	bool isForceMode = false;
	bool isPersistent = false;
	ParseParam(
		param,
		proto,
		externalPort,
		destinationHost,
		destinationPort,
		isForceMode,
		isPersistent);
	return GetHumanReadable(proto, destinationHost, destinationPort);
}

WString UpnpcService::CreateParam(
			Client::Proto proto,
			unsigned short externalPort,
			const std::wstring &destinationHost,
			unsigned short destinationPort,
			bool isForceMode,
			bool isPersistent) {
	std::wostringstream result;
	result << L"proto=";
	switch (proto) {
		default:
			BOOST_ASSERT(false);
		case Client::PROTO_TCP:
			result << L"tcp";
			break;
		case Client::PROTO_UDP:
			result << L"udp";
			break;
	}
	result
		<< L"&ext_port=" << externalPort
		<< L"&dest_host=" << StringUtil::EncodeUrl(destinationHost)
		<< L"&dest_port=" << destinationPort
		<< L"&is_force=" << (isForceMode ? "true" : "false")
		<< L"&is_persistent=" << (isPersistent ? "true" : "false");
	return result.str().c_str();
}

void UpnpcService::ParseParam(
			const WString &paramIn,
			Client::Proto &proto,
			unsigned short &externalPort,
			std::wstring &destinationHost,
			unsigned short &destinationPort,
			bool &isForceMode,
			bool &isPersistent) {

	const std::wstring param(paramIn.GetCStr());
		
	EndpointResourceIdentifierParsers::UrlSplitConstIterator paramIt
		= boost::make_split_iterator(param, boost::token_finder(boost::is_any_of(L"&")));

	unsigned short externalPortTmp;
	EndpointResourceIdentifierParsers::ParseUrlParam(
		paramIt,
		L"ext_port",
		boost::bind(
			&EndpointResourceIdentifierParsers::ParseUrlParamValue<unsigned short>,
			_1,
			boost::ref(externalPortTmp)),
		false);
	unsigned short destinationPortTmp;
	EndpointResourceIdentifierParsers::ParseUrlParam(
		paramIt,
		L"dest_port",
		boost::bind(
			&EndpointResourceIdentifierParsers::ParseUrlParamValue<unsigned short>,
			_1,
			boost::ref(destinationPortTmp)),
		false);

	std::wstring protoStr;
	EndpointResourceIdentifierParsers::ParseUrlParam(
		paramIt,
		L"proto",
		boost::bind(
			&EndpointResourceIdentifierParsers::ParseUrlParamValue<std::wstring>,
			_1,
			boost::ref(protoStr)),
		false);
	Client::Proto protoTmp;
	if (boost::iequals(protoStr, L"tcp")) {
		protoTmp = Client::PROTO_TCP;
	} else if (boost::iequals(protoStr, L"udp")) {
		protoTmp = Client::PROTO_UDP;
	} else {
		throw TunnelEx::InvalidLinkException(
			L"Unknown protocol for port mapping by UPnP");
	}

	std::wstring destinationHostTmp;
	EndpointResourceIdentifierParsers::ParseUrlParam(
		paramIt,
		L"dest_host",
		boost::bind(
			&EndpointResourceIdentifierParsers::ParseUrlParamValue<std::wstring>,
			_1,
			boost::ref(destinationHostTmp)),
		false);
	destinationHostTmp = StringUtil::DecodeUrl(destinationHostTmp);
	if (destinationHostTmp.empty()) {
		throw TunnelEx::InvalidLinkException(
			L"Unknown destination host for port mapping by UPnP");
	}

	bool isForceModeTmp;
	EndpointResourceIdentifierParsers::ParseUrlParam(
		paramIt,
		L"is_force",
		boost::bind(
			&EndpointResourceIdentifierParsers::ParseUrlParamValue<bool>,
			_1,
			boost::ref(isForceModeTmp)),
		false);

	bool isPersistentTmp;
	EndpointResourceIdentifierParsers::ParseUrlParam(
		paramIt,
		L"is_persistent",
		boost::bind(
		&EndpointResourceIdentifierParsers::ParseUrlParamValue<bool>,
		_1,
		boost::ref(isPersistentTmp)),
		false);

	proto = protoTmp;
	std::swap(externalPortTmp, externalPort);
	std::swap(destinationHostTmp, destinationHost);
	destinationPort = destinationPortTmp;
	isForceMode = isForceModeTmp;
	isPersistent = isPersistentTmp;

}

void UpnpcService::Start() {
	BOOST_ASSERT(!IsStarted());
	ACE_Time_Value nextCheck = m_pimpl->GetNextCheckTime();
	m_pimpl->m_client.AddPortMapping(
		m_pimpl->m_externalPort,
		m_pimpl->m_destinationHost,
		m_pimpl->m_destinationPort,
		m_pimpl->m_proto,
		m_pimpl->m_id,
		m_pimpl->m_isForceMode);
	SetStarted(true);
	std::swap(nextCheck, m_pimpl->m_nextCheckTime);
}

void UpnpcService::Stop() throw() {
	if (IsStarted()) {
		m_pimpl->m_nextCheckTime = ACE_Time_Value::zero;
		m_pimpl->m_client.DeletePortMapping(m_pimpl->m_id);
		SetStarted(false);
	}
}

void UpnpcService::DoWork() {
	
	BOOST_ASSERT(IsStarted());
	BOOST_ASSERT(m_pimpl->m_nextCheckTime != ACE_Time_Value::zero);
	
	if (ACE_OS::gettimeofday() < m_pimpl->m_nextCheckTime) {
		return;
	}
	
	const bool isMappingExist = m_pimpl->m_client.CheckMapping(
		m_pimpl->m_externalPort,
		m_pimpl->m_destinationHost,
		m_pimpl->m_destinationPort,
		m_pimpl->m_proto,
		m_pimpl->m_id);

	if (isMappingExist) {
		Log::GetInstance().AppendDebug(
			"Checked UPnP port mapping: ok (%1%->%2%:%3%, %4%).",
			m_pimpl->m_externalPort,
			m_pimpl->m_destinationHost,
			m_pimpl->m_destinationPort,
			m_pimpl->m_id);
		m_pimpl->m_nextCheckTime = m_pimpl->GetNextCheckTime();
	} else {
		Format message(
			"UPnP port mapping rule \"%1% to %2%:%3%\" has been deleted by router, trying to restore...");
		message
			% m_pimpl->m_externalPort
			% m_pimpl->m_destinationHost
			% m_pimpl->m_destinationPort;
		Log::GetInstance().AppendWarn(message.str().c_str());
		m_pimpl->m_client.AddPortMapping(
			m_pimpl->m_externalPort,
			m_pimpl->m_destinationHost,
			m_pimpl->m_destinationPort,
			m_pimpl->m_proto,
			m_pimpl->m_id,
			m_pimpl->m_isForceMode);
	}

}

//////////////////////////////////////////////////////////////////////////

namespace TunnelEx { namespace Mods { namespace Upnp {
	
	UniquePtr<TunnelEx::Service> CreateUpnpcService(
				SharedPtr<const ServiceRule> rule,
				const ServiceRule::Service &service) {
		return UniquePtr<TunnelEx::Service>(new UpnpcService(rule, service));
	}

} } }
