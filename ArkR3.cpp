#include "ArkR3.h"
#include <windows.h>



PSEGDESC ArkR3::GetSingeGDT(UINT cpuIndex, PGDTR pGdtr)
{
    DWORD gdtSize = pGdtr->Limit + 1;
    PSEGDESC pBuffer = (PSEGDESC)malloc(gdtSize);
    if (!pBuffer) {
        LogErr("GetSingeGDT malloc err");
        return nullptr;
    }

    GDT_DATA_REQ req = { 0 };
    req.CpuIndex = cpuIndex;
    req.Gdtr = *pGdtr;

    DWORD dwRetBytes = 0;
    DeviceIoControl(m_hDriver, CTL_GET_GDT_DATA, &req, sizeof(GDT_DATA_REQ),
        pBuffer, gdtSize, &dwRetBytes, NULL);

    return pBuffer;
}

std::vector<GDT_INFO> ArkR3::GetGDTVec()
{
    GDTVec_.clear();

    GDTR gdtr = { 0 };
    SYSTEM_INFO SystemInfo;
    GetSystemInfo(&SystemInfo);
    Log("GetGDTVec GDT dwNumberOfProcessors:%d\n", SystemInfo.dwNumberOfProcessors);

    for (UINT i = 0; i < SystemInfo.dwNumberOfProcessors; i++) {
        DWORD_PTR mask = 1UL << i;  //Mask按位 和 i 一致 
        SetProcessAffinityMask(GetCurrentProcess(), mask);
        __asm {
            sgdt gdtr
        }

        Log("GetGDTVec CPU %d: GDTR Base=%p, Limit=%X\n", i, (void*)gdtr.Base, gdtr.Limit);
        PSEGDESC pGdtData = GetSingeGDT(i, &gdtr);
        if (pGdtData) {
            DWORD descCount = (gdtr.Limit + 1) / 8;  // 段描述符数量
            Log("GetGDTVec CPU %d: 解析 %d 个段描述符\n", i, descCount);

            for (USHORT index = 0; index < descCount; index++) {
                //pDesc是段描述符指针 下面将原始数据转换成UI上显示的数据格式 
                PSEGDESC pDesc = (PSEGDESC)((PUCHAR)pGdtData + index * sizeof(SegmentDescriptor));

                // 解析成GDT_INFO
                GDT_INFO gdtInfo = { 0 };
                gdtInfo.cpuIndex = i;
                gdtInfo.selector = index * sizeof(SegmentDescriptor);

                // 基址重组 32bit = Base1(16) + Base2(8) + Base3(8)
                gdtInfo.base = pDesc->Base1 |(pDesc->Base2 << 16) |(pDesc->Base3 << 24);

                // 界限重组：Limit1(16) + Limit2(4)
                gdtInfo.limit = pDesc->Limit1 | (pDesc->Limit2 << 16);

                // 段粒度 
                gdtInfo.g = pDesc->g;
                if (gdtInfo.g) {
                    gdtInfo.limit = (gdtInfo.limit << 12) | 0xFFF;  // 低12bit置1 = 4K
                }

                gdtInfo.dpl = pDesc->dpl;           // 段特权级
                gdtInfo.type = pDesc->type;         // 段类型
                gdtInfo.system = pDesc->s;          // 系统段标志
                gdtInfo.p = pDesc->p;               // 存在位

                GDTVec_.emplace_back(gdtInfo);
                Log("GetGDTVec  [%02d] Sel:0x%04X Base:0x%08X Limit:0x%08X DPL:%d Type:0x%X %s\n",
                    index, gdtInfo.selector, (DWORD)gdtInfo.base, gdtInfo.limit,
                    gdtInfo.dpl, gdtInfo.type, gdtInfo.p ? "P" : "NP");
            }

            free(pGdtData);

        }
        else {
            Log("GetGDTVec CPU %d: pGdtData nullptr\n", i);
        }

    }

    Log("GetGDTVec成功获取 %zu 个段描述符信息\n", GDTVec_.size());
    return GDTVec_;
}

//PPROCESS_INFO ArkR3::GetProcessInfo(DWORD dwEntryNum)
//{
//    DWORD dwRetBytes;
//    DWORD dwBufferSize = sizeof(PROCESS_INFO) * dwEntryNum;
//    PPROCESS_INFO pEntryInfo = (PPROCESS_INFO)malloc(dwBufferSize);
//    DeviceIoControl(m_hDriver, CTL_ENUM_PROCESS, NULL, NULL, pEntryInfo, dwBufferSize, &dwRetBytes, NULL);
//    return pEntryInfo;
//}


// 获取进程信息并存储到数组中
std::vector<PROCESSENTRY32> ArkR3::EnumProcesses32() {
    HANDLE hSnapshot = INVALID_HANDLE_VALUE;
    PROCESSENTRY32 pe32;

    ProcVec_.clear();

    // 创建进程快照
    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        LogErr("CreateToolhelp32Snapshot失败\n");
        return ProcVec_;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnapshot, &pe32)) {
        Log("Process32First失败\n");
        CloseHandle(hSnapshot);
        return ProcVec_;
    }

    do {
        ProcVec_.push_back(pe32);
    } while (Process32Next(hSnapshot, &pe32));

    CloseHandle(hSnapshot);
    Log("获取到 %d 个进程\n", (int)ProcVec_.size());
    return ProcVec_;
}

//读取
//R3 : [PROCESS_MEM_REQ] → R0 → R0 : [读取数据] → R3
//发送12字节请求          接收Size字节
BOOL ArkR3::AttachReadMem(DWORD ProcessId, ULONG VirtualAddress, PVOID Buffer ,DWORD Size)
{
    //增加一个数据结构存储返回的数据 
    PVOID pBuffer = malloc(Size);
    if (!pBuffer) {
        LogErr("AttachReadMem err ");
        return FALSE;
    }

    PPROCESS_MEM_REQ req = (PPROCESS_MEM_REQ)pBuffer;
    req->ProcessId = (HANDLE)ProcessId;
    req->VirtualAddress = (PVOID)VirtualAddress;
    req->Size = Size;

    DWORD dwRetBytes = 0;
    BOOL bResult = DeviceIoControl(m_hDriver, CTL_ATTACH_MEM_READ, 
        pBuffer, sizeof(PROCESS_MEM_REQ),//发送请求头就够了
        pBuffer, Size,//接受的时候只接受读到的数据 也就是size大小 驱动返回的格式
        &dwRetBytes, NULL);

    if (bResult) {
        // 读到的数据写回
        memcpy(Buffer, pBuffer, Size);
        Log("AttachReadMem: PID=%d, Addr=0x%08X, Size=%d - Success\n", 
            ProcessId, VirtualAddress, Size);
        /*free(pBuffer);*/
        return TRUE;
    }
    else {
        LogErr("AttachReadMem !bResult");
        free(pBuffer);
        return FALSE;
    }
}

//写入：
//R3 : [PROCESS_MEM_REQ] [写入数据] → R0 → 处理完成
//发送12 + Size字节              不需要返回数据
BOOL ArkR3::AttachWriteMem(DWORD ProcessId, ULONG VirtualAddress, PVOID Buffer, DWORD Size)
{
    DWORD totalSize = sizeof(PROCESS_MEM_REQ) + Size;
    PVOID pBuffer = malloc(totalSize);//请求头大小+写入的数据大小

    if (!pBuffer) {
        LogErr("WriteProcessMemory: err");
        return FALSE;
    }

    PPROCESS_MEM_REQ req = (PPROCESS_MEM_REQ)pBuffer;
    req->ProcessId = (HANDLE)ProcessId;
    req->VirtualAddress = (PVOID)VirtualAddress;
    req->Size = Size;
    memcpy((PUCHAR)pBuffer + sizeof(PROCESS_MEM_REQ), Buffer, Size);//把要写入的数据考到缓冲区

    DWORD dwRetBytes = 0;
    BOOL bResult = DeviceIoControl(m_hDriver, CTL_ATTACH_MEM_WRITE,
        pBuffer, totalSize,           
        NULL,0,  // 输出：不接受信息了
        &dwRetBytes, NULL);

    if (bResult) {
        Log("AttachWriteMem成功: PID=%d, Addr=0x%08X, Size=%d\n",
            ProcessId, VirtualAddress, Size);
        free(pBuffer);
        return TRUE;
    }
    else {
        Log("AttachWriteMem失败: PID=%d, Addr=0x%08X, Size=%d (dwRetBytes=%d)\n",
            ProcessId, VirtualAddress, Size, dwRetBytes);
        free(pBuffer);
        return FALSE;
    }
}
