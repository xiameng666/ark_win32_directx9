#include "ArkR3.h"

bool ArkR3::EnumProcess(PROCESS_INFO* out, int count)
{
    DWORD ret = 0;
    BOOL flag = DeviceIoControl(
        m_hDevice,           // �豸���
        CTL_ENUM_PROCESS,    // ������
        nullptr, 0,          // ������
        out, sizeof(PROCESS_INFO) * count, // ���������
        &ret, nullptr
    );

    return flag;
}
