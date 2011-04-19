/**************************************************************************
 *   Created: 2010/05/24 22:59
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "Client.hpp"
#include "ClientLib.hpp"

#include "Core/Log.hpp"

using namespace TunnelEx;
using namespace TunnelEx::Mods::Upnp;

//////////////////////////////////////////////////////////////////////////

Client::Exception::Exception(const wchar_t *what) throw()
		: LocalException(what) {
	//...//
}

Client::Exception::Exception(const Exception &rhs) throw()
		: LocalException(rhs) {
	//...//
}

Client::Exception::~Exception() throw() {
	//...//
}

const Client::Exception & Client::Exception::operator =(const Exception &rhs) throw() {
	LocalException::operator =(rhs);
	return *this;
}

UniquePtr<LocalException> Client::Exception::Clone() const {
	return UniquePtr<LocalException>(new Exception(*this));
}

//////////////////////////////////////////////////////////////////////////

Client::DeviceNotExistException::DeviceNotExistException(const wchar_t *what) throw()
		: Exception(what) {
	//...//
}

Client::DeviceNotExistException::DeviceNotExistException(const DeviceNotExistException &rhs) throw()
		: Exception(rhs) {
	//...//
}

Client::DeviceNotExistException::~DeviceNotExistException() throw() {
	//...//
}

const Client::DeviceNotExistException & Client::DeviceNotExistException::operator =(
			const DeviceNotExistException &rhs)
		throw() {
	Exception::operator =(rhs);
	return *this;
}

UniquePtr<LocalException> Client::DeviceNotExistException::Clone() const {
	return UniquePtr<LocalException>(new DeviceNotExistException(*this));
}

//////////////////////////////////////////////////////////////////////////

class Client::Implementation : private boost::noncopyable {

public:

	struct Port {
		std::string externalPort;
		std::string internalHost;
		std::string internalPort;
		Proto proto;
		std::string description;
		bool isMyDescription;
	};
	typedef std::list<Port> Ports;

	struct PortsCache {

		typedef ACE_RW_Mutex Mutex;
		typedef ACE_Read_Guard<Mutex> ReadLock;
		typedef ACE_Write_Guard<Mutex> WriteLock;

		PortsCache()
				: lastUpdate(ACE_Time_Value::zero) {
			//...//
		}

		bool IsActual() const {
			// see UpnpcService::Implementation::GetNextCheckTime
			return (ACE_OS::gettimeofday() - lastUpdate) < ACE_Time_Value(time_t(60 * 5), 0);
		}

		Mutex mutex;
		ACE_Time_Value lastUpdate;
		
		Ports ports;

	};

public:

	ClientLib m_lib;
	boost::shared_ptr<UPNPDev> m_devList;
	ClientLib::Urls m_urls;
	IGDdatas m_data;
	std::string m_lanAddr;

private:

	void RefreshPortsCache(const PortsCache::WriteLock &) {
		
		Log::GetInstance().AppendDebug("Refreshing UPnP ports state cache...");
		
		Ports ports;
		for (size_t i = 0; ; ++i) {
			Ports::value_type port;
			std::string proto;
			const bool isFound = m_lib.GetGenericPortMappingEntry(
				m_urls,
				m_data,
				i,
				port.externalPort,
				port.internalHost,
				port.internalPort,
				proto,
				port.description);
			if (!isFound) {
				break;
			}
			port.proto = StrToProto(proto);
			port.isMyDescription = boost::starts_with(port.description, TUNNELEX_NAME " ");
			ports.push_back(port);
		}
		
		GetPortsCache().lastUpdate = ACE_OS::gettimeofday();
		swap(GetPortsCache().ports, ports);
		
	}


	bool CheckMapping(
				const Ports &ports,
				const std::string &externalPort,
				const std::string &internalHost,
				const std::string &internalPort,
				Proto proto,
				const std::string &id)
			const {
		foreach (const Ports::value_type &port, ports) {
			if (boost::iequals(port.externalPort, externalPort) && port.proto == proto) {
				return port.isMyDescription
					&& boost::iequals(port.internalHost, internalHost)
					&& boost::iequals(port.internalPort, internalPort)
					&& boost::iends_with(port.description, std::string(":") + id);
			}
		}
		return false;
	}

public:

	const char * ProtoToStr(Proto proto) const {
		BOOST_ASSERT(proto == PROTO_TCP || proto == PROTO_UDP);
		return proto == PROTO_TCP ? "TCP" : "UDP";
	}

	Proto StrToProto(const std::string &str) const {
		if (boost::iequals(str, "tcp")) {
			return PROTO_TCP;
		} else if (boost::iequals(str, "udp")) {
			return PROTO_UDP;
		} else {
			throw LocalException(L"UPnP client: could not resolve protocol");
		}
	}

	PortsCache & GetPortsCache() throw() {
		static PortsCache cache;
		return cache;
	}

	const PortsCache & GetPortsCache() const throw() {
		return const_cast<Implementation *>(this)->GetPortsCache();
	}

	bool CheckMapping(
				const std::string &externalPort,
				const std::string &internalHost,
				const std::string &internalPort,
				Proto proto,
				const std::string &id)
			const {

		bool result = false;
		bool isChecked = false;
	
		{
			PortsCache::ReadLock lock(const_cast<Implementation *>(this)->GetPortsCache().mutex);
			if (GetPortsCache().IsActual()) {
				isChecked = true;
				result = CheckMapping(
					GetPortsCache().ports,
					externalPort,
					internalHost,
					internalPort,
					proto,
					id);
			}
		}
	
		if (!result) {
			PortsCache::WriteLock lock(
				const_cast<Implementation *>(this)->GetPortsCache().mutex);
			if (!isChecked && GetPortsCache().IsActual()) {
				result = CheckMapping(
					GetPortsCache().ports,
					externalPort,
					internalHost,
					internalPort,
					proto,
					id);
			}
			if (!result) {
				const_cast<Implementation *>(this)->RefreshPortsCache(lock);
			}
		}

		if (!result) {
			PortsCache::ReadLock lock(
				const_cast<Implementation *>(this)->GetPortsCache().mutex);
			BOOST_ASSERT(GetPortsCache().IsActual());
			result = CheckMapping(
				GetPortsCache().ports,
				externalPort,
				internalHost,
				internalPort,
				proto,
				id);
		}

		return result;

	}

	void ResetPortsCache(PortsCache::WriteLock &) {
		GetPortsCache().lastUpdate = ACE_Time_Value::zero;
		Ports().swap(GetPortsCache().ports);
	}

};

//////////////////////////////////////////////////////////////////////////

Client::Client()
		: m_pimpl(new Implementation) {
	
	m_pimpl->m_devList = m_pimpl->m_lib.Discover();
	if (Log::GetInstance().IsDebugRegistrationOn()) {
		std::ostringstream oss;
		oss << "Found UPnP devices: ";
		size_t count = 1;
		for (	UPNPDev *device = m_pimpl->m_devList.get();
				device;
				device = device->pNext) {
			oss << count << ": " << device->descURL << "(" << device->st << ")";
			if (device->pNext) {
				oss << ", ";
				++count;
			}
		}
		oss << ".";
		Log::GetInstance().AppendDebug(oss.str().c_str());
	}

	switch (
			m_pimpl->m_lib.GetValidIgd(
				*m_pimpl->m_devList,
				m_pimpl->m_urls,
				m_pimpl->m_data,
				m_pimpl->m_lanAddr)) {
		case 1:
			Log::GetInstance().AppendDebug(
				"Found valid IGD: %1%, local LAN IP: %2%.",
				m_pimpl->m_urls.impl.controlURL,
				m_pimpl->m_lanAddr);
			break;
		case 2:
			Log::GetInstance().AppendDebug(
				"Found valid IGD: %1% (not connected), local LAN IP: %2%.",
				m_pimpl->m_urls.impl.controlURL,
				m_pimpl->m_lanAddr);
			break;
		case 3:
			Log::GetInstance().AppendDebug(
				"UPnP device has been found but was not recognized as an IGD: %1%, local LAN IP: %2%.",
				m_pimpl->m_urls.impl.controlURL,
				m_pimpl->m_lanAddr);
			break;
		default:
			Log::GetInstance().AppendDebug(
				"Find unknown device: %1%, local LAN IP: %2%.",
				m_pimpl->m_urls.impl.controlURL,
				m_pimpl->m_lanAddr);
	}

}

Client::~Client() throw() {
	delete m_pimpl;
}

const std::string & Client::GetLocalIpAddress() const {
	return m_pimpl->m_lanAddr;
}

std::string Client::GetExternalIpAddress() const {
	const std::string result
		= m_pimpl->m_lib.GetExternalIPAddress(m_pimpl->m_urls, m_pimpl->m_data);
	Log::GetInstance().AppendDebug(
		"External address for UPnP device: %1%.",
		result);
	return result;
}

const char * Client::ProtoToStr(Proto proto) const {
	return m_pimpl->ProtoToStr(proto);
}

void Client::AddPortMapping(
			const std::string &externalPort,
			const std::string &internalHost,
			const std::string &internalPort,
			Proto proto,
			const std::string &id,
			bool force) {

	Implementation::PortsCache::WriteLock cacheLock(m_pimpl->GetPortsCache().mutex);

	if (force) {
		m_pimpl->m_lib.DeletePortMapping(
			m_pimpl->m_urls,
			m_pimpl->m_data,
			externalPort.c_str(),
			ProtoToStr(proto));
	} else {
		for (size_t i = 0; ; ++i) {
			std::string currExternalPort;
			std::string currInternalHost;
			std::string currInternalPort;
			std::string currDescription;
			std::string currProto;
			const bool isFound = m_pimpl->m_lib.GetGenericPortMappingEntry(
				m_pimpl->m_urls,
				m_pimpl->m_data,
				i,
				currExternalPort,
				currInternalHost,
				currInternalPort,
				currProto,
				currDescription);
			if (!isFound) {
				break;
			}
			if (boost::iequals(currExternalPort, externalPort)) {
				if (	boost::iequals(currInternalHost, internalHost)
						&& boost::iequals(currInternalPort, internalPort)
						&& boost::iequals(currProto, ProtoToStr(proto))) {
					Log::GetInstance().AppendDebug(
						"Recreating UPnP port mapping: %1% to %2%:%3% (\"%4%\")...",
						currExternalPort,
						currInternalHost,
						currInternalPort,
						currDescription);
					m_pimpl->m_lib.DeletePortMapping(
						m_pimpl->m_urls,
						m_pimpl->m_data,
						externalPort.c_str(),
						ProtoToStr(proto));
					break;
				} else {
					WFormat message(
						L"Could not add new UPnP port mapping:"
							L" port %1% already used by another application");
					message % ConvertString<WString>(externalPort.c_str()).GetCStr();
					throw ConnectionOpeningException(message.str().c_str());
				}
			}
		}
	}

	Format description(TUNNELEX_NAME " %1%:%2%");
	description % m_pimpl->m_lanAddr % id;

	m_pimpl->m_lib.AddPortMapping(
		m_pimpl->m_urls,
		m_pimpl->m_data,
		externalPort.c_str(),
		internalPort.c_str(),
		internalHost.c_str(),
		ProtoToStr(proto),
		description.str().c_str());
	m_pimpl->ResetPortsCache(cacheLock);

	Log::GetInstance().AppendDebug(
		"Added new port mapping %1% %2% to %3%:%4%.",
		ProtoToStr(proto),
		externalPort,
		internalHost,
		internalPort);

}

bool Client::CheckMapping(
			const std::string &externalPort,
			const std::string &internalHost,
			const std::string &internalPort,
			Proto proto,
			const std::string &id)
		const {
	return m_pimpl->CheckMapping(
		externalPort,
		internalHost,
		internalPort,
		proto,
		id);
}

bool Client::DeletePortMapping(const std::string &id) throw() {
	try {
		Implementation::PortsCache::WriteLock cacheLock(m_pimpl->GetPortsCache().mutex);
		const std::string idSearch = ":" + id;
		for (size_t i = 0; ; ++i) {
			std::string currExternalPort;
			std::string currInternalHost;
			std::string currInternalPort;
			std::string currDescription;
			std::string currProto;
			const bool isFound = m_pimpl->m_lib.GetGenericPortMappingEntry(
				m_pimpl->m_urls,
				m_pimpl->m_data,
				i,
				currExternalPort,
				currInternalHost,
				currInternalPort,
				currProto,
				currDescription);
			if (!isFound) {
				Log::GetInstance().AppendDebug(
					"Could not find UPnP port mapping with ID \"%1%\" for deletion.",
					id);
				return false;
			} else if (
					boost::starts_with(currDescription, TUNNELEX_NAME " ")
					&& boost::iends_with(currDescription, idSearch)) {
				const bool result = m_pimpl->m_lib.DeletePortMapping(
					m_pimpl->m_urls,
					m_pimpl->m_data,
					currExternalPort.c_str(),
					currProto.c_str());
				if (result) {
					Log::GetInstance().AppendDebug(
						"Deleted UPnP port mapping %1% %2% to %3%:%4%.",
						currProto,
						currExternalPort,
						currInternalHost,
						currInternalPort);
				} else {
					Log::GetInstance().AppendDebug(
						"Could not delete UPnP port mapping %1% %2% to %3%:%4%.",
						currProto,
						currExternalPort,
						currInternalHost,
						currInternalPort);
				}
				m_pimpl->ResetPortsCache(cacheLock);
				return result;
			}
		}
	} catch (const TunnelEx::LocalException &ex) {
		WFormat message(
			L"Error in Mods::Upnp::Client::RemovePortMapping: %1%.");
		message % ex.GetWhat();
		Log::GetInstance().AppendDebug(
			ConvertString<String>(message.str().c_str()).GetCStr());
		BOOST_ASSERT(false);
	} catch (const std::exception &ex) {
		Log::GetInstance().AppendDebug(
			"Error (std) in Mods::Upnp::Client::RemovePortMapping: \"%1%\".",
			ex.what());
		BOOST_ASSERT(false);
	} catch (...) {
		Log::GetInstance().AppendDebug(
			"Unknown error in Mods::Upnp::Client::RemovePortMapping.");
		BOOST_ASSERT(false);
	}
	return false;
}
