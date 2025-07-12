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
BOOL ArkR3::AttachReadMem(DWORD ProcessId, ULONG VirtualAddress, PVOID Buffer ,DWORD Size)
{
    //����һ�����ݽṹ�洢���ص����� 
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
        pBuffer, sizeof(PROCESS_MEM_REQ),//��������ͷ�͹���
        pBuffer, Size,//���ܵ�ʱ��ֻ���ܶ��������� Ҳ����size��С �������صĸ�ʽ
        &dwRetBytes, NULL);

    if (bResult) {
        // ����������д��
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

//д�룺
//R3 : [PROCESS_MEM_REQ] [д������] �� R0 �� �������
//����12 + Size�ֽ�              ����Ҫ��������
BOOL ArkR3::AttachWriteMem(DWORD ProcessId, ULONG VirtualAddress, PVOID Buffer, DWORD Size)
{
    DWORD totalSize = sizeof(PROCESS_MEM_REQ) + Size;
    PVOID pBuffer = malloc(totalSize);//����ͷ��С+д������ݴ�С

    if (!pBuffer) {
        LogErr("WriteProcessMemory: err");
        return FALSE;
    }

    PPROCESS_MEM_REQ req = (PPROCESS_MEM_REQ)pBuffer;
    req->ProcessId = (HANDLE)ProcessId;
    req->VirtualAddress = (PVOID)VirtualAddress;
    req->Size = Size;
    memcpy((PUCHAR)pBuffer + sizeof(PROCESS_MEM_REQ), Buffer, Size);//��Ҫд������ݿ���������

    DWORD dwRetBytes = 0;
    BOOL bResult = DeviceIoControl(m_hDriver, CTL_ATTACH_MEM_WRITE,
        pBuffer, totalSize,           
        NULL,0,  // �������������Ϣ��
        &dwRetBytes, NULL);

    if (bResult) {
        Log("AttachWriteMem�ɹ�: PID=%d, Addr=0x%08X, Size=%d\n",
            ProcessId, VirtualAddress, Size);
        free(pBuffer);
        return TRUE;
    }
    else {
        Log("AttachWriteMemʧ��: PID=%d, Addr=0x%08X, Size=%d (dwRetBytes=%d)\n",
            ProcessId, VirtualAddress, Size, dwRetBytes);
        free(pBuffer);
        return FALSE;
    }
}
