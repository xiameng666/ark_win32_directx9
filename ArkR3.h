#pragma once
#include "driverloader.h"
#include <stdio.h>
#include"../include/proto.h"

class ArkR3 :public DriverLoader
{
public:

    PPROCESS_INFO GetProcessInfo(DWORD dwEntryNum);
};

