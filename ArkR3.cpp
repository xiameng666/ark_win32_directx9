#include "ArkR3.h"

PPROCESS_INFO ArkR3::GetProcessInfo(DWORD dwEntryNum)
{
    DWORD dwBytes;
    DWORD dwBufferSize = sizeof(PROCESS_INFO) * dwEntryNum;
    PPROCESS_INFO pEntryInfo = (PPROCESS_INFO)malloc(dwBufferSize);
    DeviceIoControl(m_hDriver, CTL_ENUM_PROCESS, NULL, NULL, pEntryInfo, dwBufferSize, &dwBytes, NULL);
    return pEntryInfo;
}
