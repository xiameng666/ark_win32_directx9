#pragma once
#include <windows.h>
#include <winsvc.h>
#include "../include/proto.h"

class ILogObserver {
public:
	virtual void OnLog(PCTSTR msg) = 0;
	virtual ~ILogObserver() {}
};

class DriverLoader
{
public:
	void SetPath(PCTSTR drivername, PCTSTR svcname);
	bool Load(PCTSTR drivername = nullptr, PCTSTR svcname = nullptr);
	bool Start(PCTSTR svcname = nullptr);
	bool Stop(PCTSTR svcname = nullptr);
	bool Unload(PCTSTR svcname = nullptr);
    bool Open(PCTSTR linkname);

	PCTSTR m_driverName;
	PCTSTR m_serviceName;
    HANDLE m_hDriver;

	void Log(PCTSTR fmt, ...);  //继承ILogObserver 重写OnLog 打印日志
	void LogErr(PCTSTR prefix);
	void SetLogObserver(ILogObserver* pObs) { m_pObs = pObs; }
	ILogObserver* m_pObs = nullptr;
};

