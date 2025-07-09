#pragma once
#include "driverloader.h"
#include <stdio.h>
#include"../include/proto.h"
#include <vector>
#include <memory>

//单个段描述符信息在UI上展示的信息
typedef struct _GDT_INFO {
    UINT    cpuIndex;       // CPU序号
    USHORT  selector;       // 段选择子（在GDT中的索引 * 8）
    ULONG64 base;           // 基址（Base1+Base2+Base3拼接）
    ULONG   limit;          // 界限（Limit1+Limit2拼接）
    UCHAR   g;              // 段粒度（0=字节，1=4KB）
    UCHAR   dpl;            // 段特权级（0=内核，3=用户）
    UCHAR   type;           // 段类型
    UCHAR   system;         // 系统段标志
    BOOL    p;              // 段存在位

    char    typeName[32];   // 段类型名称
} GDT_INFO, * PGDT_INFO;

class ArkR3 :public DriverLoader
{
public:
    PSEGDESC ArkR3::GetSingeGDT(UINT cpuIndex, PGDTR pGdtr);          //获得单核GDT表数据指针
    std::vector<GDT_INFO> GetGDTVec();                      //返回所有核心GDT数组_gdtVec
    PPROCESS_INFO GetProcessInfo(DWORD dwEntryNum);
    std::vector<GDT_INFO> _gdtVec;
};

