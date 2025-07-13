#include "ArkR3.h"
#include <windows.h>

// ���캯��
ArkR3::ArkR3() : m_pMemBuffer(nullptr), m_dwBufferSize(0), m_dwDataSize(0)
{
    // ��ʼ��ʱ����һ��������С�Ļ�����
    EnsureBufferSize(4096); // ��ʼ4KB
}

// ��������
ArkR3::~ArkR3()
{
    if (m_pMemBuffer) {
        free(m_pMemBuffer);
        m_pMemBuffer = nullptr;
    }
    m_dwBufferSize = 0;
    m_dwDataSize = 0;
}

// ȷ����������С�㹻
BOOL ArkR3::EnsureBufferSize(DWORD requiredSize)
{
    if (requiredSize > 0x100000) { // �������1MB
        Log("EnsureBufferSize: Size too large (%d bytes)", requiredSize);
        return FALSE;
    }

    if (m_dwBufferSize >= requiredSize) {
        return TRUE; // ��ǰ���������㹻
    }

    // �����µĻ�������С������ȡ����4KB�߽磩
    DWORD newSize = ((requiredSize + 4095) / 4096) * 4096;
    
    PVOID newBuffer = realloc(m_pMemBuffer, newSize);
    if (!newBuffer) {
        Log("EnsureBufferSize: Failed to allocate %d bytes", newSize);
        return FALSE;
    }

    m_pMemBuffer = newBuffer;
    m_dwBufferSize = newSize;
    
    Log("EnsureBufferSize: Buffer resized to %d bytes", newSize);
    return TRUE;
}

// ��ջ�����
void ArkR3::ClearBuffer()
{
    if (m_pMemBuffer && m_dwBufferSize > 0) {
        memset(m_pMemBuffer, 0, m_dwBufferSize);
    }
    m_dwDataSize = 0;
}

// ��ʮ�������ַ�����������
BOOL ArkR3::SetWriteDataFromHex(const char* hexString)
{
    if (!hexString || strlen(hexString) == 0) {
        LogErr("SetWriteDataFromHex: Invalid hex string");
        return FALSE;
    }

    size_t hexLen = strlen(hexString);
    if (hexLen % 2 != 0) {
        LogErr("SetWriteDataFromHex: Hex string length must be even");
        return FALSE;
    }

    DWORD dataSize = hexLen / 2;
    if (!EnsureBufferSize(dataSize)) {
        return FALSE;
    }

    // ת��ʮ�������ַ���Ϊ�ֽ�����
    PUCHAR pBuffer = (PUCHAR)m_pMemBuffer;
    for (size_t i = 0; i < dataSize; i++) {
        char hexByte[3] = {hexString[i*2], hexString[i*2+1], 0};
        pBuffer[i] = (UCHAR)strtoul(hexByte, NULL, 16);
    }

    m_dwDataSize = dataSize;
    Log("SetWriteDataFromHex: %d bytes converted from hex string", dataSize);
    return TRUE;
}

// ��ȡ��ʽ����ʮ�������ַ�����ʾ��16�ֽ�һ�У�
std::string ArkR3::GetDataAsFormattedHexString() const
{
    if (!m_pMemBuffer || m_dwDataSize == 0) {
        return "";
    }

    std::string hexStr;
    hexStr.reserve(m_dwDataSize * 3 + m_dwDataSize / 16); // Ԥ�����з��ռ�
    
    PUCHAR pData = (PUCHAR)m_pMemBuffer;
    for (DWORD i = 0; i < m_dwDataSize; i++) {
        char hexByte[4];
        sprintf_s(hexByte, "%02X", pData[i]);
        hexStr += hexByte;
        
        // ÿ16���ֽڻ���
        if ((i + 1) % 16 == 0 && i + 1 < m_dwDataSize) {
            hexStr += "\n";
        }
    }

    return hexStr;
}


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
        DWORD_PTR mask = 1UL << i;  //Mask��λ �� i һ�� 
        SetProcessAffinityMask(GetCurrentProcess(), mask);
        __asm {
            sgdt gdtr
        }

        Log("GetGDTVec CPU %d: GDTR Base=%p, Limit=%X\n", i, (void*)gdtr.Base, gdtr.Limit);
        PSEGDESC pGdtData = GetSingeGDT(i, &gdtr);
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
                gdtInfo.base = pDesc->Base1 |(pDesc->Base2 << 16) |(pDesc->Base3 << 24);

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

//PPROCESS_INFO ArkR3::GetProcessInfo(DWORD dwEntryNum)
//{
//    DWORD dwRetBytes;
//    DWORD dwBufferSize = sizeof(PROCESS_INFO) * dwEntryNum;
//    PPROCESS_INFO pEntryInfo = (PPROCESS_INFO)malloc(dwBufferSize);
//    DeviceIoControl(m_hDriver, CTL_ENUM_PROCESS, NULL, NULL, pEntryInfo, dwBufferSize, &dwRetBytes, NULL);
//    return pEntryInfo;
//}


// ��ȡ������Ϣ���洢��������
std::vector<PROCESSENTRY32> ArkR3::EnumProcesses32() {
    HANDLE hSnapshot = INVALID_HANDLE_VALUE;
    PROCESSENTRY32 pe32;

    ProcVec_.clear();

    // �������̿���
    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        LogErr("CreateToolhelp32Snapshotʧ��\n");
        return ProcVec_;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnapshot, &pe32)) {
        Log("Process32Firstʧ��\n");
        CloseHandle(hSnapshot);
        return ProcVec_;
    }

    do {
        ProcVec_.push_back(pe32);
    } while (Process32Next(hSnapshot, &pe32));

    CloseHandle(hSnapshot);
    Log("��ȡ�� %d ������\n", (int)ProcVec_.size());
    return ProcVec_;
}

//��ȡ
//R3 : [PROCESS_MEM_REQ] �� R0 �� R0 : [��ȡ����] �� R3
//����12�ֽ�����          ����Size�ֽ�
BOOL ArkR3::AttachReadMem(DWORD ProcessId, ULONG VirtualAddress, DWORD Size)
{
    // ������֤
    if (ProcessId == 0 || Size == 0) {
        LogErr("AttachReadMem: Invalid args");
        return FALSE;
    }

    // ȷ���ڲ��������㹻��
    if (!EnsureBufferSize(Size)) {
        LogErr("AttachReadMem: Failed to ensure buffer size");
        return FALSE;
    }

    // ������ʱ������
    DWORD totalSize = sizeof(PROCESS_MEM_REQ) + Size;
    PVOID pTempBuffer = malloc(totalSize);
    if (!pTempBuffer) {
        Log("AttachReadMem: Failed to allocate temp buffer, size: %d", totalSize);
        return FALSE;
    }

    // ��������
    PPROCESS_MEM_REQ req = (PPROCESS_MEM_REQ)pTempBuffer;
    req->ProcessId = (HANDLE)ProcessId;
    req->VirtualAddress = (PVOID)VirtualAddress;
    req->Size = Size;

    DWORD dwRetBytes = 0;
    BOOL bResult = DeviceIoControl(m_hDriver, CTL_ATTACH_MEM_READ, 
        pTempBuffer, sizeof(PROCESS_MEM_REQ),
        pTempBuffer, Size,
        &dwRetBytes, NULL);

    if (bResult) {
        // �����������ݸ��Ƶ��ڲ�������
        memcpy(m_pMemBuffer, pTempBuffer, Size);
        m_dwDataSize = Size;
        
        Log("AttachReadMem: PID=%d, Addr=0x%08X, Size=%d - Success", 
            ProcessId, VirtualAddress, Size);
            
        free(pTempBuffer);
        return TRUE;
    }
    else {
        Log("AttachReadMem: DeviceIoControl failed, PID=%d, Addr=0x%08X, Size=%d", 
               ProcessId, VirtualAddress, Size);
        free(pTempBuffer);
        return FALSE;
    }
}

//д�룺
//R3 : [PROCESS_MEM_REQ] [д������] �� R0 �� �������
//����12 + Size�ֽ�              ����Ҫ��������
BOOL ArkR3::AttachWriteMem(DWORD ProcessId, ULONG VirtualAddress, DWORD Size)
{
    // ������֤
    if (ProcessId == 0 || VirtualAddress == 0 || Size == 0) {
        Log("AttachWriteMem: Invalid parameters");
        return FALSE;
    }

    // ����ڲ��������Ƿ����㹻������
    if (m_dwDataSize < Size) {
        Log("AttachWriteMem: Not enough data in buffer, available: %d, required: %d", 
               m_dwDataSize, Size);
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
    memcpy((PUCHAR)pBuffer + sizeof(PROCESS_MEM_REQ), m_pMemBuffer, Size);

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
