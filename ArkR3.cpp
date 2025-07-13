#include "ArkR3.h"
#include <windows.h>

// 构造函数
ArkR3::ArkR3() : m_pMemBuffer(nullptr), m_dwBufferSize(0), m_dwDataSize(0)
{
    // 初始化时分配一个基础大小的缓冲区
    EnsureBufferSize(4096); // 初始4KB
}

// 析构函数
ArkR3::~ArkR3()
{
    if (m_pMemBuffer) {
        free(m_pMemBuffer);
        m_pMemBuffer = nullptr;
    }
    m_dwBufferSize = 0;
    m_dwDataSize = 0;
}

// 确保缓冲区大小足够
BOOL ArkR3::EnsureBufferSize(DWORD requiredSize)
{
    if (requiredSize > 0x100000) { // 限制最大1MB
        Log("EnsureBufferSize: Size too large (%d bytes)", requiredSize);
        return FALSE;
    }

    if (m_dwBufferSize >= requiredSize) {
        return TRUE; // 当前缓冲区已足够
    }

    // 计算新的缓冲区大小（向上取整到4KB边界）
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

// 清空缓冲区
void ArkR3::ClearBuffer()
{
    if (m_pMemBuffer && m_dwBufferSize > 0) {
        memset(m_pMemBuffer, 0, m_dwBufferSize);
    }
    m_dwDataSize = 0;
}

// 从十六进制字符串设置数据
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

    // 转换十六进制字符串为字节数组
    PUCHAR pBuffer = (PUCHAR)m_pMemBuffer;
    for (size_t i = 0; i < dataSize; i++) {
        char hexByte[3] = {hexString[i*2], hexString[i*2+1], 0};
        pBuffer[i] = (UCHAR)strtoul(hexByte, NULL, 16);
    }

    m_dwDataSize = dataSize;
    Log("SetWriteDataFromHex: %d bytes converted from hex string", dataSize);
    return TRUE;
}

// 获取格式化的十六进制字符串表示（16字节一行）
std::string ArkR3::GetDataAsFormattedHexString() const
{
    if (!m_pMemBuffer || m_dwDataSize == 0) {
        return "";
    }

    std::string hexStr;
    hexStr.reserve(m_dwDataSize * 3 + m_dwDataSize / 16); // 预留换行符空间
    
    PUCHAR pData = (PUCHAR)m_pMemBuffer;
    for (DWORD i = 0; i < m_dwDataSize; i++) {
        char hexByte[4];
        sprintf_s(hexByte, "%02X", pData[i]);
        hexStr += hexByte;
        
        // 每16个字节换行
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
BOOL ArkR3::AttachReadMem(DWORD ProcessId, ULONG VirtualAddress, DWORD Size)
{
    // 参数验证
    if (ProcessId == 0 || Size == 0) {
        LogErr("AttachReadMem: Invalid args");
        return FALSE;
    }

    // 确保内部缓冲区足够大
    if (!EnsureBufferSize(Size)) {
        LogErr("AttachReadMem: Failed to ensure buffer size");
        return FALSE;
    }

    // 分配临时缓冲区
    DWORD totalSize = sizeof(PROCESS_MEM_REQ) + Size;
    PVOID pTempBuffer = malloc(totalSize);
    if (!pTempBuffer) {
        Log("AttachReadMem: Failed to allocate temp buffer, size: %d", totalSize);
        return FALSE;
    }

    // 构造请求
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
        // 将读到的数据复制到内部缓冲区
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

//写入：
//R3 : [PROCESS_MEM_REQ] [写入数据] → R0 → 处理完成
//发送12 + Size字节              不需要返回数据
BOOL ArkR3::AttachWriteMem(DWORD ProcessId, ULONG VirtualAddress, DWORD Size)
{
    // 参数验证
    if (ProcessId == 0 || VirtualAddress == 0 || Size == 0) {
        Log("AttachWriteMem: Invalid parameters");
        return FALSE;
    }

    // 检查内部缓冲区是否有足够的数据
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

    // 构造请求头
    PPROCESS_MEM_REQ req = (PPROCESS_MEM_REQ)pBuffer;
    req->ProcessId = (HANDLE)ProcessId;
    req->VirtualAddress = (PVOID)VirtualAddress;
    req->Size = Size;
    
    // 从内部缓冲区复制数据到请求缓冲区
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
