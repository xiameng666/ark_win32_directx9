#pragma once
#include "driverloader.h"

class ArkR3 :public DriverLoader
{
public:
    bool EnumProcess(PROCESS_INFO* out, int count = 60);

    HANDLE m_hDevice;
};

