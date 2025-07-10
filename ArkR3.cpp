#include "ArkR3.h"


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
    _gdtVec.clear();

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

            for (UINT index = 0; index < descCount; index++) {
                //pDesc是段描述符指针 下面将原始数据转换成UI上显示的数据格式 
                PSEGDESC pDesc = (PSEGDESC)((PUCHAR)pGdtData + index * sizeof(SegmentDescriptor));

                //跳过空描述符 
  /*              if (index == 0 || !pDesc->p ||
                    (pDesc->Base1 == 0 && pDesc->Base2 == 0 && pDesc->Base3 == 0 &&
                        pDesc->Limit1 == 0 && pDesc->Limit2 == 0 && pDesc->type == 0)) {
                    continue;  
                }*/

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

                _gdtVec.emplace_back(gdtInfo);
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

    Log("GetGDTVec成功获取 %zu 个段描述符信息\n", _gdtVec.size());
    return _gdtVec;
}

PPROCESS_INFO ArkR3::GetProcessInfo(DWORD dwEntryNum)
{
    DWORD dwRetBytes;
    DWORD dwBufferSize = sizeof(PROCESS_INFO) * dwEntryNum;
    PPROCESS_INFO pEntryInfo = (PPROCESS_INFO)malloc(dwBufferSize);
    DeviceIoControl(m_hDriver, CTL_ENUM_PROCESS, NULL, NULL, pEntryInfo, dwBufferSize, &dwRetBytes, NULL);
    return pEntryInfo;
}
