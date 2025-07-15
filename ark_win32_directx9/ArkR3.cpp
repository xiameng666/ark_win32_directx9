#include "ArkR3.h"
#include <windows.h>

// ���캯��
ArkR3::ArkR3() : memBuffer_(nullptr), memBufferSize_(0), memDataSize_(0)
{
    // ��ʼ��ʱ����һ��������С�Ļ�����
    MemEnsureBufferSize(4096); // ��ʼ4KB
}

// ��������
ArkR3::~ArkR3()
{
    if (memBuffer_) {
        free(memBuffer_);
        memBuffer_ = nullptr;
    }
    memBufferSize_ = 0;
    memDataSize_ = 0;
}

// ȷ����������С�㹻
BOOL ArkR3::MemEnsureBufferSize(DWORD requiredSize)
{
    if (requiredSize > 0x100000) { // �������1MB
        Log("EnsureBufferSize: Size too large (%d bytes)", requiredSize);
        return FALSE;
    }

    if (memBufferSize_ >= requiredSize) {
        return TRUE; // ��ǰ���������㹻
    }

    // �����µĻ�������С������ȡ����4KB�߽磩
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
        DWORD_PTR mask = 1UL << i;  //Mask��λ �� i һ�� 
        SetProcessAffinityMask(GetCurrentProcess(), mask);
        __asm {
            sgdt gdtr
        }

        Log("GetGDTVec CPU %d: GDTR Base=%p, Limit=%X\n", i, (void*)gdtr.Base, gdtr.Limit);
        PSEGDESC pGdtData = GDTGetSingle(i, &gdtr);
        if (pGdtData) {
            DWORD descCount = (gdtr.Limit + 1) / 8;  // ������������
            Log("GetGDTVec CPU %d: ���� %d ����������\n", i, descCount);

            for (USHORT index = 0; index < descCount; index++) {
                //pDesc�Ƕ�������ָ�� ���潫ԭʼ����ת����UI����ʾ�����ݸ�ʽ 
                PSEGDESC pDesc = (PSEGDESC)((PUCHAR)pGdtData + index * sizeof(SegmentDescriptor));

                // ������GDT_INFO
                GDT_INFO gdtInfo = { 0 };
                gdtInfo.cpuIndex = i;
                gdtInfo.selector = index * sizeof(SegmentDescriptor);

                // ��ַ���� 32bit = Base1(16) + Base2(8) + Base3(8)
                gdtInfo.base = pDesc->Base1 | (pDesc->Base2 << 16) | (pDesc->Base3 << 24);

                // �������飺Limit1(16) + Limit2(4)
                gdtInfo.limit = pDesc->Limit1 | (pDesc->Limit2 << 16);

                // ������ 
                gdtInfo.g = pDesc->g;
                if (gdtInfo.g) {
                    gdtInfo.limit = (gdtInfo.limit << 12) | 0xFFF;  // ��12bit��1 = 4K
                }

                gdtInfo.dpl = pDesc->dpl;           // ����Ȩ��
                gdtInfo.type = pDesc->type;         // ������
                gdtInfo.system = pDesc->s;          // ϵͳ�α�־
                gdtInfo.p = pDesc->p;               // ����λ

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

    Log("GetGDTVec�ɹ���ȡ %zu ������������Ϣ\n", GDTVec_.size());
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

            Log("ProcessGetVec ����[%d]: PID=%d, ��PID=%d, ����=%s, EPROCESS=%p\n",
                i, pInfo.ProcessId, pInfo.ParentProcessId,
                pInfo.ImageFileName, pInfo.EprocessAddr);
        }
    }
    
    free(pEntryInfo);

    return ProcVec_;
}

//// ��ȡ������Ϣ���洢��������  
//std::vector<PROCESSENTRY32> ArkR3::EnumProcesses32() {
//    HANDLE hSnapshot = INVALID_HANDLE_VALUE;
//    PROCESSENTRY32 pe32;
//
//    ProcVec_.clear();
//
//    // �������̿���
//    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
//    if (hSnapshot == INVALID_HANDLE_VALUE) {
//        LogErr("CreateToolhelp32Snapshotʧ��\n");
//        return ProcVec_;
//    }
//
//    pe32.dwSize = sizeof(PROCESSENTRY32);
//
//    if (!Process32First(hSnapshot, &pe32)) {
//        Log("Process32Firstʧ��\n");
//        CloseHandle(hSnapshot);
//        return ProcVec_;
//    }
//
//    do {
//        ProcVec_.push_back(pe32);
//    } while (Process32Next(hSnapshot, &pe32));
//
//    CloseHandle(hSnapshot);
//    Log("��ȡ�� %d ������\n", (int)ProcVec_.size());
//    return ProcVec_;
//}

//��ȡ
//R3 : [PROCESS_MEM_REQ] �� R0 �� R0 : [��ȡ����] �� R3
//����12�ֽ�����          ����Size�ֽ�
BOOL ArkR3::MemAttachRead(DWORD ProcessId, ULONG VirtualAddress, DWORD Size)
{
    // ������֤
    if (ProcessId == 0 || Size == 0) {
        LogErr("AttachReadMem: Invalid args");
        return FALSE;
    }

    // ȷ���ڲ��������㹻��
    if (!MemEnsureBufferSize(Size)) {
        LogErr("AttachReadMem: Failed to ensure buffer size");
        return FALSE;
    }

    // ��������ṹ�壨ջ�Ϸ��伴�ɣ�
    PROCESS_MEM_REQ req;
    req.ProcessId = (HANDLE)ProcessId;
    req.VirtualAddress = (PVOID)VirtualAddress;
    req.Size = Size;

    DWORD dwRetBytes = 0;
    BOOL bResult = DeviceIoControl(m_hDriver, CTL_ATTACH_MEM_READ,
        &req, sizeof(PROCESS_MEM_REQ),           // ���룺����ṹ��
        memBuffer_, Size,                        // �����ֱ��д���ڲ�������
        &dwRetBytes, NULL);

    if (bResult) {
        memDataSize_ = Size;  // ������⿽����

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

//д�룺
//R3 : [PROCESS_MEM_REQ] [д������] �� R0 �� �������
//����12 + Size�ֽ�              ����Ҫ��������
BOOL ArkR3::MemAttachWrite(DWORD ProcessId, ULONG VirtualAddress, DWORD Size)
{
    // ������֤
    if (ProcessId == 0 || VirtualAddress == 0 || Size == 0) {
        Log("AttachWriteMem: Invalid parameters");
        return FALSE;
    }

    // ����ڲ��������Ƿ����㹻������
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

    // ��������ͷ
    PPROCESS_MEM_REQ req = (PPROCESS_MEM_REQ)pBuffer;
    req->ProcessId = (HANDLE)ProcessId;
    req->VirtualAddress = (PVOID)VirtualAddress;
    req->Size = Size;
    
    // ���ڲ��������������ݵ����󻺳���
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

// ��ȡ�ں�ģ������
DWORD ArkR3::ModuleGetCount()
{
    DWORD dwBytes = 0;
    DWORD dwEntryNum = 0;

    DeviceIoControl(m_hDriver, CTL_ENUM_MODULE_COUNT, NULL, NULL, &dwEntryNum, sizeof(DWORD), &dwBytes, NULL);

    return dwEntryNum;
}

//��ȡ�ں�ģ����Ϣ
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

            Log("ModuleGetVec ģ��[%d]: ����=%s, ����ַ=%p, ��С=0x%X, ·��=%s\n",
                i, mInfo.Name, mInfo.ImageBase, mInfo.ImageSize, mInfo.FullPath);
        }
    }

    free(pEntryInfo);
    return MoudleVec_;
}

// ��ȡָ�����̵�ģ������
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

// ��ȡָ�����̵�ģ����Ϣ
std::vector<MODULE_INFO> ArkR3::ProcessModuleGetVec(DWORD processId, DWORD moduleCount)
{
    DWORD dwRetBytes;
    DWORD dwBufferSize = sizeof(MODULE_INFO) * moduleCount;
    PMODULE_INFO pEntryInfo = (PMODULE_INFO)malloc(dwBufferSize);
    
    PROCESS_MODULE_REQ req;
    req.ProcessId = (HANDLE)processId;
    req.ModuleCount = moduleCount;
    
    // ������ṹ���Ƶ���������ͷ
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

            Log("ProcessModuleGetVec ����[%d]ģ��[%d]: ����=%s, ����ַ=%p, ��С=0x%X\n",
                processId, i, mInfo.Name, mInfo.ImageBase, mInfo.ImageSize);
        }
    }
    
    free(pEntryInfo);

    return ProcessModuleVec_;
}
