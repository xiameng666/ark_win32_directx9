#include "driverloader.h"
#include <stdio.h>


void DriverLoader::SetPath()
{
    char path[MAX_PATH] = { 0 };
    GetCurrentDirectoryA(MAX_PATH, path);
    strcat_s(path, SYS_REL_NAME_);
    strcpy_s(m_driverFullPath, path);
    Log("SetPath: m_driverFullPath=%s\n", m_driverFullPath);
}

bool DriverLoader::Load(const char* drivername, const char* svcname)
{
	if (drivername == nullptr) drivername = m_driverFullPath;
	if (svcname == nullptr) svcname = m_serviceName;
	Log("Load: drivername=%s, svcname=%s, \n",
		drivername, svcname);

	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	//char szPath[MAX_PATH];
	//GetCurrentDirectory(sizeof(szPath), szPath);
	//lstrcat(szPath, "\\");
	//lstrcat(szPath, drivername);

	schSCManager = OpenSCManager(
		NULL,                   
		NULL,                   
		SC_MANAGER_ALL_ACCESS   
	);

	if (NULL == schSCManager) {
		LogErr("OpenSCManager ");
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
	
	Log("Service created successfully.\n");

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
	return true;
}

bool DriverLoader::Start(const char* svcname)
{
	if (svcname == nullptr) svcname = m_serviceName;

	Log("Start  svcname=%s, \n", svcname);

	SC_HANDLE schSCManager = NULL;
	SC_HANDLE hs = NULL;
	bool bRet = false;

	do {
		schSCManager = OpenSCManager
		(
			NULL,                   
			NULL,                   
			SC_MANAGER_ALL_ACCESS   
		);

		if (schSCManager == NULL) {
			LogErr("OpenSCManager failed");
			break;
		}

		hs = OpenService(schSCManager, svcname, SERVICE_ALL_ACCESS);
		if (hs == NULL) {
			LogErr("OpenService failed");
			break;
		}

		if (!StartService(hs, 0, 0)) {
			LogErr("StartService failed");
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
			LogErr("OpenSCManager failed");
			break;
		}

		hs = OpenService(schSCManager, svcname, SERVICE_ALL_ACCESS);
		if (!hs) {
			LogErr("OpenService failed");
			break;
		}

		SERVICE_STATUS status = {};
		if (!ControlService(hs, SERVICE_CONTROL_STOP, &status)) {
			LogErr("ControlService stop failed");
			break;
		}

		Log("Service stopped successfully.\n");
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

    if (m_hDriver != INVALID_HANDLE_VALUE) {
        CloseHandle(m_hDriver);
        m_hDriver = INVALID_HANDLE_VALUE;
    }

	do {
		schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (!schSCManager) {
			LogErr("OpenSCManager failed");
			break;
		}

		hs = OpenService(schSCManager, svcname, SERVICE_ALL_ACCESS);
		if (!hs) {
			LogErr("OpenService failed");
			break;
		}

        SERVICE_STATUS status = {};
        if (!QueryServiceStatus(hs, &status)) {
            LogErr("QueryServiceStatus failed");
            break;
        }

        Log("Service current state: %d\n", status.dwCurrentState);

        // 如果服务正在运行，先停止它
        if (status.dwCurrentState != SERVICE_STOPPED) {
            Log("Stopping service...\n");

            if (!ControlService(hs, SERVICE_CONTROL_STOP, &status)) {
                DWORD error = GetLastError();
                if (error != ERROR_SERVICE_NOT_ACTIVE) {
                    LogErr("ControlService stop failed");
                  
                }
            }

            // 等待服务完全停止（最多等待10秒）
            for (int i = 0; i < 50; i++) {
                if (!QueryServiceStatus(hs, &status)) {
                    LogErr("QueryServiceStatus failed during wait");
                    break;
                }

                if (status.dwCurrentState == SERVICE_STOPPED) {
                    Log("Service stopped successfully.\n");
                    break;
                }

                if (status.dwCurrentState == SERVICE_STOP_PENDING) {
                    Log("Service stopping... (attempt %d/50)\n", i + 1);
                    Sleep(200); // 等待200ms
                    continue;
                }

                // 其他状态
                Log("Service state: %d, waiting...\n", status.dwCurrentState);
                Sleep(200);
            }

        }
		if (!DeleteService(hs)) {
			LogErr("UnloadService failed");
			break;
		}

		Log("Service unloaded successfully.\n");
		bRet = true;
	} while (false);

	if (hs) CloseServiceHandle(hs);
	if (schSCManager) CloseServiceHandle(schSCManager);

	return bRet;
}

bool DriverLoader::Open()
{
	Log("Open: linkname=%s\n",m_openName);
	m_hDriver = CreateFile(
        m_openName,
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
		LogErr("Failed to open driver");
	return FALSE;
}

void DriverLoader::Log(const char* fmt, ...)
{
	char buf[512] = { 0 };
	va_list args;
	va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	OutputDebugString(buf);
	if (m_pObs) m_pObs->OnLog(buf);
}

void DriverLoader::LogErr(const char* prefix)
{
	DWORD err = GetLastError();
	char msgBuf[512] = { 0 };
	FormatMessage(
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
