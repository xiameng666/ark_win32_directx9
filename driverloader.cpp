#include "driverloader.h"
#include <stdio.h>


void DriverLoader::SetPath(const char* drivername, const char* svcname)
{
	m_driverName = drivername;
	m_serviceName = svcname;
	return;
}

bool DriverLoader::Load(const char* drivername, const char* svcname)
{
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
		LogErr("OpenSCManager 失败");
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
				LogErr("OpenService 失败");
				CloseServiceHandle(schSCManager);
				return false;
			}
		}
		else {
			LogErr("CreateService 失败");
			CloseServiceHandle(schSCManager);
			return false;
		}
	}
	
	Log("Service 安装成功\n");

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
	return true;
}

bool DriverLoader::Start(const char* svcname)
{
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
			LogErr("OpenSCManager 失败");
			break;
		}

		hs = OpenService(schSCManager, svcname, SERVICE_ALL_ACCESS);
		if (hs == NULL) {
			LogErr("OpenService 失败");
			break;
		}

		if (!StartService(hs, 0, 0)) {
			LogErr("StartService 失败");
			break;
		}

		Log("Service start successful.");
		bRet = true;

	} while (false);

	if (hs) CloseServiceHandle(hs);
	if (schSCManager) CloseServiceHandle(schSCManager);
	return bRet;
}

bool DriverLoader::Stop(const char* svcname)
{
	if (svcname == nullptr) svcname = m_serviceName;
	SC_HANDLE schSCManager = NULL;
	SC_HANDLE hs = NULL;
	bool bRet = false;

	do {
		schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (!schSCManager) {
			LogErr("OpenSCManager 失败");
			break;
		}

		hs = OpenService(schSCManager, svcname, SERVICE_ALL_ACCESS);
		if (!hs) {
			LogErr("OpenService 失败");
			break;
		}

		SERVICE_STATUS status = {};
		if (!ControlService(hs, SERVICE_CONTROL_STOP, &status)) {
			LogErr("ControlService stop 失败");
			break;
		}

		Log("Service 停止成功.");
		bRet = true;
	} while (false);

	if (hs) CloseServiceHandle(hs);
	if (schSCManager) CloseServiceHandle(schSCManager);
	return bRet;
}

bool DriverLoader::Unload(const char* svcname)
{
	if (svcname == nullptr) svcname = m_serviceName;
	SC_HANDLE schSCManager = NULL;
	SC_HANDLE hs = NULL;
	bool bRet = false;

	do {
		schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (!schSCManager) {
			LogErr("OpenSCManager 失败");
			break;
		}

		hs = OpenService(schSCManager, svcname, SERVICE_ALL_ACCESS);
		if (!hs) {
			LogErr("OpenService 失败");
			break;
		}

		if (!DeleteService(hs)) {
			LogErr("UnloadService failed");
			break;
		}

		Log("Service 卸载成功.");
		bRet = true;
	} while (false);

	if (hs) CloseServiceHandle(hs);
	if (schSCManager) CloseServiceHandle(schSCManager);
	return bRet;
}

void DriverLoader::Log(const char* fmt, ...)
{
	char buf[512] = { 0 };
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	if (m_pObs) m_pObs->OnLog(buf);
}

void DriverLoader::LogErr(const char* prefix)
{
	DWORD err = GetLastError();
	char msgBuf[512] = { 0 };
	FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		err,
		0,
		msgBuf,
		sizeof(msgBuf),
		NULL
	);

	Log("%s (%lu): %s", prefix, err, msgBuf);
}
