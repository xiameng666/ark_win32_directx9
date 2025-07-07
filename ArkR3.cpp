#include "ArkR3.h"

bool ArkR3::EnumProcess(PROCESS_INFO* out, int count)
{
    DWORD ret = 0;
    BOOL flag = DeviceIoControl(
        m_hDevice,           // 设备句柄
        CTL_ENUM_PROCESS,    // 控制码
        nullptr, 0,          // 无输入
        out, sizeof(PROCESS_INFO) * count, // 输出缓冲区
        &ret, nullptr
    );

    return flag;
}
