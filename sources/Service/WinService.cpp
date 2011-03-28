/**************************************************************************
 *   Created: 2007/12/27 0:36
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 * -------------------------------------------------------------------
 *       $Id: WinService.cpp 1032 2010-10-14 14:20:52Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "WinService.hpp"
#include "ServiceControl/Configuration.hpp"
#include "ConnectionAcceptEvent.hpp"
#include "TexServiceImplementation.hpp"
#include "ServiceEndpointBroadcaster.hpp"

#include <TunnelEx/Log.hpp>
#include <TunnelEx/Server.hpp>
#include <TunnelEx/Exceptions.hpp>
#include <TunnelEx/String.hpp>

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace TunnelEx;

//////////////////////////////////////////////////////////////////////////

TexWinService::TexWinService() {
	BOOST_ASSERT(!m_texService.get());
	m_texService.reset(new TexServiceImplementation);
	soap_init2(&m_soap, SOAP_IO_KEEPALIVE, SOAP_IO_KEEPALIVE);
	m_soap.max_keep_alive = 1000;
}

TexWinService::~TexWinService() {
	ServiceEndpointBroadcaster().CallbackAll();
	soap_done(&m_soap);
	{
		mutex::scoped_lock lock(m_connectionRemoveMutex);
		const Connections::const_iterator end(m_connections.end());
		for (Connections::const_iterator i(m_connections.begin()); i != end; ++i) {
			soap_done(*i);
		}
		Connections().swap(m_connections);
	}
	m_adminThreads.join_all();
	BOOST_ASSERT(m_texService.get());
	m_texService.reset();
}

void TexWinService::HandleSoapRequest() {

	SOAP_SOCKET acceptedSocket = soap_accept(&m_soap);
	if (acceptedSocket < 0) {
		LogSoapError();
		return;
	} else if (Log::GetInstance().IsDebugRegistrationOn()) {
		Log::GetInstance().AppendDebug(
			"Accepted administrative connection from %d.%d.%d.%d",
			(m_soap.ip >> 24) & 0xFF,
			(m_soap.ip >> 16) & 0xFF,
			(m_soap.ip >> 8) & 0xFF,
			m_soap.ip & 0xFF); 
	}
	
	if (2130706433 != m_soap.ip) { // 127.0.0.1
		Log::GetInstance().AppendFatalError(
			"Rejecting external administrative connection, access denied.");
	} else {
		soap *connection = soap_copy(&m_soap);
		m_connections.insert(connection);
		m_adminThreads.create_thread(
			boost::bind(&TexWinService::SoapServeThread, this, connection));

	}

	soap_destroy(&m_soap);
	soap_end(&m_soap);

}

void TexWinService::SoapServeThread(soap *soap) {
	
	class Cleaner : private noncopyable {
	public:
		Cleaner(::soap *soap, Connections &connections, mutex &connectionsMutex)
				: m_soap(soap),
				m_connections(connections),
				m_connectionsMutex(connectionsMutex) {
			//...//
		}
		~Cleaner() {
			soap_destroy(m_soap);
			soap_end(m_soap);
			{
				mutex::scoped_lock lock(m_connectionsMutex);
				const Connections::iterator pos(m_connections.find(m_soap));
				if (pos != m_connections.end()) {
					soap_done(m_soap);
					m_connections.erase(pos);
				}
			}
			free(m_soap);
			Log::GetInstance().AppendDebug("Handling of administrative request finished.");
		}
	private:
		::soap *m_soap;
		Connections &m_connections;
		mutex &m_connectionsMutex;
	};

	Cleaner cleaner(soap, m_connections, m_connectionRemoveMutex);
	const int serveResult = soap_serve(soap);
	if (serveResult != SOAP_OK && serveResult != SOAP_EOF && serveResult != SOAP_NO_METHOD) {
		LogSoapError();
	}

}

//! \todo: will be dangerous in multi-threading [2008/02/17 5:39]
shared_ptr<TexServiceImplementation> TexWinService::m_texService;

shared_ptr<TexServiceImplementation> TexWinService::GetTexServiceInstance() {
	//! \todo: will be dangerous in multi-threading [2008/02/17 5:39]
	return m_texService;
}

void TexWinService::RunSoapServer(HANDLE stopEvent) {
	const char *const serviceHost = "localhost";
	SOAP_SOCKET masterSocket = SOAP_INVALID_SOCKET;
	for (
			int port = TUNNELEX_SOAP_SERVICE_PORT, i = 0;
			i < 2 && masterSocket == SOAP_INVALID_SOCKET && (!i || m_soap.error != WSAEACCES);
			port = 0, ++i) {
		masterSocket = soap_bind(&m_soap, serviceHost, port, 100);
	}
	unsigned char eventsNumb = 1;
	HANDLE events[2] = {stopEvent, NULL};
	shared_ptr<ConnectionAcceptEvent> acceptEvent;
	if (masterSocket < 0) {
		LogSoapError();
	} else {
		sockaddr_in hostInfo;
		int hostInfoSize = sizeof(hostInfo);
		if (getsockname(masterSocket, reinterpret_cast<sockaddr*>(&hostInfo), &hostInfoSize)) {
			BOOST_ASSERT(false);
		}
		m_soap.port = ntohs(hostInfo.sin_port);
		acceptEvent.reset(new ConnectionAcceptEvent(masterSocket));
		eventsNumb = 2;
		events[1] = acceptEvent->GetEvent();
	}
	ServiceEndpointBroadcaster().Broadcast(serviceHost, m_soap.port, false);
	for ( ; ; ) {
		const DWORD event = WaitForMultipleObjects(eventsNumb, events, FALSE, INFINITE);
		switch (event) {
			case WAIT_OBJECT_0 + 1:
				if (eventsNumb > 1) {
					HandleSoapRequest();
					ResetEvent(events[1]);
					continue;
				}
			default:
				LogUnexpectedEvent(event);
			case WAIT_OBJECT_0:
				return;
		}
	}
}

void TexWinService::Run(HANDLE stopEvent) {
	RunSoapServer(stopEvent);
	GetTexServiceInstance()->Stop();
}

void TexWinService::LogSoapError() const {
	ostringstream oss;
	soap_stream_fault(&m_soap, oss);
	string soapError(
		regex_replace(oss.str(), regex("[\n\t\r]"), " ", match_default | format_all));
	trim(soapError);
	if (soapError.empty()) {
		soapError = "Unknown SOAP error.";
	}
	Log::GetInstance().AppendFatalError(soapError);
}

void TexWinService::LogUnexpectedEvent(const DWORD event) const {
	Format message("Unexpected event %1% in main service loop (system error %1%).");
	message % event;
	message % GetLastError();
	Log::GetInstance().AppendSystemError(message.str());
}

//////////////////////////////////////////////////////////////////////////

const wchar_t* const g_serviceName = TUNNELEX_NAME_W;

SERVICE_STATUS g_serviceStatus;
SERVICE_STATUS_HANDLE g_serviceStatusHandle;
HANDLE g_serviceShutdownEvent = NULL;
HANDLE g_serviceShutdownedEvent = NULL;

//! Controller of the service.
void WINAPI ServiceCtrlHandler(DWORD opcode) {
	Log::GetInstance().AppendDebug(
		(Format("Service control handler called with command \"%1%\".") % opcode).str());
	switch (opcode) { 
		case SERVICE_CONTROL_PAUSE:
			Log::GetInstance().AppendDebug("SERVICE_CONTROL_PAUSE");
			g_serviceStatus.dwCurrentState = SERVICE_PAUSED; 
			break; 
		case SERVICE_CONTROL_CONTINUE:
			Log::GetInstance().AppendDebug("SERVICE_CONTROL_CONTINUE");
			g_serviceStatus.dwCurrentState = SERVICE_RUNNING; 
			break; 
		case SERVICE_CONTROL_STOP: 
			Log::GetInstance().AppendDebug("Sending command to service - stop.");
			SetEvent(g_serviceShutdownEvent);
			break;
		case SERVICE_CONTROL_INTERROGATE: 
			Log::GetInstance().AppendDebug("SERVICE_CONTROL_INTERROGATE");
			break;
		default:
			Log::GetInstance().AppendWarn(
				(Format("Unknown service control command - \"%1%\".") % opcode).str());
	}
}

//! Main function of the service.
void WINAPI ServiceMain(DWORD, LPWSTR*) {
	
	g_serviceStatus.dwServiceType				= SERVICE_WIN32_OWN_PROCESS; 
	g_serviceStatus.dwCurrentState				= SERVICE_RUNNING; 
	g_serviceStatus.dwControlsAccepted			= SERVICE_ACCEPT_STOP; 
	g_serviceStatus.dwWin32ExitCode				= 0; 
	g_serviceStatus.dwServiceSpecificExitCode	= 0; 
	g_serviceStatus.dwCheckPoint				= 0; 
	g_serviceStatus.dwWaitHint					= 0; 
	
	g_serviceStatusHandle = RegisterServiceCtrlHandler(g_serviceName, &ServiceCtrlHandler);
	if (!g_serviceStatusHandle) {
		Log::GetInstance().AppendSystemError("Service handle registration has been failed.");
		return;
	}

	g_serviceShutdownEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	SetServiceStatus(g_serviceStatusHandle, &g_serviceStatus);
	Log::GetInstance().AppendInfo("Service stated.");

	///////////////////////////////////////////////////////////////////////

	try {
		TexWinService().Run(g_serviceShutdownEvent);
	} catch (const TunnelEx::LocalException&) {
		/*...*/
	}

	///////////////////////////////////////////////////////////////////////
	
	CloseHandle(g_serviceShutdownEvent);

	g_serviceStatus.dwWin32ExitCode	= 0; 
	g_serviceStatus.dwCurrentState	= SERVICE_STOPPED; 
	g_serviceStatus.dwCheckPoint	= 0; 
	g_serviceStatus.dwWaitHint		= 0;
	Log::GetInstance().AppendInfo("Service stopped.");
	SetServiceStatus(g_serviceStatusHandle, &g_serviceStatus);

}

//! Starts a service.
void RunAsTexService() {
	Log::GetInstance().AppendDebug("Starting service...");
	const SERVICE_TABLE_ENTRYW dispatchTable[] = {
		{const_cast<wchar_t*>(g_serviceName), &ServiceMain},
		{NULL, NULL}
	};
	StartServiceCtrlDispatcher(dispatchTable);
}


//! Stops a broadcasting service.
DWORD StopService(		SC_HANDLE hSCM
				   ,	SC_HANDLE hService
				   ,	BOOL fStopDependencies
				   ,	DWORD dwTimeout) {

	Log::GetInstance().AppendDebug("Stopping service...");

	SERVICE_STATUS ss;
	DWORD dwStartTime = GetTickCount();

	// Make sure the service is not already stopped
	if (!QueryServiceStatus(hService, &ss)) {
		return GetLastError();
	}

	if (ss.dwCurrentState == SERVICE_STOPPED)  {
		return ERROR_SUCCESS;
	}

	// If a stop is pending, just wait for it
	while (ss.dwCurrentState == SERVICE_STOP_PENDING) {
		Sleep(ss.dwWaitHint);
		if (!QueryServiceStatus(hService, &ss)) {
			return GetLastError();
		}
		if (ss.dwCurrentState == SERVICE_STOPPED) {
			return ERROR_SUCCESS;
		}
		if (GetTickCount() - dwStartTime > dwTimeout) {
			return ERROR_TIMEOUT;
		}
	}

	// If the service is running, dependencies must be stopped first
	if (fStopDependencies) {
		DWORD i;
		DWORD dwBytesNeeded;
		DWORD dwCount;

		ENUM_SERVICE_STATUS		ess;
		struct Handlers {
			LPENUM_SERVICE_STATUS	lpDependencies;
			SC_HANDLE				hDepService;
			Handlers() : lpDependencies(NULL), hDepService(NULL) {}
			~Handlers() {
				if (lpDependencies != NULL) {
					HeapFree(GetProcessHeap(), 0, lpDependencies);
				}
				if (hDepService != NULL) {
					CloseServiceHandle(hDepService);
				}
			}
		} handlers;

		// Pass a zero-length buffer to get the required buffer size
		if (EnumDependentServices(hService, SERVICE_ACTIVE, handlers.lpDependencies, 0, &dwBytesNeeded, &dwCount)) {
			// If the Enum call succeeds, then there are no dependent
			// services so do nothing
		} else {
			if (GetLastError() != ERROR_MORE_DATA) {
				return GetLastError(); // Unexpected error
			}
			// Allocate a buffer for the dependencies
			handlers.lpDependencies = (LPENUM_SERVICE_STATUS)HeapAlloc( 
				GetProcessHeap(), HEAP_ZERO_MEMORY, dwBytesNeeded );
			if (!handlers.lpDependencies) {
				return GetLastError();
			}
			// Enumerate the dependencies
			if (!EnumDependentServices(hService, SERVICE_ACTIVE,
					handlers.lpDependencies, dwBytesNeeded, &dwBytesNeeded, &dwCount)) {
				return GetLastError();
			}
			for (i = 0; i < dwCount; i++) {
				ess = *(handlers.lpDependencies + i);
				// Open the service
				handlers.hDepService = OpenService( hSCM, ess.lpServiceName, 
					SERVICE_STOP | SERVICE_QUERY_STATUS );
				if (!handlers.hDepService) {
					return GetLastError();
				}
				// Send a stop code
				if (!ControlService(handlers.hDepService, SERVICE_CONTROL_STOP, &ss)) {
					return GetLastError();
				}
				// Wait for the service to stop
				while (ss.dwCurrentState != SERVICE_STOPPED) {
					Sleep(ss.dwWaitHint);
					if (!QueryServiceStatus(handlers.hDepService, &ss)) {
						return GetLastError();
					}
					if (ss.dwCurrentState == SERVICE_STOPPED) {
						break;
					}
					if (GetTickCount() - dwStartTime > dwTimeout) {
						return ERROR_TIMEOUT;
					}
				}
			}
		}
	}

	// Send a stop code to the main service
	if (!ControlService(hService, SERVICE_CONTROL_STOP, &ss)) {
		return GetLastError();
	}

	// Wait for the service to stop
	while (ss.dwCurrentState != SERVICE_STOPPED) {
		Sleep(ss.dwWaitHint);
		if (!QueryServiceStatus(hService, &ss)) {
			return GetLastError();
		}
		if (ss.dwCurrentState == SERVICE_STOPPED) {
			break;
		}
		if (GetTickCount() - dwStartTime > dwTimeout) {
			return ERROR_TIMEOUT;
		}
	}
	
	// Return success
	return ERROR_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

//! Installs service.
bool InstallTexService() {
	
	SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!scManager) {
		const DWORD err = GetLastError();
		WString message;
		if (err == ERROR_ACCESS_DENIED) {
			message =	L"Services database opening failed:"
						L" The requested access was denied.";
		} else {
			WFormat messagef(L"Services database opening failed with error code \"%1%\".");
			message = (messagef % err).str().c_str();
		}
		Log::GetInstance().AppendError(ConvertString<String>(message).GetCStr());
		MessageBox(NULL, message.GetCStr(), L"Service error", MB_OK | MB_ICONERROR);
		return FALSE;
	}

	bool isSuccess = true;

	const wstring serviceBinary = Helpers::GetModuleFilePath().string() + L" --service";

	SC_HANDLE service = CreateServiceW(
		scManager,
		g_serviceName,
		g_serviceName,						// service name to display
		SERVICE_ALL_ACCESS,					// desired access
		SERVICE_WIN32_OWN_PROCESS,			// service type
		SERVICE_AUTO_START,					// start type
		SERVICE_ERROR_NORMAL,				// error control type 
		serviceBinary.c_str(),				// service's binary 
		NULL,								// no load ordering group 
		NULL,								// no tag identifier 
		NULL,								// no dependencies 
		NULL,								// LocalSystem account 
		NULL								// no password
	);                      
	
	if (!service) {
		const DWORD err = GetLastError();
		WString message;
		if (err == ERROR_SERVICE_EXISTS) {
			message	=	L"Service creating failed: "
						L"The specified service already exists.";
		} else {
			WFormat messagef(L"Service creating failed with error code \"%1%\".");
			message = (messagef % err).str().c_str();
		}
		Log::GetInstance().AppendError(ConvertString<String>(message).GetCStr());
		MessageBox(NULL, message.GetCStr(), L"Service error", MB_OK | MB_ICONERROR);
		isSuccess = false;
	} else {
		CloseServiceHandle(service); 
	}
	
	CloseServiceHandle(scManager);

	return isSuccess;

}

//! Uninstalls service
bool UninstallTexService() {
	
	SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!scManager) {
		const DWORD err = GetLastError();
		WString message;
		if (err == ERROR_ACCESS_DENIED) {
			message =	L"Services database opening failed:"
						L" The requested access was denied.";
		} else {
			WFormat messagef(L"Services database opening failed with error code \"%1%\".");
			message = (messagef % err).str().c_str();
		}
		Log::GetInstance().AppendError(ConvertString<String>(message).GetCStr());
		MessageBox(NULL, message.GetCStr(), L"Service error", MB_OK | MB_ICONERROR);
		return FALSE;
	}
	
	bool isSuccess = true;
	
	SC_HANDLE service = OpenService(scManager, g_serviceName, SERVICE_ALL_ACCESS);
	if (!service) {
		const DWORD err = GetLastError();
		WString message;
		if (err == ERROR_SERVICE_DOES_NOT_EXIST) {
			message	=	L"Service removing failed: "
						L"The specified service does not exist as an installed service.";
		} else {
			WFormat messagef(L"Service opening failed with error code \"%1%\".");
			message = (messagef % err).str().c_str();
		}
		Log::GetInstance().AppendError(ConvertString<String>(message).GetCStr());
		MessageBox(NULL, message.GetCStr(), L"Service error", MB_OK | MB_ICONERROR);
		isSuccess = false;
	} else {
		StopService(scManager, service, TRUE, 1000*60);
		if (!DeleteService(service)) {
			const DWORD err = GetLastError();
			WString message;
			if (err == ERROR_SERVICE_MARKED_FOR_DELETE) {
				message	=	L"Service removing failed: "
							L"The specified service has been marked for deletion.";
			} else {
				WFormat messagef(L"Service removing failed with error code \"%1%\".");
				message = (messagef % err).str().c_str();
			}
			Log::GetInstance().AppendError(ConvertString<String>(message).GetCStr());
			MessageBox(NULL, message.GetCStr(), L"Service error", MB_OK | MB_ICONERROR);
			isSuccess = false;
		}
		CloseServiceHandle(service);
	}

	CloseServiceHandle(scManager);

	return isSuccess;

}
