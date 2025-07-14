#include "driver.h"

ULONG g_TestValue = 0x11223344;
ULONG g_WindowsVersion = 0;

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
        KdPrint(("[XM] Version: WinXP\n"));
    }
    else if (versionInfo.dwMajorVersion == 6 && versionInfo.dwMinorVersion == 1) {
        g_WindowsVersion = Win7;
        KdPrint(("[XM] Version: Win7\n"));
    }
    else {
        KdPrint(("[XM] Version: Other\n"));
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
        KdPrint(("[XM] AttachReadVirtualMem PsLookupProcessByProcessId Status:%08X\n", Status));
        return Status;
    }
    KdPrint(("[XM] AttachReadVirtualMem PEPROCESS:%p\n", Process));

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    KeStackAttachProcess(Process, &ApcState);//切换CR3

    PhysicalAddress = MmGetPhysicalAddress(BaseAddress);
    KdPrint(("[XM] AttachReadVirtualMem PhysicalAddress: 0x%08X\n", PhysicalAddress.LowPart));
    
    PVOID lpMapBase = MmMapIoSpace(PhysicalAddress, ReadBytes, MmNonCached);
    if (lpMapBase != NULL) {
        KdPrint(("[XM] AttachReadVirtualMem MmMapIoSpace lpMapBase: %p\n", lpMapBase));
        RtlCopyMemory(Buffer, lpMapBase, ReadBytes);
        MmUnmapIoSpace(lpMapBase, ReadBytes);
        Status = STATUS_SUCCESS;
    } else {
        KdPrint(("[XM] AttachReadVirtualMem MmMapIoSpace Failed\n"));
        MmUnmapIoSpace(lpMapBase, ReadBytes);
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
        KdPrint(("[XM] AttachWriteVirtualMem PsLookupProcessByProcessId Status:%08X\n", Status));
        return Status;
    }
    KdPrint(("[XM] AttachWriteVirtualMem Process:%p\n", Process));

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    KeStackAttachProcess(Process, &ApcState);//切换CR3

    PhysicalAddress = MmGetPhysicalAddress(BaseAddress);

    //映射的物理地址为0 不读了 
    if (PhysicalAddress.QuadPart == 0) {
        KdPrint(("[XM] AttachReadVirtualMem VA: %p (maps to physical 0x0)\n", BaseAddress));
        KeUnstackDetachProcess(&ApcState);
        KeLowerIrql(OldIrql);
        ObDereferenceObject(Process);
        return STATUS_INVALID_ADDRESS;
    }

    KdPrint(("[XM] AttachWriteVirtualMem PhysicalAddress: 0x%08X\n", PhysicalAddress.LowPart));
    
    PVOID lpMapBase = MmMapIoSpace(PhysicalAddress, WriteBytes, MmCached);
    if (lpMapBase != NULL) {
        RtlCopyMemory(lpMapBase, Buffer, WriteBytes);
        MmUnmapIoSpace(lpMapBase, WriteBytes);
        Status = STATUS_SUCCESS;
        KdPrint(("[XM] AttachWriteVirtualMem MmMapIoSpace Success: %p\n", lpMapBase));
    } else {
        KdPrint(("[XM] AttachWriteVirtualMem MmMapIoSpace Failed\n"));
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

    KdPrint(("[XM] %s\n", __FUNCTION__));

    return CompleteRequest(Irp);
}

NTSTATUS DispatchClose(
    _In_ struct _DEVICE_OBJECT* DeviceObject, _Inout_ struct _IRP* Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    KdPrint(("[XM] %s\n", __FUNCTION__));


    return CompleteRequest(Irp);
}

NTSTATUS DispatchRead(
    _In_ struct _DEVICE_OBJECT* DeviceObject, _Inout_ struct _IRP* Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    KdPrint(("[XM] %s\n", __FUNCTION__));

    return CompleteRequest(Irp);
}

NTSTATUS DispatchWrite(
    _In_ struct _DEVICE_OBJECT* DeviceObject, _Inout_ struct _IRP* Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    KdPrint(("[XM] %s\n", __FUNCTION__));

    return CompleteRequest(Irp);
}

NTSTATUS DispatchDeviceControl(
    _In_ struct _DEVICE_OBJECT* DeviceObject, _Inout_ struct _IRP* Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    KdPrint(("[XM] %s\n", __FUNCTION__));

    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    ULONG_PTR info = 0;
    switch (code) {
    case CTL_ATTACH_MEM_READ:
    {
        PPROCESS_MEM_REQ memReq = (PPROCESS_MEM_REQ)Irp->AssociatedIrp.SystemBuffer;
        __try {
            KdPrint(("[XM] CTL_ATTACH_MEM_READ ProcessId:%p Address:%p Size:%d\n",
                    memReq->ProcessId, memReq->VirtualAddress, memReq->Size));
            HANDLE processId = memReq->ProcessId;
            PVOID VirtualAddress = memReq->VirtualAddress;
            unsigned Size = memReq->Size;

            status = AttachReadVirtualMem(processId, VirtualAddress,
                Irp->AssociatedIrp.SystemBuffer, Size);

            if (NT_SUCCESS(status)) {
                info = Size;
                KdPrint(("[XM] CTL_ATTACH_MEM_READ Success\n"));
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            status = STATUS_UNSUCCESSFUL;
            KdPrint(("[XM] CTL_ATTACH_MEM_READ exception\n"));
        }
    }
    break;
    case CTL_ATTACH_MEM_WRITE:
    {
        PPROCESS_MEM_REQ memReq = (PPROCESS_MEM_REQ)Irp->AssociatedIrp.SystemBuffer;
        __try {
            KdPrint(("[XM] CTL_ATTACH_MEM_WRITE ProcessId:%p Address:%p Size:%d\n",
                    memReq->ProcessId, memReq->VirtualAddress, memReq->Size));

            //要写的数据在请求头后
            PVOID writeData = (PUCHAR)Irp->AssociatedIrp.SystemBuffer + sizeof(PROCESS_MEM_REQ);
            status = AttachWriteVirtualMem(memReq->ProcessId, memReq->VirtualAddress,
                writeData, memReq->Size);

            if (NT_SUCCESS(status)) {
                info = sizeof(PROCESS_MEM_REQ);
                KdPrint(("[XM] CTL_ATTACH_MEM_WRITE Success\n"));
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            status = STATUS_UNSUCCESSFUL;
            KdPrint(("[XM] CTL_ATTACH_MEM_WRITE exception\n"));
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
            KdPrint(("[XM] CpuIndex: %d GdtBase: %08x GdtLimit: %08x, Size: %d\n",
                cpuIndex, gdtr.Base, gdtr.Limit, gdtSize));

            //KeSetSystemAffinityThread(cpuIndex);
            KAFFINITY affinity = 1UL << cpuIndex; // 将CPU索引转换为位掩码
            KeSetSystemAffinityThread(affinity);
            RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, (PVOID)gdtr.Base, gdtSize);

            info = gdtSize;
            status = STATUS_SUCCESS;
            KdPrint(("[XM] CTL_GET_GDT_DATA finish"));
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            status = STATUS_UNSUCCESSFUL;
            KdPrint(("[XM] CTL_GET_GDT_DATA exception\n"));
        }
    }
    break;
    case CTL_ENUM_PROCESS_COUNT:
    {
        ULONG processCount = 0;
        status = EnumProcessEx(NULL, true, &processCount); 
        if (NT_SUCCESS(status)) {
            *(PULONG)Irp->AssociatedIrp.SystemBuffer = processCount;
            info = sizeof(ULONG);
            KdPrint(("[XM] CTL_ENUM_PROCESS_COUNT: %d 个进程\n", processCount));
        }
    }
    break;
    case CTL_ENUM_PROCESS:
    {
        ULONG processCount = 0;
        status = EnumProcessEx((PPROCESS_INFO)Irp->AssociatedIrp.SystemBuffer,
            false, &processCount);
        if (NT_SUCCESS(status)) {
            info = processCount * sizeof(PROCESS_INFO);
            KdPrint(("[XM] CTL_ENUM_PROCESS: 返回 %d 个进程信息\n", processCount));
        }
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
            KdPrint(("[XM] Read exception at %p\n", (void*)req->Address));

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
            KdPrint(("[XM] WRITE exception at %p\n", (void*)req->Address));
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

    KdPrint(("[XM] Unload 完成\n"));

}

NTSTATUS DriverEntry(
    __in struct _DRIVER_OBJECT* DriverObject, __in PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    KdPrint(("[XM] g_TestValue address: %p\n", &g_TestValue));

    KdPrint(("[XM] DriverEntry DriverObject:%p RegistryPath:%wZ\n",
        DriverObject, RegistryPath));

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
            KdPrint(("[XM] Driver Entry IoCreateDevice ErrCode:%08x\n", status));
            break;
        }
        KdPrint(("[XM] Driver Entry IoCreateDevice Ok pDevObj:%p\n", pDevObj));

        // 设置设备标志
        pDevObj->Flags |= DO_BUFFERED_IO;          // 使用缓冲IO
        pDevObj->Flags &= ~DO_DEVICE_INITIALIZING; // 清除初始化标志

        //创建符号链接  通讯
        UNICODE_STRING usSymbolicLinkName;
        RtlInitUnicodeString(&usSymbolicLinkName, SYMBOL_NAME);

        status = IoCreateSymbolicLink(&usSymbolicLinkName, &usDeviceName);
        if (!NT_SUCCESS(status))
        {
            KdPrint(("[XM] IoCreateSymbolicLink ErrCode:%08x\n", status));
            IoDeleteDevice(pDevObj);
            break;
        }
        KdPrint(("[XM] IoCreateSymbolicLink Ok\n"));

    } while (false);

    // 初始化系统版本和进程偏移量
    DetectWindowsVersion();
    InitProcessOffsets();
    
    return status;
}
