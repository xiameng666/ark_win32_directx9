#pragma once
#include <ntifs.h>
#include <ntddk.h>
#include "../include/proto.h"

extern "C" {

    // �������ƫ����
    typedef struct ENUM_PROCESS_OFFSETS {
        ULONG EThreadToProcess;     // ETHREAD -> EPROCESS ƫ��
        ULONG ProcessId;           // UniqueProcessId ƫ��
        ULONG ActiveProcessLinks;  // ActiveProcessLinks ƫ��  
        ULONG ParentProcessId;     // InheritedFromUniqueProcessId ƫ��
        ULONG ImageFileName;       // ImageFileName ƫ��
        ULONG DirectoryTableBase;  // CR3 ƫ��
    }*PENUM_PROCESS_OFFSETS;

    // �ⲿ��������
    extern ENUM_PROCESS_OFFSETS g_ProcessOffsets;

    NTSTATUS InitProcessOffsets();

    PEPROCESS GetCurrentEprocess();

    //onlyGetCount �ж� ��ȡ�����������Ǳ�����������
    NTSTATUS EnumProcessEx(PPROCESS_INFO processBuffer, bool onlyGetCount, PULONG processCount);

}
