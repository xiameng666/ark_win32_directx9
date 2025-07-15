#pragma once
#include <ntifs.h>
#include <ntddk.h>
//#include <ntstrsafe.h>  // 添加字符串安全函数支持
#include "../include/proto.h"
#include "process.h"

extern "C" {

    NTSTATUS DetectWindowsVersion();

    NTSTATUS AttachReadVirtualMem(HANDLE ProcessId, PVOID BaseAddress, PVOID Buffer, unsigned ReadBytes);

    NTSTATUS AttachWriteVirtualMem(HANDLE ProcessId, PVOID BaseAddress, PVOID Buffer, unsigned WriteBytes);

    // 模块遍历
    NTSTATUS EnumModuleEx(PMODULE_INFO ModuleBuffer, bool CountOnly, PULONG ModuleCount);
    
    // 进程模块遍历
    //NTSTATUS EnumProcessModuleEx(HANDLE ProcessId, PMODULE_INFO ModuleBuffer, bool CountOnly, PULONG ModuleCount);

    NTSTATUS CompleteRequest(struct _IRP* Irp, ULONG_PTR Information = 0, NTSTATUS Status = STATUS_SUCCESS);

    NTSTATUS DispatchCreate(_In_ struct _DEVICE_OBJECT* DeviceObject,
        _Inout_ struct _IRP* Irp);

    NTSTATUS DispatchClose(_In_ struct _DEVICE_OBJECT* DeviceObject,
        _Inout_ struct _IRP* Irp);

    NTSTATUS DispatchRead(_In_ struct _DEVICE_OBJECT* DeviceObject,
        _Inout_ struct _IRP* Irp);
    NTSTATUS DispatchWrite(_In_ struct _DEVICE_OBJECT* DeviceObject,
        _Inout_ struct _IRP* Irp);

    NTSTATUS DispatchDeviceControl(_In_ struct _DEVICE_OBJECT* DeviceObject,
        _Inout_ struct _IRP* Irp);

    VOID Unload(__in struct _DRIVER_OBJECT* DriverObject);

    NTSTATUS  DriverEntry(
        __in struct _DRIVER_OBJECT* DriverObject,
        __in PUNICODE_STRING  RegistryPath);

}

struct CMutex {
    void Init() {
        KeInitializeMutex(&m_Object, 0);
        KdPrint(("CMutex Init"));
    }

    void Lock() {
        KeWaitForSingleObject(&m_Object, Executive, KernelMode, TRUE, NULL);
        KdPrint(("CMutex lock"));
    }

    void Unlock() {
        KeReleaseMutex(&m_Object, FALSE);
        KdPrint(("CMutex Unlock"));
    }
private:
    KMUTEX m_Object;
};



struct MY_DEV_EXT {
    CMutex Lock;
};

typedef struct _SYSTEM_PROCESS_INFORMATION {
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    LARGE_INTEGER SpareLi1;
    LARGE_INTEGER SpareLi2;
    LARGE_INTEGER SpareLi3;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    UNICODE_STRING ImageName;
    KPRIORITY BasePriority;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
    ULONG HandleCount;
    ULONG SessionId;
    ULONG_PTR PageDirectoryBase;
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
    LARGE_INTEGER ReadOperationCount;
    LARGE_INTEGER WriteOperationCount;
    LARGE_INTEGER OtherOperationCount;
    LARGE_INTEGER ReadTransferCount;
    LARGE_INTEGER WriteTransferCount;
    LARGE_INTEGER OtherTransferCount;
} SYSTEM_PROCESS_INFORMATION, * PSYSTEM_PROCESS_INFORMATION;

typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation,
    SystemProcessorInformation,             // obsolete...delete
    SystemPerformanceInformation,
    SystemTimeOfDayInformation,
    SystemPathInformation,
    SystemProcessInformation,
    SystemCallCountInformation,
    SystemDeviceInformation,
    SystemProcessorPerformanceInformation,
    SystemFlagsInformation,
    SystemCallTimeInformation,
    SystemModuleInformation,
    SystemLocksInformation,
    SystemStackTraceInformation,
    SystemPagedPoolInformation,
    SystemNonPagedPoolInformation,
    SystemHandleInformation,
    SystemObjectInformation,
    SystemPageFileInformation,
    SystemVdmInstemulInformation,
    SystemVdmBopInformation,
    SystemFileCacheInformation,
    SystemPoolTagInformation,
    SystemInterruptInformation,
    SystemDpcBehaviorInformation,
    SystemFullMemoryInformation,
    SystemLoadGdiDriverInformation,
    SystemUnloadGdiDriverInformation,
    SystemTimeAdjustmentInformation,
    SystemSummaryMemoryInformation,
    SystemMirrorMemoryInformation,
    SystemPerformanceTraceInformation,
    SystemObsolete0,
    SystemExceptionInformation,
    SystemCrashDumpStateInformation,
    SystemKernelDebuggerInformation,
    SystemContextSwitchInformation,
    SystemRegistryQuotaInformation,
    SystemExtendServiceTableInformation,
    SystemPrioritySeperation,
    SystemVerifierAddDriverInformation,
    SystemVerifierRemoveDriverInformation,
    SystemProcessorIdleInformation,
    SystemLegacyDriverInformation,
    SystemCurrentTimeZoneInformation,
    SystemLookasideInformation,
    SystemTimeSlipNotification,
    SystemSessionCreate,
    SystemSessionDetach,
    SystemSessionInformation,
    SystemRangeStartInformation,
    SystemVerifierInformation,
    SystemVerifierThunkExtend,
    SystemSessionProcessInformation,
    SystemLoadGdiDriverInSystemSpace,
    SystemNumaProcessorMap,
    SystemPrefetcherInformation,
    SystemExtendedProcessInformation,
    SystemRecommendedSharedDataAlignment,
    SystemComPlusPackage,
    SystemNumaAvailableMemory,
    SystemProcessorPowerInformation,
    SystemEmulationBasicInformation,
    SystemEmulationProcessorInformation,
    SystemExtendedHandleInformation,
    SystemLostDelayedWriteInformation,
    SystemBigPoolInformation,
    SystemSessionPoolTagInformation,
    SystemSessionMappedViewInformation,
    SystemHotpatchInformation,
    SystemObjectSecurityMode,
    SystemWatchdogTimerHandler,
    SystemWatchdogTimerInformation,
    SystemLogicalProcessorInformation,
    SystemWow64SharedInformation,
    SystemRegisterFirmwareTableInformationHandler,
    SystemFirmwareTableInformation,
    SystemModuleInformationEx,
    SystemVerifierTriageInformation,
    SystemSuperfetchInformation,
    SystemMemoryListInformation,
    SystemFileCacheInformationEx,
    MaxSystemInfoClass  // MaxSystemInfoClass should always be the last enum
} SYSTEM_INFORMATION_CLASS;

typedef struct _RTL_PROCESS_MODULE_INFORMATION {
    HANDLE Section;                 // Not filled in
    PVOID MappedBase;
    PVOID ImageBase;
    ULONG ImageSize;
    ULONG Flags;
    USHORT LoadOrderIndex;
    USHORT InitOrderIndex;
    USHORT LoadCount;
    USHORT OffsetToFileName;
    UCHAR  FullPathName[256];
} RTL_PROCESS_MODULE_INFORMATION, * PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES {
    ULONG NumberOfModules;
    RTL_PROCESS_MODULE_INFORMATION Modules[1];
} RTL_PROCESS_MODULES, * PRTL_PROCESS_MODULES;



#pragma alloc_text("PAGE",AttachReadVirtualMem)
#pragma alloc_text("PAGE",AttachWriteVirtualMem)
#pragma alloc_text("PAGE",CompleteRequest)
#pragma alloc_text("PAGE",DispatchCreate)
#pragma alloc_text("PAGE",DispatchClose)
#pragma alloc_text("PAGE",DispatchRead)
#pragma alloc_text("PAGE",DispatchWrite)
#pragma alloc_text("PAGE",DispatchDeviceControl)
#pragma alloc_text("INIT",DriverEntry)

