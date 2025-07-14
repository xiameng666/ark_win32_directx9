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
} *PGDT_INFO;

class ArkR3 :public DriverLoader
{
private://附加进程读写相关
    PVOID memBuffer_;
    DWORD memBufferSize_;
    DWORD memDataSize_;



public:
    ArkR3();
    ~ArkR3();

    PSEGDESC ArkR3::GDTGetSingle(UINT cpuIndex, PGDTR pGdtr);              //获得单核GDT表数据指针
    std::vector<GDT_INFO> GDTGetVec();                                     //返回所有核心GDT数组_gdtVec
    std::vector<GDT_INFO> GDTVec_;

    


    BOOL MemAttachRead(DWORD ProcessId, ULONG VirtualAddress, DWORD Size); //附加读
    BOOL MemAttachWrite(DWORD ProcessId, ULONG VirtualAddress, DWORD Size);//附加写
    BOOL MemEnsureBufferSize(DWORD requiredSize);                          //确保缓冲区大小
    void MemClearBuffer();                                                 //清空内存读写的缓冲区
    PVOID GetBufferData() const { return memBuffer_; }
    DWORD GetDataSize() const { return memDataSize_; }
    DWORD GetBufferSize() const { return memBufferSize_; }

    DWORD ProcessGetCount();                                                //获取进程数量
    std::vector<PROCESS_INFO> ProcessGetVec(DWORD processCount = 0);        //返回所有进程数据的数组ProcVec_
    std::vector<PROCESS_INFO> ProcVec_;
    //std::vector<PROCESSENTRY32> EnumProcesses32();                         //R3的枚举进程
};

