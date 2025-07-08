#include "driverloader.h"
#include <stdio.h>
#include <tchar.h>


void DriverLoader::SetPath(PCTSTR drivername, PCTSTR svcname)
{
	Log(_T("SetPath: drivername=%s, svcname=%s\n"), drivername ? drivername : _T("(null)"), svcname ? svcname : _T("(null)"));
	m_driverName = drivername;
	m_serviceName = svcname;
	return;
}

bool DriverLoader::Load(PCTSTR drivername, PCTSTR svcname)
{
	Log(_T("Load: drivername=%s, svcname=%s\n"), drivername ? drivername : _T("(null)"), svcname ? svcname : _T("(null)"));
	if (drivername == nullptr) drivername = m_driverName;
	if (svcname == nullptr) svcname = m_serviceName;

	SC_HANDLE schSCManager;
	SC_HANDLE schService;

	schSCManager = OpenSCManager(
		NULL,                   // 目标计算机的名称,NULL：连接本地计算机上的服务控制管理器
		NULL,                   // 服务控制管理器数据库的名称，NULL：打开 SERVICES_ACTIVE_DATABASE 数据库
		SC_MANAGER_ALL_ACCESS   // 所有权限
	);

	if (NULL == schSCManager) {
		LogErr(_T("OpenSCManager 失败"));
		return false;
	}

	// Create the service
	schService = CreateService(
		schSCManager,              // SCM database 
		svcname,                   // name of service 
		svcname,                   // service name to display 
		SERVICE_ALL_ACCESS,        // desired access 
		SERVICE_KERNEL_DRIVER, // service type 
		SERVICE_DEMAND_START,      // start type 
		SERVICE_ERROR_NORMAL,      // error control type 
		drivername,                    // path to service's binary 
		NULL,                      // no load ordering group 
		NULL,                      // no tag identifier 
		NULL,                      // no dependencies 
		NULL,                      // LocalSystem account 
		NULL);                     // no password 

	if (schService == NULL) {
		if (ERROR_SERVICE_EXISTS == GetLastError()) {
			schService = OpenService(schSCManager, svcname, SERVICE_ALL_ACCESS);
			if (schService == NULL) {
				LogErr(_T("OpenService 失败"));
				CloseServiceHandle(schSCManager);
				return false;
			}
		}
		else {
			LogErr(_T("CreateService 失败"));
			CloseServiceHandle(schSCManager);
			return false;
		}
	}
	
	Log(_T("Service 安装成功\n"));

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
	return true;
}

bool DriverLoader::Start(PCTSTR svcname)
{
	Log(_T("Start: svcname=%s\n"), svcname ? svcname : _T("(null)"));
	if (svcname == nullptr) svcname = m_serviceName;

	SC_HANDLE schSCManager = NULL;
	SC_HANDLE hs = NULL;
	bool bRet = false;

	do {
		schSCManager = OpenSCManager
		(
			NULL,                   // 目标计算机的名称,NULL：连接本地计算机上的服务控制管理器
			NULL,                   // 服务控制管理器数据库的名称，NULL：打开 SERVICES_ACTIVE_DATABASE 数据库
			SC_MANAGER_ALL_ACCESS   // 所有权限
		);

		if (schSCManager == NULL) {
			LogErr(_T("OpenSCManager 失败"));
			break;
		}

		hs = OpenService(schSCManager, svcname, SERVICE_ALL_ACCESS);
		if (hs == NULL) {
			LogErr(_T("OpenService 失败"));
			break;
		}

		if (!StartService(hs, 0, 0)) {
			LogErr(_T("StartService 失败"));
			break;
		}

		Log(_T("Service start successful."));
		bRet = true;

	} while (false);

	if (hs) CloseServiceHandle(hs);
	if (schSCManager) CloseServiceHandle(schSCManager);
	return bRet;
}

bool DriverLoader::Stop(PCTSTR svcname)
{
	Log(_T("Stop: svcname=%s\n"), svcname ? svcname : _T("(null)"));
	if (svcname == nullptr) svcname = m_serviceName;
	SC_HANDLE schSCManager = NULL;
	SC_HANDLE hs = NULL;
	bool bRet = false;

	do {
		schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (!schSCManager) {
			LogErr(_T("OpenSCManager 失败"));
			break;
		}

		hs = OpenService(schSCManager, svcname, SERVICE_ALL_ACCESS);
		if (!hs) {
			LogErr(_T("OpenService 失败"));
			break;
		}

		SERVICE_STATUS status = {};
		if (!ControlService(hs, SERVICE_CONTROL_STOP, &status)) {
			LogErr(_T("ControlService stop 失败"));
			break;
		}

		Log(_T("Service 停止成功."));
		bRet = true;
	} while (false);

	if (hs) CloseServiceHandle(hs);
	if (schSCManager) CloseServiceHandle(schSCManager);
	return bRet;
}

bool DriverLoader::Unload(PCTSTR svcname)
{
	Log(_T("Unload: svcname=%s\n"), svcname ? svcname : _T("(null)"));
	if (svcname == nullptr) svcname = m_serviceName;
	SC_HANDLE schSCManager = NULL;
	SC_HANDLE hs = NULL;
	bool bRet = false;

	do {
		schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (!schSCManager) {
			LogErr(_T("OpenSCManager 失败"));
			break;
		}

		hs = OpenService(schSCManager, svcname, SERVICE_ALL_ACCESS);
		if (!hs) {
			LogErr(_T("OpenService 失败"));
			break;
		}

		if (!DeleteService(hs)) {
			LogErr(_T("UnloadService failed"));
			break;
		}

		Log(_T("Service 卸载成功."));
		bRet = true;
	} while (false);

	if (hs) CloseServiceHandle(hs);
	if (schSCManager) CloseServiceHandle(schSCManager);
	return bRet;
}

bool DriverLoader::Open(PCTSTR linkname)
{
	Log(_T("Open: linkname=%s\n"), linkname ? linkname : _T("(null)"));
	m_hDriver = CreateFile(
		linkname,
		GENERIC_ALL,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (m_hDriver != INVALID_HANDLE_VALUE)
		return TRUE;
	else
		LogErr(_T("打开驱动失败"));
		return FALSE;
}

void DriverLoader::Log(PCTSTR fmt, ...)
{
	TCHAR buf[512] = { 0 };
	va_list args;
	va_start(args, fmt);
	_vsntprintf(buf, sizeof(buf) / sizeof(TCHAR), fmt, args);
	va_end(args);

	OutputDebugString(buf);
	if (m_pObs) m_pObs->OnLog(buf);

}

void DriverLoader::LogErr(PCTSTR prefix)
{
	DWORD err = GetLastError();
	TCHAR msgBuf[512] = { 0 };
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		err,
		0,
		msgBuf,
		sizeof(msgBuf) / sizeof(TCHAR),
		NULL
	);

	Log(_T("%s (%lu): %s"), prefix, err, msgBuf);
}
