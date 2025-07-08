#pragma once
#include "driverloader.h"
#include <stdio.h>

class ArkR3 :public DriverLoader
{
public:
    //ArkR3() {
    //    Open(DOS_NAME);
    //}

    //~ArkR3() {
    //    // 停止驱动
    //    Stop();
    //    // 卸载驱动
    //    Unload();
    //}



    PPROCESS_INFO GetProcessInfo(DWORD dwEntryNum);
};

