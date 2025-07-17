#include "driver.h"

ULONG g_TestValue = 0x11223344;
ULONG g_WindowsVersion = 0;
BOOLEAN g_LogOn = FALSE;  // 日志开关

extern "C"
NTSTATUS NTAPI ZwQuerySystemInformation(
    __in SYSTEM_INFORMATION_CLASS SystemInformationClass,
    __out_bcount_opt(SystemInformationLength) PVOID SystemInformation,
    __in ULONG SystemInformationLength,
    __out_opt PULONG ReturnLength
);

NTSTATUS EnumModuleEx(PMODULE_INFO ModuleBuffer, bool CountOnly, PULONG ModuleCount)
{
    NTSTATUS Status;
    PRTL_PROCESS_MODULES Modules;
    PRTL_PROCESS_MODULE_INFORMATION ModuleInfo;
    PVOID Buffer;
    ULONG BufferSize = 4096;
    ULONG ReturnLength;
    ULONG i;
    ULONG Count = 0;

    Log("[XM] EnumModuleEx: CountOnly=%d\n", CountOnly);

retry:
    Buffer = ExAllocatePoolWithTag(NonPagedPool, BufferSize, 'MDLE');
    if (!Buffer) {
        return STATUS_NO_MEMORY;
    }

    Status = ZwQuerySystemInformation(SystemModuleInformation,
        Buffer,
        BufferSize,
        &ReturnLength
    );

    if (Status == STATUS_INFO_LENGTH_MISMATCH) {
        ExFreePool(Buffer);
        BufferSize = ReturnLength;
        goto retry;
    }

    if (!NT_SUCCESS(Status)) {
        ExFreePool(Buffer);
        return Status;
    }

    Modules = (PRTL_PROCESS_MODULES)Buffer;
    Count = Modules->NumberOfModules;

    if (CountOnly) {
        *ModuleCount = Count;
        ExFreePool(Buffer);
        return STATUS_SUCCESS;
    }

    // 填充模块信息
    for (i = 0, ModuleInfo = &(Modules->Modules[0]);
        i < Modules->NumberOfModules && ModuleBuffer;
        i++, ModuleInfo++) {
        
        __try {
            // 清空结构体
            RtlZeroMemory(&ModuleBuffer[i], sizeof(MODULE_INFO));
            
            // 提取模块名称（从完整路径中提取文件名）
            CHAR* fileName = (CHAR*)ModuleInfo->FullPathName + ModuleInfo->OffsetToFileName;
            RtlCopyMemory(ModuleBuffer[i].Name, fileName, 
                min(strlen(fileName), sizeof(ModuleBuffer[i].Name) - 1));
            
            // 完整路径
            RtlCopyMemory(ModuleBuffer[i].FullPath, ModuleInfo->FullPathName,
                min(strlen((CHAR*)ModuleInfo->FullPathName), sizeof(ModuleBuffer[i].FullPath) - 1));
            
            // 基地址和大小
            ModuleBuffer[i].ImageBase = ModuleInfo->ImageBase;
            ModuleBuffer[i].ImageSize = ModuleInfo->ImageSize;
            ModuleBuffer[i].LoadOrderIndex = ModuleInfo->LoadOrderIndex;
            ModuleBuffer[i].LoadCount = ModuleInfo->LoadCount;
            
            
            Log("[XM] Module[%d]: Name=%s, Base=%p, Size=0x%X\n",
                i, ModuleBuffer[i].Name, ModuleBuffer[i].ImageBase, ModuleBuffer[i].ImageSize);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Log("[XM] EnumProcessModuleEx: Exception processing module %d\n", i);
            continue;
        }
    }

    *ModuleCount = Count;
    ExFreePool(Buffer);
    return STATUS_SUCCESS;
}

void Log(const char* Format, ...)
{
    if (!g_LogOn) {
        return;
    }

    char buffer[512];
    va_list args;
    va_start(args, Format);

    RtlStringCbVPrintfA(buffer, sizeof(buffer), Format, args);
    va_end(args);

    DbgPrint("%s", buffer);
}

NTSTATUS DetectWindowsVersion()
{
    RTL_OSVERSIONINFOW versionInfo = { 0 };
    versionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOW);
    NTSTATUS status = RtlGetVersion(&versionInfo);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (versionInfo.dwMajorVersion == 5) {
        g_WindowsVersion = WinXP;
        Log(("[XM] Version: WinXP\n"));
    }
    else if (versionInfo.dwMajorVersion == 6 && versionInfo.dwMinorVersion == 1) {
        g_WindowsVersion = Win7;
        Log(("[XM] Version: Win7\n"));
    }
    else {
        Log(("[XM] Version: Other\n"));
        return STATUS_NOT_SUPPORTED;
    }

    return STATUS_SUCCESS;
}

NTSTATUS AttachReadVirtualMem(HANDLE ProcessId, PVOID BaseAddress, PVOID Buffer, unsigned ReadBytes)
{
    PEPROCESS Process = NULL;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    KAPC_STATE ApcState = { 0 };
    PHYSICAL_ADDRESS PhysicalAddress = { 0 };
    KIRQL  OldIrql = 0;

    Status = PsLookupProcessByProcessId(ProcessId, &Process);
    if (!NT_SUCCESS(Status)) {
        Log("[XM] AttachReadVirtualMem PsLookupProcessByProcessId Status:%08X\n", Status);
        return Status;
    }
    Log("[XM] AttachReadVirtualMem PEPROCESS:%p\n", Process);

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    KeStackAttachProcess(Process, &ApcState);

    PhysicalAddress = MmGetPhysicalAddress(BaseAddress);
    Log("[XM] AttachReadVirtualMem PhysicalAddress: 0x%08X\n", PhysicalAddress.LowPart);
    
    PVOID lpMapBase = MmMapIoSpace(PhysicalAddress, ReadBytes, MmNonCached);
    if (lpMapBase != NULL) {
        Log("[XM] AttachReadVirtualMem MmMapIoSpace lpMapBase: %p\n", lpMapBase);
        RtlCopyMemory(Buffer, lpMapBase, ReadBytes);
        MmUnmapIoSpace(lpMapBase, ReadBytes);
        Status = STATUS_SUCCESS;
    } else {
        Log("[XM] AttachReadVirtualMem MmMapIoSpace Failed\n");
        return Status;
    }

    KeUnstackDetachProcess(&ApcState);
    KeLowerIrql(OldIrql);

    if (Process) {
        ObDereferenceObject(Process);
    }
    
    return Status;
}

NTSTATUS AttachWriteVirtualMem(HANDLE ProcessId, PVOID BaseAddress, PVOID Buffer, unsigned WriteBytes)
{
    PEPROCESS Process = NULL;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    KAPC_STATE ApcState = { 0 };
    PHYSICAL_ADDRESS PhysicalAddress = { 0 };
    KIRQL  OldIrql = 0;

    Status = PsLookupProcessByProcessId(ProcessId, &Process);
    if (!NT_SUCCESS(Status)) {
        Log("[XM] AttachWriteVirtualMem PsLookupProcessByProcessId Status:%08X\n", Status);
        return Status;
    }
    Log("[XM] AttachWriteVirtualMem Process:%p\n", Process);

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    KeStackAttachProcess(Process, &ApcState);

    PhysicalAddress = MmGetPhysicalAddress(BaseAddress);

    if (PhysicalAddress.QuadPart == 0) {
        Log("[XM] AttachWriteVirtualMem VA: %p (maps to physical 0x0)\n", BaseAddress);
        KeUnstackDetachProcess(&ApcState);
        KeLowerIrql(OldIrql);
        ObDereferenceObject(Process);
        return STATUS_INVALID_ADDRESS;
    }

    Log("[XM] AttachWriteVirtualMem PhysicalAddress: 0x%08X\n", PhysicalAddress.LowPart);
    
    PVOID lpMapBase = MmMapIoSpace(PhysicalAddress, WriteBytes, MmCached);
    if (lpMapBase != NULL) {
        RtlCopyMemory(lpMapBase, Buffer, WriteBytes);
        MmUnmapIoSpace(lpMapBase, WriteBytes);
        Status = STATUS_SUCCESS;
        Log("[XM] AttachWriteVirtualMem MmMapIoSpace Success: %p\n", lpMapBase);
    } else {
        Log("[XM] AttachWriteVirtualMem MmMapIoSpace Failed\n");
        MmUnmapIoSpace(lpMapBase, WriteBytes);
        return Status;
    }

    KeUnstackDetachProcess(&ApcState);
    KeLowerIrql(OldIrql);

    if (Process) {
        ObDereferenceObject(Process);
    }
    
    return Status;
}

NTSTATUS CompleteRequest(struct _IRP* Irp, ULONG_PTR Information, NTSTATUS Status) {
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Information;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS DispatchCreate(
    _In_ struct _DEVICE_OBJECT* DeviceObject, _Inout_ struct _IRP* Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Log("[XM] %s\n", __FUNCTION__);

    return CompleteRequest(Irp);
}

NTSTATUS DispatchClose(
    _In_ struct _DEVICE_OBJECT* DeviceObject, _Inout_ struct _IRP* Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Log("[XM] %s\n", __FUNCTION__);


    return CompleteRequest(Irp);
}

NTSTATUS DispatchRead(
    _In_ struct _DEVICE_OBJECT* DeviceObject, _Inout_ struct _IRP* Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Log("[XM] %s\n", __FUNCTION__);

    return CompleteRequest(Irp);
}

NTSTATUS DispatchWrite(
    _In_ struct _DEVICE_OBJECT* DeviceObject, _Inout_ struct _IRP* Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Log("[XM] %s\n", __FUNCTION__);

    return CompleteRequest(Irp);
}

NTSTATUS DispatchDeviceControl(
    _In_ struct _DEVICE_OBJECT* DeviceObject, _Inout_ struct _IRP* Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    Log("[XM] %s\n", __FUNCTION__);

    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    ULONG_PTR info = 0;
    switch (code) {
    case CTL_ATTACH_MEM_READ:
    {
        PPROCESS_MEM_REQ memReq = (PPROCESS_MEM_REQ)Irp->AssociatedIrp.SystemBuffer;
        __try {
            Log("[XM] CTL_ATTACH_MEM_READ ProcessId:%p Address:%p Size:%d\n",
                    memReq->ProcessId, memReq->VirtualAddress, memReq->Size);
            HANDLE processId = memReq->ProcessId;
            PVOID VirtualAddress = memReq->VirtualAddress;
            unsigned Size = memReq->Size;

            status = AttachReadVirtualMem(processId, VirtualAddress,
                Irp->AssociatedIrp.SystemBuffer, Size);

            if (NT_SUCCESS(status)) {
                info = Size;
                Log("[XM] CTL_ATTACH_MEM_READ Success\n");
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            status = STATUS_UNSUCCESSFUL;
            Log("[XM] CTL_ATTACH_MEM_READ exception\n");
        }
    }
    break;
    case CTL_ATTACH_MEM_WRITE:
    {
        PPROCESS_MEM_REQ memReq = (PPROCESS_MEM_REQ)Irp->AssociatedIrp.SystemBuffer;
        __try {
            Log("[XM] CTL_ATTACH_MEM_WRITE ProcessId:%p Address:%p Size:%d\n",
                    memReq->ProcessId, memReq->VirtualAddress, memReq->Size);

            //要写的数据在请求头后
            PVOID writeData = (PUCHAR)Irp->AssociatedIrp.SystemBuffer + sizeof(PROCESS_MEM_REQ);
            status = AttachWriteVirtualMem(memReq->ProcessId, memReq->VirtualAddress,
                writeData, memReq->Size);

            if (NT_SUCCESS(status)) {
                info = sizeof(PROCESS_MEM_REQ);
                Log("[XM] CTL_ATTACH_MEM_WRITE Success\n");
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            status = STATUS_UNSUCCESSFUL;
            Log("[XM] CTL_ATTACH_MEM_WRITE exception\n");
        }
    }
    break;
    case CTL_GET_GDT_DATA:
    {
        PGDT_DATA_REQ gdtReq = (PGDT_DATA_REQ)Irp->AssociatedIrp.SystemBuffer;
        __try {
            ULONG cpuIndex = gdtReq->CpuIndex;
            GDTR gdtr = gdtReq->Gdtr;
            ULONG gdtSize = gdtr.Limit + 1;
            Log("[XM] CpuIndex: %d GdtBase: %08x GdtLimit: %08x, Size: %d\n",
                cpuIndex, gdtr.Base, gdtr.Limit, gdtSize);

            //KeSetSystemAffinityThread(cpuIndex);
            KAFFINITY affinity = 1UL << cpuIndex; // 将CPU索引转换为位掩码
            KeSetSystemAffinityThread(affinity);
            RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, (PVOID)gdtr.Base, gdtSize);

            info = gdtSize;
            status = STATUS_SUCCESS;
            Log("[XM] CTL_GET_GDT_DATA finish");
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            status = STATUS_UNSUCCESSFUL;
            Log("[XM] CTL_GET_GDT_DATA exception\n");
        }
    }
    break;
    case CTL_ENUM_PROCESS_COUNT:
    {
        __try {
            ULONG processCount = 0;
            status = EnumProcessEx(NULL, true, &processCount); 
            if (NT_SUCCESS(status)) {
                *(PULONG)Irp->AssociatedIrp.SystemBuffer = processCount;
                info = sizeof(ULONG);
                Log("[XM] CTL_ENUM_PROCESS_COUNT: %d 个进程\n", processCount);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            status = STATUS_UNSUCCESSFUL;
            Log("[XM] CTL_ENUM_PROCESS_COUNT exception\n");
        }
    }
    break;
    case CTL_ENUM_PROCESS:
    {
        __try {
            ULONG processCount = 0;
            status = EnumProcessEx((PPROCESS_INFO)Irp->AssociatedIrp.SystemBuffer,
                false, &processCount);
            if (NT_SUCCESS(status)) {
                info = processCount * sizeof(PROCESS_INFO);
                Log("[XM] CTL_ENUM_PROCESS: 返回 %d 个进程信息\n", processCount);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            status = STATUS_UNSUCCESSFUL;
            Log("[XM] CTL_ENUM_PROCESS exception\n");
        }
    }
    break;
    case CTL_ENUM_MODULE_COUNT:
    {
        __try {
            ULONG moduleCount = 0;
            status = EnumModuleEx(NULL, TRUE, &moduleCount);
            if (NT_SUCCESS(status)) {
                *(PULONG)Irp->AssociatedIrp.SystemBuffer = moduleCount;
                info = sizeof(ULONG);
                Log("[XM] CTL_ENUM_MODULE_COUNT: %d 个模块\n", moduleCount);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            status = STATUS_UNSUCCESSFUL;
            Log("[XM] CTL_ENUM_MODULE_COUNT exception\n");
        }
    }
    break;
    case CTL_ENUM_MODULE:
    {
        __try {
            ULONG moduleCount = 0;
            status = EnumModuleEx((PMODULE_INFO)Irp->AssociatedIrp.SystemBuffer,
                FALSE, &moduleCount);
            if (NT_SUCCESS(status)) {
                info = moduleCount * sizeof(MODULE_INFO);
                Log("[XM] CTL_ENUM_MODULE: 返回 %d 个模块信息\n", moduleCount);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            status = STATUS_UNSUCCESSFUL;
            Log("[XM] CTL_ENUM_MODULE exception\n");
        }
    }
    break;
    
    case CTL_ENUM_PROCESS_MODULE_COUNT:
    {
        //PPROCESS_MODULE_REQ req = (PPROCESS_MODULE_REQ)Irp->AssociatedIrp.SystemBuffer;
        //ULONG moduleCount = 0;
        //status = EnumProcessModuleEx(req->ProcessId, NULL, true, &moduleCount);
        //if (NT_SUCCESS(status)) {
        //    *(PULONG)Irp->AssociatedIrp.SystemBuffer = moduleCount;
        //    info = sizeof(ULONG);
        //    Log(("[XM] CTL_ENUM_PROCESS_MODULE_COUNT: 进程 %p 有 %d 个模块\n", 
        //        req->ProcessId, moduleCount));
        //}
    }
    break;
    
    case CTL_ENUM_PROCESS_MODULE:
    {
        //PPROCESS_MODULE_REQ req = (PPROCESS_MODULE_REQ)Irp->AssociatedIrp.SystemBuffer;
        //ULONG moduleCount = req->ModuleCount;
        //status = EnumProcessModuleEx(req->ProcessId, (PMODULE_INFO)Irp->AssociatedIrp.SystemBuffer,
        //    false, &moduleCount);
        //if (NT_SUCCESS(status)) {
        //    info = moduleCount * sizeof(MODULE_INFO);
        //    Log(("[XM] CTL_ENUM_PROCESS_MODULE: 进程 %p 返回 %d 个模块信息\n", 
        //        req->ProcessId, moduleCount));
        //}
    }
    break;

    case CTL_READ_MEM:
    {
        PKERNEL_RW_REQ req = (PKERNEL_RW_REQ)Irp->AssociatedIrp.SystemBuffer;
        __try {
            RtlCopyMemory(req->Buffer, (const void*)req->Address, req->Size);
            info = sizeof(KERNEL_RW_REQ);
            status = STATUS_SUCCESS;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            status = STATUS_UNSUCCESSFUL;
            Log("[XM] Read exception at %p\n", (void*)req->Address);
        }
    }
    break;

    case CTL_WRITE_MEM:
    {
        PKERNEL_RW_REQ req = (PKERNEL_RW_REQ)Irp->AssociatedIrp.SystemBuffer;
        __try {
            RtlCopyMemory((void*)req->Address, req->Buffer, req->Size);
            info = sizeof(KERNEL_RW_REQ);
            status = STATUS_SUCCESS;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            status = STATUS_UNSUCCESSFUL;
            Log("[XM] WRITE exception at %p\n", (void*)req->Address);
        }

    }
    break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = info;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

VOID Unload(__in struct _DRIVER_OBJECT* DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNICODE_STRING usSymbolicLinkName;

    // 删除符号链接
    RtlInitUnicodeString(&usSymbolicLinkName, SYMBOL_NAME);
    IoDeleteSymbolicLink(&usSymbolicLinkName);

    // 删除设备对象
    if (DriverObject->DeviceObject != nullptr) {
        IoDeleteDevice(DriverObject->DeviceObject);
    }

    Log("[XM] Unload 完成\n");
}

NTSTATUS DriverEntry(
    __in struct _DRIVER_OBJECT* DriverObject, __in PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    Log("[XM] g_TestValue address: %p\n", &g_TestValue);

    Log("[XM] DriverEntry DriverObject:%p RegistryPath:%wZ\n",
        DriverObject, RegistryPath);

    //设置unload
    DriverObject->DriverUnload = Unload;

    //设置分发例程 回调函数跟硬件设备通讯 
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
    DriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = DispatchWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDeviceControl;

    // 创建设备对象
    PDEVICE_OBJECT pDevObj = NULL;
    UNICODE_STRING usDeviceName;
    RtlInitUnicodeString(&usDeviceName, DEVICE_NAME);
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    do {
        status = IoCreateDevice(
            DriverObject,           // 驱动对象
            0,                      // 设备扩展大小
            &usDeviceName,          // 设备名称
            FILE_DEVICE_UNKNOWN,    // 设备类型
            FILE_DEVICE_SECURE_OPEN,// 设备特性  权限
            FALSE,                  // 独占访问 
            &pDevObj               // 返回的设备对象 
        );

        if (!NT_SUCCESS(status))
        {
            Log("[XM] Driver Entry IoCreateDevice ErrCode:%08x\n", status);
            break;
        }
        Log("[XM] Driver Entry IoCreateDevice Ok pDevObj:%p\n", pDevObj);

        // 设置设备标志
        pDevObj->Flags |= DO_BUFFERED_IO;          // 使用缓冲IO
        pDevObj->Flags &= ~DO_DEVICE_INITIALIZING; // 清除初始化标志

        //创建符号链接  通讯
        UNICODE_STRING usSymbolicLinkName;
        RtlInitUnicodeString(&usSymbolicLinkName, SYMBOL_NAME);

        status = IoCreateSymbolicLink(&usSymbolicLinkName, &usDeviceName);
        if (!NT_SUCCESS(status))
        {
            Log("[XM] IoCreateSymbolicLink ErrCode:%08x\n", status);
            IoDeleteDevice(pDevObj);
            break;
        }
        Log("[XM] IoCreateSymbolicLink Ok\n");

    } while (false);

    // 初始化系统版本和进程偏移量
    DetectWindowsVersion();
    InitProcessOffsets();
    
    return status;
}

//// 枚举指定进程的模块
//NTSTATUS EnumProcessModuleEx(HANDLE ProcessId, PMODULE_INFO ModuleBuffer, bool CountOnly, PULONG ModuleCount)
//{
//    NTSTATUS Status;
//    PEPROCESS Process;
//    KAPC_STATE ApcState;
//    PVOID BaseAddress = NULL;
//    MEMORY_BASIC_INFORMATION MemInfo;
//    SIZE_T ReturnLength;
//    ULONG Count = 0;
//    
//    Log(("[XM] EnumProcessModuleEx: ProcessId=%p, CountOnly=%d\n", ProcessId, CountOnly));
//    
//    // 获取进程对象
//    Status = PsLookupProcessByProcessId(ProcessId, &Process);
//    if (!NT_SUCCESS(Status)) {
//        Log(("[XM] EnumProcessModuleEx: PsLookupProcessByProcessId failed: 0x%X\n", Status));
//        return Status;
//    }
//    
//    KeStackAttachProcess(Process, &ApcState);
//    
//    __try {
//        // 遍历进程的虚拟地址空间
//        while (BaseAddress < (PVOID)0x7FFFFFFF) {  // 用户模式地址空间上限
//            Status = ZwQueryVirtualMemory(
//                ZwCurrentProcess(),  // 当前进程句柄（已附加到目标进程）
//                BaseAddress,
//                MemoryBasicInformation,
//                &MemInfo,
//                sizeof(MemInfo),
//                &ReturnLength
//            );
//            
//            if (!NT_SUCCESS(Status)) {
//                break;
//            }
//            
//            // 检查是否为映像文件（DLL/EXE）
//            if (MemInfo.Type == MEM_IMAGE && MemInfo.State == MEM_COMMIT) {
//                if (!CountOnly && ModuleBuffer && Count < *ModuleCount) {
//                    // 填充模块信息
//                    RtlZeroMemory(&ModuleBuffer[Count], sizeof(MODULE_INFO));
//                    
//                    // 设置基地址和大小
//                    ModuleBuffer[Count].ImageBase = MemInfo.AllocationBase;
//                    ModuleBuffer[Count].ImageSize = (ULONG)MemInfo.RegionSize;
//                    
//                    // 模块名称（使用内核安全的字符串函数）
//                    RtlStringCbPrintfA(ModuleBuffer[Count].Name, sizeof(ModuleBuffer[Count].Name), 
//                                      "Module_%p", MemInfo.AllocationBase);
//                    RtlStringCbPrintfA(ModuleBuffer[Count].FullPath, sizeof(ModuleBuffer[Count].FullPath), 
//                                      "Unknown_Path_%p", MemInfo.AllocationBase);
//                    
//                    ModuleBuffer[Count].LoadOrderIndex = (USHORT)Count;
//                    ModuleBuffer[Count].LoadCount = 1;
//                    
//                    Log(("[XM] ProcessModule[%d]: Base=%p, Size=0x%X\n",
//                        Count, ModuleBuffer[Count].ImageBase, ModuleBuffer[Count].ImageSize));
//                }
//                Count++;
//            }
//            
//            // 移动到下一个内存区域
//            BaseAddress = (PVOID)((ULONG_PTR)MemInfo.BaseAddress + MemInfo.RegionSize);
//        }
//    }
//    __except (EXCEPTION_EXECUTE_HANDLER) {
//        Log(("[XM] EnumProcessModuleEx: Exception occurred\n"));
//        Status = STATUS_UNSUCCESSFUL;
//    }
//    
//    KeUnstackDetachProcess(&ApcState);
//
//    ObDereferenceObject(Process);
//    
//    *ModuleCount = Count;
//    
//    Log(("[XM] EnumProcessModuleEx: Found %d modules\n", Count));
//    return STATUS_SUCCESS;
//}






