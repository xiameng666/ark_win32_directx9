#pragma once
#include <ntifs.h>
#include <ntddk.h>
#include "../include/proto.h"
#include "process.h"

extern "C" {
    NTSTATUS DetectWindowsVersion();

    NTSTATUS AttachReadVirtualMem(HANDLE ProcessId, PVOID BaseAddress, PVOID Buffer, unsigned ReadBytes);

    NTSTATUS AttachWriteVirtualMem(HANDLE ProcessId, PVOID BaseAddress, PVOID Buffer, unsigned WriteBytes);

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

#pragma alloc_text("PAGE",AttachReadVirtualMem)
#pragma alloc_text("PAGE",AttachWriteVirtualMem)
#pragma alloc_text("PAGE",CompleteRequest)
#pragma alloc_text("PAGE",DispatchCreate)
#pragma alloc_text("PAGE",DispatchClose)
#pragma alloc_text("PAGE",DispatchRead)
#pragma alloc_text("PAGE",DispatchWrite)
#pragma alloc_text("PAGE",DispatchDeviceControl)
#pragma alloc_text("INIT",DriverEntry)

