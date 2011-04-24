/**************************************************************************
 *   Created: 2008/02/24 4:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "ServiceEndpointBroadcaster.hpp"

#include "Core/Log.hpp"
#include "Core/String.hpp"

using namespace TunnelEx;

ServiceEndpointBroadcaster::ServiceEndpointBroadcaster()
		: m_serviceSubKeyName(L"SOFTWARE\\" TUNNELEX_NAME_W L"\\Service"),
		m_endpointSubKeyName(L"InstanceEndpoint") {
	const LONG result = RegCreateKeyExW(
		HKEY_LOCAL_MACHINE,
		m_serviceSubKeyName,
		0,
		NULL,
		0,
		KEY_ALL_ACCESS,
		NULL,
		&m_key,
		NULL);
	assert(result == ERROR_SUCCESS);
	if (result != ERROR_SUCCESS) {
		HandleError("Could not store service instance endpoint (error code: %1%).", result);
	}
}

ServiceEndpointBroadcaster::~ServiceEndpointBroadcaster() {
	if (RegCloseKey(m_key) != ERROR_SUCCESS) {
		assert(false);
	}
}

void ServiceEndpointBroadcaster::Broadcast(const char* host, int port, bool isSecured) {
	Format endpointFormat("%1%://%2%:%3%");
	endpointFormat % (!isSecured ? "http" : "https") % host % port;
	typedef WString RegStringType;
	const RegStringType endpoint(ConvertString<RegStringType>(endpointFormat.str().c_str()));
#	pragma warning(push)
#	pragma warning(disable: 4244)
	const LONG result = RegSetValueExW(
		m_key, m_endpointSubKeyName, 0, REG_SZ,
		reinterpret_cast<const BYTE*>(endpoint.GetCStr()),
		(endpoint.GetLength() + 1) * sizeof(RegStringType::value_type));
#	pragma warning(pop)
	assert(result == ERROR_SUCCESS);
	if (result != ERROR_SUCCESS) {
		HandleError("Could not store service instance endpoint (error code: %1%).", result);
	}
}

void ServiceEndpointBroadcaster::CallbackAll() {
	const LONG result = RegDeleteKeyW(m_key, L"");
	assert(result == ERROR_SUCCESS);
	if (result != ERROR_SUCCESS) {
		HandleError("Could not delete service instance endpoint (error code: %1%).", result);
	}
}

void ServiceEndpointBroadcaster::HandleError(const char* message, DWORD errorCode) {
	Format format(message);
	format % errorCode;
	Log::GetInstance().AppendSystemError(format.str());
}
