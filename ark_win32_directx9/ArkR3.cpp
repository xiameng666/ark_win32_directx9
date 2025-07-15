#include "ArkR3.h"
#include <windows.h>

// 构造函数
ArkR3::ArkR3() : memBuffer_(nullptr), memBufferSize_(0), memDataSize_(0)
{
    // 初始化时分配一个基础大小的缓冲区
    MemEnsureBufferSize(4096); // 初始4KB
}

// 析构函数
ArkR3::~ArkR3()
{
    if (memBuffer_) {
        free(memBuffer_);
        memBuffer_ = nullptr;
    }
    memBufferSize_ = 0;
    memDataSize_ = 0;
}

// 确保缓冲区大小足够
BOOL ArkR3::MemEnsureBufferSize(DWORD requiredSize)
{
    if (requiredSize > 0x100000) { // 限制最大1MB
        Log("EnsureBufferSize: Size too large (%d bytes)", requiredSize);
        return FALSE;
    }

    if (memBufferSize_ >= requiredSize) {
        return TRUE; // 当前缓冲区已足够
    }

    // 计算新的缓冲区大小（向上取整到4KB边界）
    DWORD newSize = ((requiredSize + 4095) / 4096) * 4096;
    
    PVOID newBuffer = realloc(memBuffer_, newSize);
    if (!newBuffer) {
        Log("EnsureBufferSize: Failed to allocate %d bytes", newSize);
        return FALSE;
    }

    memBuffer_ = newBuffer;
    memBufferSize_ = newSize;
    
    Log("EnsureBufferSize: Buffer resized to %d bytes", newSize);
    return TRUE;
}

void ArkR3::MemClearBuffer()
{
    if (memBuffer_ && memBufferSize_ > 0) {
        memset(memBuffer_, 0, memBufferSize_);
    }
    memDataSize_ = 0;
}


PSEGDESC ArkR3::GDTGetSingle(UINT cpuIndex, PGDTR pGdtr)
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

std::vector<GDT_INFO> ArkR3::GDTGetVec()
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
        PSEGDESC pGdtData = GDTGetSingle(i, &gdtr);
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
                gdtInfo.base = pDesc->Base1 | (pDesc->Base2 << 16) | (pDesc->Base3 << 24);

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


DWORD ArkR3::ProcessGetCount()
{
    DWORD dwBytes;
    DWORD dwEntryNum = NULL;

    DeviceIoControl(m_hDriver, CTL_ENUM_PROCESS_COUNT, NULL, NULL, &dwEntryNum, sizeof(DWORD), &dwBytes, NULL);

    return dwEntryNum;
}


std::vector<PROCESS_INFO> ArkR3::ProcessGetVec(DWORD processCount)
{
    DWORD dwRetBytes;
    DWORD dwBufferSize = sizeof(PROCESS_INFO) * processCount;
    PPROCESS_INFO pEntryInfo = (PPROCESS_INFO)malloc(dwBufferSize);
    BOOL bResult = DeviceIoControl(m_hDriver, CTL_ENUM_PROCESS, NULL, NULL, pEntryInfo, dwBufferSize, &dwRetBytes, NULL);

    ProcVec_.clear();
    
    DWORD Count = 0;
    if (bResult) {
        Count = dwRetBytes / sizeof(PROCESS_INFO);
        for (DWORD i = 0; i < Count; i++) {
            PROCESS_INFO pInfo = pEntryInfo[i];     
            ProcVec_.emplace_back(pInfo);           

            Log("ProcessGetVec 进程[%d]: PID=%d, 父PID=%d, 名称=%s, EPROCESS=%p\n",
                i, pInfo.ProcessId, pInfo.ParentProcessId,
                pInfo.ImageFileName, pInfo.EprocessAddr);
        }
    }
    
    free(pEntryInfo);

    return ProcVec_;
}

//// 获取进程信息并存储到数组中  
//std::vector<PROCESSENTRY32> ArkR3::EnumProcesses32() {
//    HANDLE hSnapshot = INVALID_HANDLE_VALUE;
//    PROCESSENTRY32 pe32;
//
//    ProcVec_.clear();
//
//    // 创建进程快照
//    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
//    if (hSnapshot == INVALID_HANDLE_VALUE) {
//        LogErr("CreateToolhelp32Snapshot失败\n");
//        return ProcVec_;
//    }
//
//    pe32.dwSize = sizeof(PROCESSENTRY32);
//
//    if (!Process32First(hSnapshot, &pe32)) {
//        Log("Process32First失败\n");
//        CloseHandle(hSnapshot);
//        return ProcVec_;
//    }
//
//    do {
//        ProcVec_.push_back(pe32);
//    } while (Process32Next(hSnapshot, &pe32));
//
//    CloseHandle(hSnapshot);
//    Log("获取到 %d 个进程\n", (int)ProcVec_.size());
//    return ProcVec_;
//}

//读取
//R3 : [PROCESS_MEM_REQ] → R0 → R0 : [读取数据] → R3
//发送12字节请求          接收Size字节
BOOL ArkR3::MemAttachRead(DWORD ProcessId, ULONG VirtualAddress, DWORD Size)
{
    // 参数验证
    if (ProcessId == 0 || Size == 0) {
        LogErr("AttachReadMem: Invalid args");
        return FALSE;
    }

    // 确保内部缓冲区足够大
    if (!MemEnsureBufferSize(Size)) {
        LogErr("AttachReadMem: Failed to ensure buffer size");
        return FALSE;
    }

    // 构造请求结构体（栈上分配即可）
    PROCESS_MEM_REQ req;
    req.ProcessId = (HANDLE)ProcessId;
    req.VirtualAddress = (PVOID)VirtualAddress;
    req.Size = Size;

    DWORD dwRetBytes = 0;
    BOOL bResult = DeviceIoControl(m_hDriver, CTL_ATTACH_MEM_READ,
        &req, sizeof(PROCESS_MEM_REQ),           // 输入：请求结构体
        memBuffer_, Size,                        // 输出：直接写入内部缓冲区
        &dwRetBytes, NULL);

    if (bResult) {
        memDataSize_ = Size;  // 无需额外拷贝！

        Log("AttachReadMem: PID=%d, Addr=0x%08X, Size=%d - Success",
            ProcessId, VirtualAddress, Size);

        return TRUE;
    }
    else {
        Log("AttachReadMem: DeviceIoControl failed, PID=%d, Addr=0x%08X, Size=%d",
            ProcessId, VirtualAddress, Size);
        return FALSE;
    }
}

//写入：
//R3 : [PROCESS_MEM_REQ] [写入数据] → R0 → 处理完成
//发送12 + Size字节              不需要返回数据
BOOL ArkR3::MemAttachWrite(DWORD ProcessId, ULONG VirtualAddress, DWORD Size)
{
    // 参数验证
    if (ProcessId == 0 || VirtualAddress == 0 || Size == 0) {
        Log("AttachWriteMem: Invalid parameters");
        return FALSE;
    }

    // 检查内部缓冲区是否有足够的数据
    if (memDataSize_ < Size) {
        Log("AttachWriteMem: Not enough data in buffer, available: %d, required: %d", 
               memDataSize_, Size);
        return FALSE;
    }

    DWORD totalSize = sizeof(PROCESS_MEM_REQ) + Size;
    PVOID pBuffer = malloc(totalSize);

    if (!pBuffer) {
        Log("AttachWriteMem: Failed to allocate buffer, size: %d", totalSize);
        return FALSE;
    }

    // 构造请求头
    PPROCESS_MEM_REQ req = (PPROCESS_MEM_REQ)pBuffer;
    req->ProcessId = (HANDLE)ProcessId;
    req->VirtualAddress = (PVOID)VirtualAddress;
    req->Size = Size;
    
    // 从内部缓冲区复制数据到请求缓冲区
    memcpy((PUCHAR)pBuffer + sizeof(PROCESS_MEM_REQ), memBuffer_, Size);

    DWORD dwRetBytes = 0;
    BOOL bResult = DeviceIoControl(m_hDriver, CTL_ATTACH_MEM_WRITE,
        pBuffer, totalSize,           
        NULL, 0,
        &dwRetBytes, NULL);

    if (bResult) {
        Log("AttachWriteMem: PID=%d, Addr=0x%08X, Size=%d - Success",
            ProcessId, VirtualAddress, Size);
    } else {
        Log("AttachWriteMem: PID=%d, Addr=0x%08X, Size=%d (dwRetBytes=%d)",
            ProcessId, VirtualAddress, Size, dwRetBytes);
    }

    free(pBuffer);
    return bResult;
}

// 获取内核模块数量
DWORD ArkR3::ModuleGetCount()
{
    DWORD dwBytes = 0;
    DWORD dwEntryNum = 0;

    DeviceIoControl(m_hDriver, CTL_ENUM_MODULE_COUNT, NULL, NULL, &dwEntryNum, sizeof(DWORD), &dwBytes, NULL);

    return dwEntryNum;
}

//获取内核模块信息
std::vector<MODULE_INFO> ArkR3::ModuleGetVec(DWORD moduleCount)
{
    DWORD dwRetBytes;
    DWORD dwBufferSize = sizeof(MODULE_INFO) * moduleCount;
    PMODULE_INFO pEntryInfo = (PMODULE_INFO)malloc(dwBufferSize);
    BOOL bResult = DeviceIoControl(m_hDriver, CTL_ENUM_MODULE, NULL, NULL, pEntryInfo, dwBufferSize, &dwRetBytes, NULL);

    MoudleVec_.clear();
    
    DWORD Count = 0;
    if (bResult) {
        Count = dwRetBytes / sizeof(MODULE_INFO);
        for (DWORD i = 0; i < Count; i++) {
            MODULE_INFO mInfo = pEntryInfo[i];     
            MoudleVec_.emplace_back(mInfo);           

            Log("ModuleGetVec 模块[%d]: 名称=%s, 基地址=%p, 大小=0x%X, 路径=%s\n",
                i, mInfo.Name, mInfo.ImageBase, mInfo.ImageSize, mInfo.FullPath);
        }
    }

    free(pEntryInfo);
    return MoudleVec_;
}

// 获取指定进程的模块数量
DWORD ArkR3::ProcessModuleGetCount(DWORD processId)
{
    DWORD dwBytes = 0;
    DWORD dwEntryNum = 0;
    
    PROCESS_MODULE_REQ req;
    req.ProcessId = (HANDLE)processId;
    req.ModuleCount = 0;

    DeviceIoControl(m_hDriver, CTL_ENUM_PROCESS_MODULE_COUNT, &req, sizeof(req), 
                   &dwEntryNum, sizeof(DWORD), &dwBytes, NULL);

    return dwEntryNum;
}

// 获取指定进程的模块信息
std::vector<MODULE_INFO> ArkR3::ProcessModuleGetVec(DWORD processId, DWORD moduleCount)
{
    DWORD dwRetBytes;
    DWORD dwBufferSize = sizeof(MODULE_INFO) * moduleCount;
    PMODULE_INFO pEntryInfo = (PMODULE_INFO)malloc(dwBufferSize);
    
    PROCESS_MODULE_REQ req;
    req.ProcessId = (HANDLE)processId;
    req.ModuleCount = moduleCount;
    
    // 将请求结构复制到缓冲区开头
    memcpy(pEntryInfo, &req, sizeof(req));
    
    BOOL bResult = DeviceIoControl(m_hDriver, CTL_ENUM_PROCESS_MODULE, 
                                  pEntryInfo, dwBufferSize, 
                                  pEntryInfo, dwBufferSize, &dwRetBytes, NULL);

    ProcessModuleVec_.clear();
    
    DWORD Count = 0;
    if (bResult) {
        Count = dwRetBytes / sizeof(MODULE_INFO);
        for (DWORD i = 0; i < Count; i++) {
            MODULE_INFO mInfo = pEntryInfo[i];     
            ProcessModuleVec_.emplace_back(mInfo);           

            Log("ProcessModuleGetVec 进程[%d]模块[%d]: 名称=%s, 基地址=%p, 大小=0x%X\n",
                processId, i, mInfo.Name, mInfo.ImageBase, mInfo.ImageSize);
        }
    }
    
    free(pEntryInfo);

    return ProcessModuleVec_;
}
