/**************************************************************************
 *   Created: 2008/01/05 13:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Control.cpp 1032 2010-10-14 14:20:52Z palchukovsky $
 **************************************************************************/

#include "Prec.h"
#include "Control.hpp"

//////////////////////////////////////////////////////////////////////////

class ServiceControl::Implementation : private boost::noncopyable {

public:

	Implementation(const DWORD accessLevel)
			: manager(OpenSCManager(NULL, NULL, accessLevel)),
			service(NULL) {
		HandleError(manager != NULL);
		service = OpenServiceW(manager, TUNNELEX_NAME_W, accessLevel);
		if (service == NULL) {
			CloseServiceHandle(manager);
		}
		HandleError(service != NULL);
	}

	~Implementation() {
		if (service) {
			CloseServiceHandle(service);
		}
		if (manager) {
			CloseServiceHandle(manager);
		}
	}

	void ChangeStartType(const DWORD newType) {
		const BOOL result = ChangeServiceConfig(
			service, SERVICE_NO_CHANGE, newType,
			SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		HandleError(result);
	}

	bool QueryServiceStatus(SERVICE_STATUS_PROCESS& statusInfo) {
		DWORD bytesNeeded;
		return QueryServiceStatusEx(
			service, SC_STATUS_PROCESS_INFO,
			reinterpret_cast<LPBYTE>(&statusInfo), sizeof(SERVICE_STATUS_PROCESS),
			&bytesNeeded) == TRUE;
	}

	void WaitPending(const DWORD status, bool suchStatus) {
		SERVICE_STATUS_PROCESS statusInfo;
		if (QueryServiceStatus(statusInfo)) {
			DWORD startTickCount = GetTickCount();
			DWORD oldCheckPoint = statusInfo.dwCheckPoint;
			while (	suchStatus
					?	statusInfo.dwCurrentState == status
					:	statusInfo.dwCurrentState != status)  { 
				DWORD waitTime = statusInfo.dwWaitHint / 10;
				if (waitTime < 1000) {
					waitTime = 1000;
				} else if (waitTime > 10000) {
					waitTime = 10000;
				}
				Sleep(waitTime);
				if (!QueryServiceStatus(statusInfo)) {
					break;
				}
				if (statusInfo.dwCheckPoint > oldCheckPoint) {
					startTickCount = GetTickCount();
					oldCheckPoint = statusInfo.dwCheckPoint;
				} else if ((GetTickCount() - startTickCount) > statusInfo.dwWaitHint) {
					break;
				}
			}
		}
	}

	void HandleError(BOOL result) const {
		if (!result) {
			const DWORD err = GetLastError();
			if (err == ERROR_ACCESS_DENIED) {
				throw ServiceControl::ServiceControlAccessDeniedException(err);
			} else if (err == ERROR_SERVICE_DOES_NOT_EXIST) {
				throw ServiceControl::ServiceControlServiceDoesntExistException(err);
			} else {
				throw ServiceControl::ServiceControlException(err);
			}
		}
	}

public:
	
	SC_HANDLE manager;
	SC_HANDLE service;

};

//////////////////////////////////////////////////////////////////////////

ServiceControl::ServiceControl() {
	//...//
}

ServiceControl::~ServiceControl() {
	//...//
}

bool ServiceControl::Start() {
	boost::shared_ptr<Implementation> impl(
		new Implementation(SERVICE_START | SERVICE_CHANGE_CONFIG | SERVICE_QUERY_STATUS));
	impl->ChangeStartType(SERVICE_AUTO_START);
	impl->HandleError(StartService(impl->service, 0, NULL));
	impl->WaitPending(SERVICE_START_PENDING, true);
	SERVICE_STATUS_PROCESS statusInfo;
	return impl->QueryServiceStatus(statusInfo)
		?	statusInfo.dwCurrentState == SERVICE_RUNNING
		:	false;
}

bool ServiceControl::Stop() {
	boost::shared_ptr<Implementation> impl(
		new Implementation(SERVICE_STOP | SERVICE_CHANGE_CONFIG | SERVICE_QUERY_STATUS));
	impl->ChangeStartType(SERVICE_DEMAND_START);
	SERVICE_STATUS statusInfo;
	impl->HandleError(ControlService(impl->service, SERVICE_CONTROL_STOP, &statusInfo));
	impl->WaitPending(SERVICE_STOP_PENDING, false);
	SERVICE_STATUS_PROCESS statusInfoProcess;
	return impl->QueryServiceStatus(statusInfoProcess)
		?	statusInfoProcess.dwCurrentState == SERVICE_STOPPED
		:	false;
}

bool ServiceControl::IsStarted() const {
	boost::shared_ptr<Implementation> impl(new Implementation(SERVICE_QUERY_STATUS));
	SERVICE_STATUS_PROCESS statusInfo;
	impl->HandleError(impl->QueryServiceStatus(statusInfo));
	return statusInfo.dwCurrentState == SERVICE_RUNNING;
}

bool ServiceControl::IsStopped() const {
	boost::shared_ptr<Implementation> impl(new Implementation(SERVICE_QUERY_STATUS));
	SERVICE_STATUS_PROCESS statusInfo;
	impl->HandleError(impl->QueryServiceStatus(statusInfo));
	return statusInfo.dwCurrentState == SERVICE_STOPPED;
}

bool ServiceControl::WaitStartPending() const {
	boost::shared_ptr<Implementation> impl(new Implementation(SERVICE_QUERY_STATUS));
	impl->WaitPending(SERVICE_START_PENDING, true);
	SERVICE_STATUS_PROCESS statusInfo;
	impl->HandleError(impl->QueryServiceStatus(statusInfo));
	return statusInfo.dwCurrentState == SERVICE_RUNNING;
}

bool ServiceControl::WaitStopPending() const {
	boost::shared_ptr<Implementation> impl(new Implementation(SERVICE_QUERY_STATUS));
	impl->WaitPending(SERVICE_STOP_PENDING, false);
	SERVICE_STATUS_PROCESS statusInfo;
	impl->HandleError(impl->QueryServiceStatus(statusInfo));
	return statusInfo.dwCurrentState == SERVICE_STOPPED;
}

//////////////////////////////////////////////////////////////////////////
