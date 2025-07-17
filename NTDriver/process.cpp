#include "process.h"
#include "driver.h"

extern ULONG g_WindowsVersion;
ENUM_PROCESS_OFFSETS g_ProcessOffsets;

// 初始化偏移量
NTSTATUS InitProcessOffsets() {
    if (g_WindowsVersion == WinXP) {
        g_ProcessOffsets.EThreadToProcess = 0x44;

        g_ProcessOffsets.ProcessId = 0x84;
        g_ProcessOffsets.ActiveProcessLinks = 0x88;
        g_ProcessOffsets.ParentProcessId = 0x14C;
        g_ProcessOffsets.ImageFileName = 0x174;
        g_ProcessOffsets.DirectoryTableBase = 0x18;
    }
    else if (g_WindowsVersion == Win7) {
        g_ProcessOffsets.EThreadToProcess = 0x150;

        g_ProcessOffsets.ProcessId = 0xB4;
        g_ProcessOffsets.ActiveProcessLinks = 0xB8;
        g_ProcessOffsets.ParentProcessId = 0x140;
        g_ProcessOffsets.ImageFileName = 0x16C;
        g_ProcessOffsets.DirectoryTableBase = 0x18;
    }

    return STATUS_SUCCESS;
}

PEPROCESS GetCurrentEprocess() {
    PEPROCESS Process = NULL;

    __asm {
        mov eax, fs: [0x124]                    //ETHREAD
        mov ebx, g_ProcessOffsets.EThreadToProcess     // 0x44  或 0x150
        mov eax, [eax + ebx]                    // ETHREAD.ThreadsProcess
        mov Process, eax
    }

    return Process;
}

//onlyGetCount 判断 获取进程数量还是遍历进程数据
NTSTATUS EnumProcessEx(PPROCESS_INFO processBuffer, bool onlyGetCount, PULONG processCount) {
    PEPROCESS CurrentProcess = NULL;
    PEPROCESS StartProcess = NULL;
    ULONG Counter = 0;

    __try {
        StartProcess = GetCurrentEprocess();
        CurrentProcess = StartProcess;

        Log("[XM] 开始遍历进程，起始EPROCESS: %p\n", CurrentProcess);

        do {
            // 如果需要拿进程数据
            if (!onlyGetCount&& processBuffer!=NULL) {
                PPROCESS_INFO pInfo = &processBuffer[Counter];
                RtlZeroMemory(pInfo, sizeof(PROCESS_INFO));

                // 提取进程信息 
                pInfo->ProcessId = *(PULONG)((PUCHAR)CurrentProcess + g_ProcessOffsets.ProcessId);
                pInfo->ParentProcessId = *(PULONG)((PUCHAR)CurrentProcess + g_ProcessOffsets.ParentProcessId);
                pInfo->EprocessAddr = CurrentProcess;
                pInfo->DirectoryTableBase = *(PULONG)((PUCHAR)CurrentProcess + g_ProcessOffsets.DirectoryTableBase);

                // 复制进程名
                PUCHAR ImageFileName = (PUCHAR)CurrentProcess + g_ProcessOffsets.ImageFileName;
                RtlCopyMemory(pInfo->ImageFileName, ImageFileName, 16);
                pInfo->ImageFileName[15] = '\0';

                Log("[XM] 进程[%d]: PID=%d, Name=%s, EPROCESS=%p\n",
                    Counter, pInfo->ProcessId, pInfo->ImageFileName, CurrentProcess);
            }

            Counter++;

            //下一个进程
            PULONG ActiveProcessLinksPtr = (PULONG)((PUCHAR)CurrentProcess + g_ProcessOffsets.ActiveProcessLinks);
            ULONG NextLinkAddress = *ActiveProcessLinksPtr;  // 获取Flink
            CurrentProcess = (PEPROCESS)(NextLinkAddress - g_ProcessOffsets.ActiveProcessLinks);  // 减去偏移得到EPROCESS地址

        } while (CurrentProcess != StartProcess && Counter < 1000);

        *processCount = Counter;
        Log("[XM] 遍历完成，总共找到 %d 个进程\n", Counter);

        return STATUS_SUCCESS;

    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("[XM] err EnumerateProcessCount \n");
        return STATUS_UNSUCCESSFUL;
    }
}
