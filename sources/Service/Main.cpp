/**************************************************************************
 *   Created: 2007/12/27 0:00
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.com
 * -------------------------------------------------------------------
 *       $Id: Main.cpp 1086 2010-12-07 08:53:15Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "WinService.hpp"
#include "ServiceControl/Control.hpp"

#include <TunnelEx/Exceptions.hpp>
#include <TunnelEx/Server.hpp>
#include <TunnelEx/Log.hpp>

using namespace std;
using namespace boost;
using namespace TunnelEx;

#define TEX_SERVICE_CL_CMD_SERVICE		L"--service"
#define TEX_SERVICE_CL_CMD_INSTALL		L"--install"
#define TEX_SERVICE_CL_CMD_UNINSTALL	L"--uninstall"
#define TEX_SERVICE_CL_CMD_START		L"--start"
#define TEX_SERVICE_CL_CMD_STOP			L"--stop"
#define TEX_SERVICE_CL_CMD_STATUS		L"--status"
#define TEX_SERVICE_CL_CMD_STANDALONE	L"--standalone"

void LaunchControlCenter();
bool RunAsTexService2();
bool RunStandalone();
bool StartTexService();
bool StopTexService();
bool ShowTexServiceStatus();

void XmlErrosNull(void *, const char *, ...) {
	//...//
}

int main(int, const char*[]) {

	int argc;
	const LPWSTR *const argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (argc != 2) {
		LaunchControlCenter();
		return 1;
	}

	int result = -1;
	{

		xmlInitParser();
		TunnelEx::Helpers::Xml::SetErrorsHandler(&XmlErrosNull);
		
		ERR_load_crypto_strings();
		OpenSSL_add_all_algorithms();

		function<bool(void)> func;
		{
			typedef map<wstring, function<bool(void)> > Commands;
			Commands commands;
			commands[TEX_SERVICE_CL_CMD_SERVICE]	= &RunAsTexService2;
			commands[TEX_SERVICE_CL_CMD_INSTALL]	= &InstallTexService;
			commands[TEX_SERVICE_CL_CMD_UNINSTALL]	= &UninstallTexService;
			commands[TEX_SERVICE_CL_CMD_START]		= &StartTexService;
			commands[TEX_SERVICE_CL_CMD_STOP]		= &StopTexService;
			commands[TEX_SERVICE_CL_CMD_STATUS]		= &ShowTexServiceStatus;
			commands[TEX_SERVICE_CL_CMD_STANDALONE]	= &RunStandalone;
			const Commands::const_iterator commandPos = commands.find(argv[1]);
			if (commandPos == commands.end()) {
				result = 2;
			} else {
				func = commandPos->second;
			}
		}
		
		if (func) {
			BOOST_ASSERT(result == -1);
			result = func() ? 0 : 3;
		}

		EVP_cleanup();
		ERR_free_strings();

	}

#	if defined(_DEBUG) || defined(TEST)
		if (!wcscmp(argv[1], TEX_SERVICE_CL_CMD_STANDALONE)) {
			getchar();
		}
#	endif

	return result;

}

void LaunchControlCenter() {
	PROCESS_INFORMATION processInfo;
	STARTUPINFOA startupInfo;
	memset(&startupInfo, 0, sizeof startupInfo);
	startupInfo.cb = sizeof startupInfo;
	const BOOL createResult
		= CreateProcessA(
			0, TUNNELEX_CONTROL_CENTER_EXE_FILE_NAME, 0, 0, FALSE, 0,
			0, 0, &startupInfo, &processInfo);
	if (createResult) {
		CloseHandle(processInfo.hThread);
		CloseHandle(processInfo.hProcess);
	}
}

bool RunAsTexService2() {
	RunAsTexService();
	return true;
}

bool StartTexService() {
	try {
		ServiceControl().Start();
		return true;
	} catch (const ServiceControl::ServiceControlAccessDeniedException&) {
		MessageBoxW(NULL, L"The requested access was denied.", L"Error", MB_OK | MB_ICONSTOP);
	} catch (const ServiceControl::ServiceControlServiceDoesntExistException&) {
		MessageBoxW(NULL, TUNNELEX_NAME_W L" service is not installed.", L"Error", MB_OK | MB_ICONSTOP);
	} catch (const ServiceControl::ServiceControlException& ex) {
		WFormat message(L"Error with code \"%1%\".");
		message % ex.GetError();
		MessageBoxW(NULL, message.str().c_str(), L"Error", MB_OK | MB_ICONSTOP);
	}
	return false;
}

bool StopTexService() {
	try {
		ServiceControl().Stop();
		return true;
	} catch (const ServiceControl::ServiceControlAccessDeniedException&) {
		MessageBoxW(NULL, L"The requested access was denied.", L"Error", MB_OK | MB_ICONSTOP);
	} catch (const ServiceControl::ServiceControlServiceDoesntExistException&) {
		MessageBoxW(NULL, TUNNELEX_NAME_W L" service is not installed.", L"Error", MB_OK | MB_ICONSTOP);
	} catch (const ServiceControl::ServiceControlException& ex) {
		WFormat message(L"Error with code \"%1%\".");
		message % ex.GetError();
		MessageBoxW(NULL, message.str().c_str(), L"Error", MB_OK | MB_ICONSTOP);
	}
	return false;
}

bool ShowTexServiceStatus() {
	try {
		const wchar_t *const status = ServiceControl().IsStarted() ? L"started" : L"stopped";
		MessageBoxW(NULL, status, TUNNELEX_NAME_W L" service status", MB_OK | MB_ICONINFORMATION);
		return true;
	} catch (const ServiceControl::ServiceControlAccessDeniedException&) {
		MessageBoxW(NULL, L"The requested access was denied.", L"Error", MB_OK | MB_ICONSTOP);
	} catch (const ServiceControl::ServiceControlServiceDoesntExistException&) {
		MessageBoxW(NULL, TUNNELEX_NAME_W L" service is not installed.", L"Error", MB_OK | MB_ICONSTOP);
	} catch (const ServiceControl::ServiceControlException& ex) {
		WFormat message(L"Error with code \"%1%\".");
		message % ex.GetError();
		MessageBoxW(NULL, message.str().c_str(), L"Error", MB_OK | MB_ICONSTOP);
	}
	return false;
}

bool RunStandalone() {
	
	Log::GetInstance().SetMinimumRegistrationLevel(TunnelEx::LOG_LEVEL_INFO);
	Log::GetInstance().AttachStderrStream();
	
	class ThreadHolder : private noncopyable {
	public:
		ThreadHolder()
				: noncopyable(),
				m_stopEvent(CreateEvent(NULL, FALSE, TRUE, NULL)) {
			ResetEvent(m_stopEvent);
		}
		~ThreadHolder() {
			CloseHandle(m_stopEvent);
		}
		void Run() {
			try {
				TexWinService().Run(m_stopEvent);
			} catch (const TunnelEx::LocalException& ex) {
				MessageBoxW(NULL, ex.GetWhat(), L"Error!", MB_ICONSTOP | MB_OK);
			}
		}
		void Stop() {
			SetEvent(m_stopEvent);
		}
	private:
		HANDLE m_stopEvent;
	} threadHolder;
	thread thread(boost::bind(&ThreadHolder::Run, &threadHolder));
	
	getchar();
	threadHolder.Stop();
	thread.join();

	return true;

}
