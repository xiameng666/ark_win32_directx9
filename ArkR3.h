#pragma once
#include "driverloader.h"
#include <stdio.h>
#include"../include/proto.h"
#include <vector>
#include <memory>
#include <tlhelp32.h>
#include <psapi.h>
#include <string>
#include <vector>

#pragma comment(lib, "psapi.lib")
//单个段描述符信息在UI上展示的信息
typedef struct GDT_INFO {
    UINT    cpuIndex;       // CPU序号
    USHORT  selector;       // 段选择子（在GDT中的索引 * 8）
    ULONG64 base;           // 基址（Base1+Base2+Base3拼接）
    ULONG   limit;          // 界限（Limit1+Limit2拼接）
    UCHAR   g;              // 段粒度（0=字节，1=4KB）
    UCHAR   dpl;            // 段特权级（0=内核，3=用户）
    UCHAR   type;           // 段类型
    UCHAR   system;         // 系统段标志
    BOOL    p;              // 段存在位
} * PGDT_INFO;

class ArkR3 :public DriverLoader
{
private://附加进程读写相关
    PVOID memBuffer_;
    DWORD memBufferSize_;
    DWORD memDataSize_;

    BOOL EnsureBufferSize(DWORD requiredSize);

public:
    ArkR3();
    ~ArkR3();

    PSEGDESC ArkR3::GetSingeGDT(UINT cpuIndex, PGDTR pGdtr);               //获得单核GDT表数据指针
    std::vector<GDT_INFO> GetGDTVec();                                     //返回所有核心GDT数组_gdtVec
    std::vector<GDT_INFO> GDTVec_;

    std::vector<PROCESSENTRY32> EnumProcesses32();                         //R3的枚举进程
    std::vector<PROCESSENTRY32> ProcVec_; 

    BOOL AttachReadMem(DWORD ProcessId, ULONG VirtualAddress, DWORD Size); //附加读
    BOOL AttachWriteMem(DWORD ProcessId, ULONG VirtualAddress, DWORD Size);//附加写
    
    void ClearBuffer();                                                     //清空内存读写的缓冲区
    PVOID GetBufferData() const { return memBuffer_; }
    DWORD GetDataSize() const { return memDataSize_; }
    DWORD GetBufferSize() const { return memBufferSize_; }



    //PPROCESS_INFO GetProcessInfo(DWORD dwEntryNum);

};

