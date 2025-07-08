#pragma once
#include<Windows.h>
#include <winsvc.h>


#define DEVICE_NAME_   "\\Device\\NTDriver"
#define SYMBOL_NAME_  "\\DosDevices\\NTDriver"
#define DOS_NAME_     "\\\\.\\NTDriver"
#define SYS_REL_NAME_  "\\NTDriver.sys"
#define SVC_NAME_ "NTDriver"

class ILogObserver {
public:
	virtual void OnLog(const char* msg) = 0;
	virtual ~ILogObserver() {}
};

class DriverLoader
{
public:
    void SetPath();
	bool Load(const char* drivername = nullptr, const char* svcname = nullptr);
	bool Start(const char* svcname = nullptr);
	bool Stop(const char* svcname = nullptr);
	bool Unload(const char* svcname = nullptr);

    bool Open();

    char m_driverFullPath[MAX_PATH] = { 0 };
    const char* m_serviceName = SVC_NAME_;
    const char* m_dosName = DOS_NAME_;
    const char* m_openName = DOS_NAME_;
    HANDLE m_hDriver;

	void Log(const char* fmt, ...);
	void LogErr(const char* prefix);
	void SetLogObserver(ILogObserver* pObs) { m_pObs = pObs; }
	ILogObserver* m_pObs = nullptr;
};

