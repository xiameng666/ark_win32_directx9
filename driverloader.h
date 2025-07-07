#pragma once
#include <windows.h>
#include <winsvc.h>
#include "../include/proto.h"

class ILogObserver {
public:
	virtual void OnLog(const char* msg) = 0;
	virtual ~ILogObserver() {}
};

class DriverLoader
{
public:
	void SetPath(const char* drivername, const char* svcname);
	bool Load(const char* drivername = nullptr, const char* svcname = nullptr);
	bool Start(const char* svcname = nullptr);
	bool Stop(const char* svcname = nullptr);
	bool Unload(const char* svcname = nullptr);

	const char* m_driverName;
	const char* m_serviceName;

	void Log(const char* fmt, ...);
	void LogErr(const char* prefix);
	void SetLogObserver(ILogObserver* pObs) { m_pObs = pObs; }
	ILogObserver* m_pObs = nullptr;
};

