#include "process.h"

extern ULONG g_WindowsVersion;
ENUM_PROCESS_OFFSETS g_ProcessOffsets;

// ��ʼ��ƫ����
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
        mov ebx, g_ProcessOffsets.EThreadToProcess     // 0x44  �� 0x150
        mov eax, [eax + ebx]                    // ETHREAD.ThreadsProcess
        mov Process, eax
    }

    return Process;
}

//onlyGetCount �ж� ��ȡ�����������Ǳ�����������
NTSTATUS EnumProcessEx(PPROCESS_INFO processBuffer, bool onlyGetCount, PULONG processCount) {
    PEPROCESS CurrentProcess = NULL;
    PEPROCESS StartProcess = NULL;//��¼��ʼ�Ľ��̱���ѭ�� ˫������
    ULONG Counter = 0;

    __try {
        StartProcess = GetCurrentEprocess();
        CurrentProcess = StartProcess;

        KdPrint(("[XM] ��ʼ�������̣���ʼEPROCESS: %p\n", CurrentProcess));

        do {
            // �����Ҫ�ý�������
            if (!onlyGetCount&& processBuffer!=NULL) {
                PPROCESS_INFO pInfo = &processBuffer[Counter];
                RtlZeroMemory(pInfo, sizeof(PROCESS_INFO));

                // ��ȡ������Ϣ 
                pInfo->ProcessId = *(PULONG)((PUCHAR)CurrentProcess + g_ProcessOffsets.ProcessId);
                pInfo->ParentProcessId = *(PULONG)((PUCHAR)CurrentProcess + g_ProcessOffsets.ParentProcessId);
                pInfo->EprocessAddr = CurrentProcess;
                pInfo->DirectoryTableBase = *(PULONG)((PUCHAR)CurrentProcess + g_ProcessOffsets.DirectoryTableBase);

                // ���ƽ�����
                PUCHAR ImageFileName = (PUCHAR)CurrentProcess + g_ProcessOffsets.ImageFileName;
                RtlCopyMemory(pInfo->ImageFileName, ImageFileName, 16);
                pInfo->ImageFileName[15] = '\0';

                KdPrint(("[XM] ����[%d]: PID=%d, Name=%s, EPROCESS=%p\n",
                    Counter, pInfo->ProcessId, pInfo->ImageFileName, CurrentProcess));
            }

            Counter++;

            //��һ������
            PULONG ActiveProcessLinksPtr = (PULONG)((PUCHAR)CurrentProcess + g_ProcessOffsets.ActiveProcessLinks);
            ULONG NextLinkAddress = *ActiveProcessLinksPtr;  // ��ȡFlink
            CurrentProcess = (PEPROCESS)(NextLinkAddress - g_ProcessOffsets.ActiveProcessLinks);  // ��ȥƫ�Ƶõ�EPROCESS��ַ

        } while (CurrentProcess != StartProcess && Counter < 1000);//������ѭ��

        *processCount = Counter;
        KdPrint(("[XM] ������ɣ��ܹ��ҵ� %d ������\n", Counter));

        return STATUS_SUCCESS;

    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        KdPrint(("[XM] err EnumerateProcessCount \n"));
        return STATUS_UNSUCCESSFUL;
    }
}
