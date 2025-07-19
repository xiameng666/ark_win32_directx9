#include "ArkR3.h"
#include <windows.h>
SSDT_INFO g_SSDT_XP_SP3_Table[];

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

                //系统段解析
                const char* segmentType = " ";
                if (gdtInfo.system == 0) {          // 系统段
                    switch (gdtInfo.type) {
                    case 1: segmentType = "16-bit TSS (Available)"; break;
                    case 2: segmentType = "LDT"; break;
                    case 3: segmentType = "16-bit TSS (Busy)"; break;
                    case 4: segmentType = "16-bit Call Gate"; break;
                    case 5: segmentType = "Task Gate"; break;
                    case 6: segmentType = "16-bit Interrupt Gate"; break;
                    case 7: segmentType = "16-bit Trap Gate"; break;
                    case 9: segmentType = "32-bit TSS (Available)"; break;
                    case 11: segmentType = "32-bit TSS (Busy)"; break;
                    case 12: segmentType = "32-bit Call Gate"; break;
                    case 14: segmentType = "32-bit Interrupt Gate"; break;
                    case 15: segmentType = "32-bit Trap Gate"; break;
                    }
                }
                else {
                    if (gdtInfo.type & 8) {  // 代码段
                        segmentType = (gdtInfo.type & 2) ? "Code (R E)" : "Code (E)";
                    }
                    else {  // 数据段
                        segmentType = (gdtInfo.type & 2) ? "Data (R W E)" : "Data (R E)";
                    }
                }
                strcpy_s(gdtInfo.typeDesc, sizeof(gdtInfo.typeDesc), segmentType);

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

            // 处理完整路径
            std::string fullPath = mInfo.FullPath;
            if (fullPath.find("\\SystemRoot\\") == 0) {
                fullPath = "C:\\Windows\\" + fullPath.substr(12);
            }
            else if (fullPath.find("\\WINDOWS\\") == 0) {
                fullPath = "C:\\Windows\\" + fullPath.substr(9);
            }
            else if (fullPath.find("\\??\\C:") == 0) {
                fullPath = "C:" + fullPath.substr(6);
            }
            strcpy_s(mInfo.FullPath, sizeof(mInfo.FullPath), fullPath.c_str());

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

// 获取SSDT信息
std::vector<SSDT_INFO> ArkR3::SSDTGetVec()
{
    // 分配缓冲区，假设最大500个SSDT项
    int  nMaxCount = 500;
    DWORD bufferSize = sizeof(SSDT_INFO) * nMaxCount;
    PSSDT_INFO pSsdtInfo = (PSSDT_INFO)malloc(bufferSize);

    if (!pSsdtInfo) {
        Log("SSDTGetVec: malloc err\n");
        return SSDTVec_;
    }

    DWORD dwRetBytes = 0;
    BOOL bResult = DeviceIoControl(
        m_hDriver,
        CTL_ENUM_SSDT,
        NULL, 0,
        pSsdtInfo, bufferSize,
        &dwRetBytes,
        NULL
    );

    if (bResult && dwRetBytes > 0) {
        DWORD count = dwRetBytes / sizeof(SSDT_INFO);
        Log("SSDTGetVec: 成功获取%d个SSDT项\n", count);

        for (DWORD i = 0; i < count; i++) {
            if (pSsdtInfo[i].FunctionAddress == g_SSDT_XP_SP3_Table[i].FunctionAddress) {
                strcpy_s(pSsdtInfo[i].FunctionName, sizeof(pSsdtInfo[i].FunctionName),
                    g_SSDT_XP_SP3_Table[i].FunctionName);
            }
                  
            SSDTVec_.emplace_back(pSsdtInfo[i]);

            Log("SSDT[%03d]: %s -> 0x%p\n",
                pSsdtInfo[i].Index,
                pSsdtInfo[i].FunctionName,
                pSsdtInfo[i].FunctionAddress);
        }
    }
    else {
        LogErr("SSDTGetVec: DeviceIoControl err");
    }

    free(pSsdtInfo);
    return SSDTVec_;
}


SSDT_INFO g_SSDT_XP_SP3_Table[] = {
    {0, (PVOID)0x805a5614, "NtAcceptConnectPort"},
    {1, (PVOID)0x805f1adc, "NtAccessCheck"},
    {2, (PVOID)0x805f5312, "NtAccessCheckAndAuditAlarm"},
    {3, (PVOID)0x805f1b0e, "NtAccessCheckByType"},
    {4, (PVOID)0x805f534c, "NtAccessCheckByTypeAndAuditAlarm"},
    {5, (PVOID)0x805f1b44, "NtAccessCheckByTypeResultList"},
    {6, (PVOID)0x805f5390, "NtAccessCheckByTypeResultListAndAuditAlarm"},
    {7, (PVOID)0x805f53d4, "NtAccessCheckByTypeResultListAndAuditAlarmByHandle"},
    {8, (PVOID)0x806163a2, "NtAddAtom"},
    {9, (PVOID)0x806170e4, "NtAddBootEntry"},
    {10, (PVOID)0x805eceda, "NtAdjustGroupsToken"},
    {11, (PVOID)0x805ecb32, "NtAdjustPrivilegesToken"},
    {12, (PVOID)0x805d5b3a, "NtAlertResumeThread"},
    {13, (PVOID)0x805d5aea, "NtAlertThread"},
    {14, (PVOID)0x806169c8, "NtAllocateLocallyUniqueId"},
    {15, (PVOID)0x805b6f72, "NtAllocateUserPhysicalPages"},
    {16, (PVOID)0x80615fe4, "NtAllocateUuids"},
    {17, (PVOID)0x805a9a9e, "NtAllocateVirtualMemory"},
    {18, (PVOID)0x805b1596, "NtAreMappedFilesTheSame"},
    {19, (PVOID)0x805d75fe, "NtAssignProcessToJobObject"},
    {20, (PVOID)0x8050288c, "NtCallbackReturn"},
    {21, (PVOID)0x806170d6, "NtModifyBootEntry"},
    {22, (PVOID)0x80577ae6, "NtCancelIoFile"},
    {23, (PVOID)0x80539bd2, "NtCancelTimer"},
    {24, (PVOID)0x8060f5b2, "NtClearEvent"},
    {25, (PVOID)0x805bd4ec, "NtClose"},
    {26, (PVOID)0x805f584c, "NtCloseObjectAuditAlarm"},
    {27, (PVOID)0x80624356, "NtCompactKeys"},
    {28, (PVOID)0x805f9d3e, "NtCompareTokens"},
    {29, (PVOID)0x805a5d02, "NtCompleteConnectPort"},
    {30, (PVOID)0x806245aa, "NtCompressKey"},
    {31, (PVOID)0x805a55b4, "NtConnectPort"},
    {32, (PVOID)0x80545e6c, "NtContinue"},
    {33, (PVOID)0x80642e7c, "NtCreateDebugObject"},
    {34, (PVOID)0x805bf49c, "NtCreateDirectoryObject"},
    {35, (PVOID)0x8060f602, "NtCreateEvent"},
    {36, (PVOID)0x8061795a, "NtCreateEventPair"},
    {37, (PVOID)0x8057a084, "NtCreateFile"},
    {38, (PVOID)0x80579a62, "NtCreateIoCompletion"},
    {39, (PVOID)0x805d65c2, "NtCreateJobObject"},
    {40, (PVOID)0x805d62fa, "NtCreateJobSet"},
    {41, (PVOID)0x80624786, "NtCreateKey"},
    {42, (PVOID)0x8057a192, "NtCreateMailslotFile"},
    {43, (PVOID)0x80617d52, "NtCreateMutant"},
    {44, (PVOID)0x8057a0be, "NtCreateNamedPipeFile"},
    {45, (PVOID)0x805ac9d4, "NtCreatePagingFile"},
    {46, (PVOID)0x805a60d0, "NtCreatePort"},
    {47, (PVOID)0x805d21ec, "NtCreateProcess"},
    {48, (PVOID)0x805d2136, "NtCreateProcessEx"},
    {49, (PVOID)0x80618172, "NtCreateProfile"},
    {50, (PVOID)0x805ac3ae, "NtCreateSection"},
    {51, (PVOID)0x80615702, "NtCreateSemaphore"},
    {52, (PVOID)0x805c49b6, "NtCreateSymbolicLinkObject"},
    {53, (PVOID)0x805d1fd4, "NtCreateThread"},
    {54, (PVOID)0x80617622, "NtCreateTimer"},
    {55, (PVOID)0x805fa0e6, "NtCreateToken"},
    {56, (PVOID)0x805a60f4, "NtCreateWaitablePort"},
    {57, (PVOID)0x80643f58, "NtDebugActiveProcess"},
    {58, (PVOID)0x806440a8, "NtDebugContinue"},
    {59, (PVOID)0x80617026, "NtDelayExecution"},
    {60, (PVOID)0x80616858, "NtDeleteAtom"},
    {61, (PVOID)0x806170d6, "NtModifyBootEntry"},
    {62, (PVOID)0x80577c2c, "NtDeleteFile"},
    {63, (PVOID)0x80624c16, "NtDeleteKey"},
    {64, (PVOID)0x805f5958, "NtDeleteObjectAuditAlarm"},
    {65, (PVOID)0x80624de6, "NtDeleteValueKey"},
    {66, (PVOID)0x8057a24a, "NtDeviceIoControlFile"},
    {67, (PVOID)0x80613680, "NtDisplayString"},
    {68, (PVOID)0x805befc4, "NtDuplicateObject"},
    {69, (PVOID)0x805edd88, "NtDuplicateToken"},
    {70, (PVOID)0x806170e4, "NtAddBootEntry"},
    {71, (PVOID)0x80624fc6, "NtEnumerateKey"},
    {72, (PVOID)0x806170c8, "NtEnumerateSystemEnvironmentValuesEx"},
    {73, (PVOID)0x80625230, "NtEnumerateValueKey"},
    {74, (PVOID)0x805b4c9e, "NtExtendSection"},
    {75, (PVOID)0x805edf34, "NtFilterToken"},
    {76, (PVOID)0x8061660c, "NtFindAtom"},
    {77, (PVOID)0x80577cf8, "NtFlushBuffersFile"},
    {78, (PVOID)0x805b7806, "NtFlushInstructionCache"},
    {79, (PVOID)0x8062549a, "NtFlushKey"},
    {80, (PVOID)0x805ad6e8, "NtFlushVirtualMemory"},
    {81, (PVOID)0x805b77a8, "NtFlushWriteBuffer"},
    {82, (PVOID)0x805b7314, "NtFreeUserPhysicalPages"},
    {83, (PVOID)0x805b3f7e, "NtFreeVirtualMemory"},
    {84, (PVOID)0x8057a27e, "NtFsControlFile"},
    {85, (PVOID)0x805d24e6, "NtGetContextThread"},
    {86, (PVOID)0x805c9640, "NtGetDevicePowerState"},
    {87, (PVOID)0x8059a116, "NtGetPlugPlayEvent"},
    {88, (PVOID)0x8052217a, "NtGetWriteWatch"},
    {89, (PVOID)0x805f9a32, "NtImpersonateAnonymousToken"},
    {90, (PVOID)0x805a615e, "NtImpersonateClientOfPort"},
    {91, (PVOID)0x805d87be, "NtImpersonateThread"},
    {92, (PVOID)0x806228dc, "NtInitializeRegistry"},
    {93, (PVOID)0x805c9426, "NtInitiatePowerAction"},
    {94, (PVOID)0x805d61be, "NtIsProcessInJob"},
    {95, (PVOID)0x805c962c, "NtIsSystemResumeAutomatic"},
    {96, (PVOID)0x805a636a, "NtListenPort"},
    {97, (PVOID)0x8058513a, "NtLoadDriver"},
    {98, (PVOID)0x80626982, "NtLoadKey"},
    {99, (PVOID)0x8062658e, "NtLoadKey2"},
    {100, (PVOID)0x8057a2b2, "NtLockFile"},
    {101, (PVOID)0x80613c72, "NtLockProductActivationKeys"},
    {102, (PVOID)0x80624656, "NtLockRegistryKey"},
    {103, (PVOID)0x805b790e, "NtLockVirtualMemory"},
    {104, (PVOID)0x805bf292, "NtMakePermanentObject"},
    {105, (PVOID)0x805bd590, "NtMakeTemporaryObject"},
    {106, (PVOID)0x805b63d2, "NtMapUserPhysicalPages"},
    {107, (PVOID)0x805b6922, "NtMapUserPhysicalPagesScatter"},
    {108, (PVOID)0x805b3006, "NtMapViewOfSection"},
    {109, (PVOID)0x806170d6, "NtModifyBootEntry"},
    {110, (PVOID)0x8057aeca, "NtNotifyChangeDirectoryFile"},
    {111, (PVOID)0x8062694c, "NtNotifyChangeKey"},
    {112, (PVOID)0x8062559c, "NtNotifyChangeMultipleKeys"},
    {113, (PVOID)0x805bf56e, "NtOpenDirectoryObject"},
    {114, (PVOID)0x8060f702, "NtOpenEvent"},
    {115, (PVOID)0x80617a32, "NtOpenEventPair"},
    {116, (PVOID)0x8057b182, "NtOpenFile"},
    {117, (PVOID)0x80579b3a, "NtOpenIoCompletion"},
    {118, (PVOID)0x805d6748, "NtOpenJobObject"},
    {119, (PVOID)0x80625b58, "NtOpenKey"},
    {120, (PVOID)0x80617e2a, "NtOpenMutant"},
    {121, (PVOID)0x805f541a, "NtOpenObjectAuditAlarm"},
    {122, (PVOID)0x805cc3fc, "NtOpenProcess"},
    {123, (PVOID)0x805ee722, "NtOpenProcessToken"},
    {124, (PVOID)0x805ee386, "NtOpenProcessTokenEx"},
    {125, (PVOID)0x805ab3d2, "NtOpenSection"},
    {126, (PVOID)0x806157fc, "NtOpenSemaphore"},
    {127, (PVOID)0x805c4b9c, "NtOpenSymbolicLinkObject"},
    {128, (PVOID)0x805cc688, "NtOpenThread"},
    {129, (PVOID)0x805ee740, "NtOpenThreadToken"},
    {130, (PVOID)0x805ee4f6, "NtOpenThreadTokenEx"},
    {131, (PVOID)0x80617744, "NtOpenTimer"},
    {132, (PVOID)0x8064614a, "NtPlugPlayControl"},
    {133, (PVOID)0x805ca4ae, "NtPowerInformation"},
    {134, (PVOID)0x805f8ae4, "NtPrivilegeCheck"},
    {135, (PVOID)0x805f472c, "NtPrivilegeObjectAuditAlarm"},
    {136, (PVOID)0x805f4918, "NtPrivilegedServiceAuditAlarm"},
    {137, (PVOID)0x805b93da, "NtProtectVirtualMemory"},
    {138, (PVOID)0x8060f7ba, "NtPulseEvent"},
    {139, (PVOID)0x80577ed6, "NtQueryAttributesFile"},
    {140, (PVOID)0x806170e4, "NtAddBootEntry"},
    {141, (PVOID)0x806170e4, "NtAddBootEntry"},
    {142, (PVOID)0x80540bb6, "NtQueryDebugFilterState"},
    {143, (PVOID)0x806113ac, "NtQueryDefaultLocale"},
    {144, (PVOID)0x8061200c, "NtQueryDefaultUILanguage"},
    {145, (PVOID)0x8057ae64, "NtQueryDirectoryFile"},
    {146, (PVOID)0x805bf60e, "NtQueryDirectoryObject"},
    {147, (PVOID)0x8057b1b2, "NtQueryEaFile"},
    {148, (PVOID)0x8060f882, "NtQueryEvent"},
    {149, (PVOID)0x8057802a, "NtQueryFullAttributesFile"},
    {150, (PVOID)0x80616880, "NtQueryInformationAtom"},
    {151, (PVOID)0x8057ba1e, "NtQueryInformationFile"},
    {152, (PVOID)0x805d6c1a, "NtQueryInformationJobObject"},
    {153, (PVOID)0x805a63c8, "NtQueryInformationPort"},
    {154, (PVOID)0x805cdf50, "NtQueryInformationProcess"},
    {155, (PVOID)0x805ccb7e, "NtQueryInformationThread"},
    {156, (PVOID)0x805ee820, "NtQueryInformationToken"},
    {157, (PVOID)0x806117aa, "NtQueryInstallUILanguage"},
    {158, (PVOID)0x806185f4, "NtQueryIntervalProfile"},
    {159, (PVOID)0x80579be2, "NtQueryIoCompletion"},
    {160, (PVOID)0x80625e7e, "NtQueryKey"},
    {161, (PVOID)0x806238d4, "NtQueryMultipleValueKey"},
    {162, (PVOID)0x80617ed2, "NtQueryMutant"},
    {163, (PVOID)0x805c6288, "NtQueryObject"},
    {164, (PVOID)0x80623f80, "NtQueryOpenSubKeys"},
    {165, (PVOID)0x80618682, "NtQueryPerformanceCounter"},
    {166, (PVOID)0x8057c800, "NtQueryQuotaInformationFile"},
    {167, (PVOID)0x805b959c, "NtQuerySection"},
    {168, (PVOID)0x805c1056, "NtQuerySecurityObject"},
    {169, (PVOID)0x806158b4, "NtQuerySemaphore"},
    {170, (PVOID)0x805c4c3c, "NtQuerySymbolicLinkObject"},
    {171, (PVOID)0x80617100, "NtQuerySystemEnvironmentValue"},
    {172, (PVOID)0x806170ba, "NtSetSystemEnvironmentValueEx"},
    {173, (PVOID)0x8061208c, "NtQuerySystemInformation"},
    {174, (PVOID)0x8061384c, "NtQuerySystemTime"},
    {175, (PVOID)0x806177fc, "NtQueryTimer"},
    {176, (PVOID)0x806138de, "NtQueryTimerResolution"},
    {177, (PVOID)0x806229be, "NtQueryValueKey"},
    {178, (PVOID)0x805b9c2a, "NtQueryVirtualMemory"},
    {179, (PVOID)0x8057ccea, "NtQueryVolumeInformationFile"},
    {180, (PVOID)0x805d2232, "NtQueueApcThread"},
    {181, (PVOID)0x80545eb4, "NtRaiseException"},
    {182, (PVOID)0x80615526, "NtRaiseHardError"},
    {183, (PVOID)0x8057d48a, "NtReadFile"},
    {184, (PVOID)0x8057d9f4, "NtReadFileScatter"},
    {185, (PVOID)0x805a6e50, "NtReadRequestData"},
    {186, (PVOID)0x805b528a, "NtReadVirtualMemory"},
    {187, (PVOID)0x805d3754, "NtRegisterThreadTerminatePort"},
    {188, (PVOID)0x8061800a, "NtReleaseMutant"},
    {189, (PVOID)0x806159e4, "NtReleaseSemaphore"},
    {190, (PVOID)0x80579eda, "NtRemoveIoCompletion"},
    {191, (PVOID)0x80644028, "NtRemoveProcessDebug"},
    {192, (PVOID)0x806241a8, "NtRenameKey"},
    {193, (PVOID)0x80626832, "NtReplaceKey"},
    {194, (PVOID)0x805a64d0, "NtReplyPort"},
    {195, (PVOID)0x805a7498, "NtReplyWaitReceivePort"},
    {196, (PVOID)0x805a6ea0, "NtReplyWaitReceivePortEx"},
    {197, (PVOID)0x805a67ba, "NtReplyWaitReplyPort"},
    {198, (PVOID)0x805c95be, "NtRequestDeviceWakeup"},
    {199, (PVOID)0x805a3a2e, "NtRequestPort"},
    {200, (PVOID)0x805a3d5a, "NtRequestWaitReplyPort"},
    {201, (PVOID)0x805c93cc, "NtRequestWakeupLatency"},
    {202, (PVOID)0x8060f994, "NtResetEvent"},
    {203, (PVOID)0x80522662, "NtResetWriteWatch"},
    {204, (PVOID)0x8062613e, "NtRestoreKey"},
    {205, (PVOID)0x805d5a94, "NtResumeProcess"},
    {206, (PVOID)0x805d5976, "NtResumeThread"},
    {207, (PVOID)0x8062623a, "NtSaveKey"},
    {208, (PVOID)0x80626320, "NtSaveKeyEx"},
    {209, (PVOID)0x80626448, "NtSaveMergedKeys"},
    {210, (PVOID)0x805a4d48, "NtSecureConnectPort"},
    {211, (PVOID)0x806170e4, "NtAddBootEntry"},
    {212, (PVOID)0x806170e4, "NtAddBootEntry"},
    {213, (PVOID)0x805d26f6, "NtSetContextThread"},
    {214, (PVOID)0x80646ce0, "NtSetDebugFilterState"},
    {215, (PVOID)0x806153d0, "NtSetDefaultHardErrorPort"},
    {216, (PVOID)0x806114fc, "NtSetDefaultLocale"},
    {217, (PVOID)0x80611d6e, "NtSetDefaultUILanguage"},
    {218, (PVOID)0x8057b6c6, "NtSetEaFile"},
    {219, (PVOID)0x8060fa54, "NtSetEvent"},
    {220, (PVOID)0x8060fb1e, "NtSetEventBoostPriority"},
    {221, (PVOID)0x80617cee, "NtSetHighEventPair"},
    {222, (PVOID)0x80617c1e, "NtSetHighWaitLowEventPair"},
    {223, (PVOID)0x806439f2, "NtSetInformationDebugObject"},
    {224, (PVOID)0x8057c010, "NtSetInformationFile"},
    {225, (PVOID)0x805d7928, "NtSetInformationJobObject"},
    {226, (PVOID)0x806234a0, "NtSetInformationKey"},
    {227, (PVOID)0x805c57fe, "NtSetInformationObject"},
    {228, (PVOID)0x805cee46, "NtSetInformationProcess"},
    {229, (PVOID)0x805cd0ca, "NtSetInformationThread"},
    {230, (PVOID)0x805fae60, "NtSetInformationToken"},
    {231, (PVOID)0x80618156, "NtSetIntervalProfile"},
    {232, (PVOID)0x80579e78, "NtSetIoCompletion"},
    {233, (PVOID)0x805d48c0, "NtSetLdtEntries"},
    {234, (PVOID)0x80617c8a, "NtSetLowEventPair"},
    {235, (PVOID)0x80617bb2, "NtSetLowWaitHighEventPair"},
    {236, (PVOID)0x8057c7de, "NtSetQuotaInformationFile"},
    {237, (PVOID)0x805c15ea, "NtSetSecurityObject"},
    {238, (PVOID)0x80617384, "NtSetSystemEnvironmentValue"},
    {239, (PVOID)0x806170ba, "NtSetSystemEnvironmentValueEx"},
    {240, (PVOID)0x806103ba, "NtSetSystemInformation"},
    {241, (PVOID)0x80653e18, "NtSetSystemPowerState"},
    {242, (PVOID)0x80614b54, "NtSetSystemTime"},
    {243, (PVOID)0x805c92e0, "NtSetThreadExecutionState"},
    {244, (PVOID)0x80539d62, "NtSetTimer"},
    {245, (PVOID)0x80614026, "NtSetTimerResolution"},
    {246, (PVOID)0x80615e9a, "NtSetUuidSeed"},
    {247, (PVOID)0x80622d0c, "NtSetValueKey"},
    {248, (PVOID)0x8057d0f4, "NtSetVolumeInformationFile"},
    {249, (PVOID)0x80613644, "NtShutdownSystem"},
    {250, (PVOID)0x80527758, "NtSignalAndWaitForSingleObject"},
    {251, (PVOID)0x806183a0, "NtStartProfile"},
    {252, (PVOID)0x8061854a, "NtStopProfile"},
    {253, (PVOID)0x805d5a3e, "NtSuspendProcess"},
    {254, (PVOID)0x805d58b0, "NtSuspendThread"},
    {255, (PVOID)0x8061876e, "NtSystemDebugControl"},
    {256, (PVOID)0x805d84bc, "NtTerminateJobObject"},
    {257, (PVOID)0x805d399e, "NtTerminateProcess"},
    {258, (PVOID)0x805d3b98, "NtTerminateThread"},
    {259, (PVOID)0x805d5bfe, "NtTestAlert"},
    {260, (PVOID)0x805360f8, "NtTraceEvent"},
    {261, (PVOID)0x806170f2, "NtTranslateFilePath"},
    {262, (PVOID)0x805852ce, "NtUnloadDriver"},
    {263, (PVOID)0x80623036, "NtUnloadKey"},
    {264, (PVOID)0x80623250, "NtUnloadKeyEx"},
    {265, (PVOID)0x8057a656, "NtUnlockFile"},
    {266, (PVOID)0x805b7e9c, "NtUnlockVirtualMemory"},
    {267, (PVOID)0x805b3e14, "NtUnmapViewOfSection"},
    {268, (PVOID)0x805fc218, "NtVdmControl"},
    {269, (PVOID)0x8064375a, "NtWaitForDebugEvent"},
    {270, (PVOID)0x805c17a0, "NtWaitForMultipleObjects"},
    {271, (PVOID)0x805c16b6, "NtWaitForSingleObject"},
    {272, (PVOID)0x80617b4e, "NtWaitHighEventPair"},
    {273, (PVOID)0x80617aea, "NtWaitLowEventPair"},
    {274, (PVOID)0x8057def2, "NtWriteFile"},
    {275, (PVOID)0x8057e4d6, "NtWriteFileGather"},
    {276, (PVOID)0x805a6e78, "NtWriteRequestData"},
    {277, (PVOID)0x805b5394, "NtWriteVirtualMemory"},
    {278, (PVOID)0x80505ad8, "NtYieldExecution"},
    {279, (PVOID)0x80618bc6, "NtCreateKeyedEvent"},
    {280, (PVOID)0x80618cb0, "NtOpenKeyedEvent"},
    {281, (PVOID)0x80618d62, "NtReleaseKeyedEvent"},
    {282, (PVOID)0x80618fbe, "NtWaitForKeyedEvent"},
    {283, (PVOID)0x805cc8fe, "NtQueryPortInformationProcess"}
};
