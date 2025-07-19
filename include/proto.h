#pragma once


#define DEVICE_NAME L"\\Device\\ADriver1"
#define SYMBOL_NAME L"\\DosDevices\\ADriver1"
#define DOS_NAME     L"\\\\\.\\ADriver1"

enum WindowsVersion
{
    WinXP,
    Win7,
    Other
};

#define MY_CTL_CODE(code)           CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800 + code, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define CTL_READ_MEM                MY_CTL_CODE(0)
#define CTL_WRITE_MEM               MY_CTL_CODE(1)


#define CTL_ATTACH_MEM_READ         MY_CTL_CODE(2)      //附加进程读
#define CTL_ATTACH_MEM_WRITE        MY_CTL_CODE(3)      //写


#define CTL_GET_GDT_DATA            MY_CTL_CODE(10)    //获取GDT表数据

#define CTL_ENUM_PROCESS_COUNT      MY_CTL_CODE(20)   // 枚举进程 返回数量
#define CTL_ENUM_PROCESS            MY_CTL_CODE(21)   // 返回数据
#define CTL_KILL_PROCESS            MY_CTL_CODE(22)   // 终止进程

#define CTL_ENUM_MODULE_COUNT       MY_CTL_CODE(30)   // 枚举模块 返回数量
#define CTL_ENUM_MODULE             MY_CTL_CODE(31)   // 枚举模块 返回数据

#define CTL_ENUM_PROCESS_MODULE_COUNT MY_CTL_CODE(32)   // 枚举进程模块 返回数量
#define CTL_ENUM_PROCESS_MODULE       MY_CTL_CODE(33)   // 枚举进程模块 返回数据

#define CTL_ENUM_SSDT               MY_CTL_CODE(35)  // 枚举SSDT 返回数据  假定max 500条记录 不要返回数量了

//#define CTL_ENUM_DRIVER_COUNT       MY_CTL_CODE(40)
//#define CTL_ENUM_DRIVER             MY_CTL_CODE(41)   // 枚举驱动
//
//#define CTL_ENUM_THREAD_COUNT       MY_CTL_CODE(50)
//#define CTL_ENUM_THREAD             MY_CTL_CODE(51)   // 枚举线程
//
//#define CTL_ENUM_HANDLE_COUNT       MY_CTL_CODE(60)
//#define CTL_ENUM_HANDLE             MY_CTL_CODE(61)   // 枚举句柄
//#define CTL_QUERY_SYSINFO           MY_CTL_CODE(62)   // 查询系统信息
//
//#define CTL_ENUM_REGISTRY_COUNT     MY_CTL_CODE(70)
//#define CTL_ENUM_REGISTRY           MY_CTL_CODE(71)   // 枚举注册表项
//#define CTL_READ_REGISTRY           MY_CTL_CODE(72)  // 读取注册表
//#define CTL_WRITE_REGISTRY          MY_CTL_CODE(73)  // 写注册表
//
//#define CTL_ENUM_CALLBACK_COUNT     MY_CTL_CODE(80)
//#define CTL_ENUM_CALLBACK           MY_CTL_CODE(81)  // 枚举内核回调
//
//#define CTL_ENUM_HOOK_COUNT         MY_CTL_CODE(90)
//#define CTL_ENUM_HOOK               MY_CTL_CODE(91)  // 



typedef struct KERNEL_RW_REQ {
    unsigned Address;
    unsigned Size;
    unsigned char Buffer[256];
}*PKERNEL_RW_REQ;

//读写目标进程内存
typedef struct PROCESS_MEM_REQ {
    HANDLE ProcessId;        // 目标进程ID
    PVOID VirtualAddress;    // 虚拟地址
    unsigned Size;           // 数据大小  
                             //写到systembuffer 不需要在结构体定义缓冲区
}*PPROCESS_MEM_REQ;

#pragma pack(push, 1)
typedef struct GDTR {
  unsigned short Limit;
  unsigned int   Base;
}*PGDTR;
#pragma pack(pop)

typedef struct GDT_DATA_REQ {
    unsigned CpuIndex;
    GDTR Gdtr;
}*PGDT_DATA_REQ;

typedef struct PROCESS_INFO {
    ULONG ProcessId;                    // 进程ID
    ULONG ParentProcessId;              // 父进程ID
    CHAR ImageFileName[16];             // 进程名称（短名）
    PVOID EprocessAddr;                 // EPROCESS地址
    ULONG DirectoryTableBase;           // CR3页目录基地址  

}*PPROCESS_INFO;

typedef struct MODULE_INFO {
    CHAR Name[64];                      // 模块名称（短名）
    CHAR FullPath[256];                 // 模块完整路径
    PVOID ImageBase;                    // 模块基地址
    ULONG ImageSize;                    // 模块大小
    USHORT LoadOrderIndex;              // 加载顺序索引
    USHORT LoadCount;                   // 加载计数

    //CHAR Company[128];                // 厂商信息（暂时为空，需要解析PE）
}*PMODULE_INFO;

typedef struct PROCESS_MODULE_REQ {
    HANDLE ProcessId;                   // 目标进程ID
    ULONG ModuleCount;                  // 模块数量（输出参数）
}*PPROCESS_MODULE_REQ;

typedef struct SegmentDescriptor {
    // ========== 历史字段 (来自80286) ==========
    unsigned Limit1 : 16;    // 界限低16位 - 从80286继承
    unsigned Base1 : 16;     // 基址低16位 - 从80286继承  
    unsigned Base2 : 8;      // 基址中8位 - 从80286继承
    unsigned type : 4;       // 段类型 - 从80286继承，但扩展了
    unsigned s : 1;          // 系统段标志 - 从80286继承
    unsigned dpl : 2;        // 特权级 - 从80286继承
    unsigned p : 1;          // 存在位 - 从80286继承

    // ========== 80386新增字段 ==========
    unsigned Limit2 : 4;     // 界限高4位 - 新增，支持32位界限
    unsigned avl : 1;        // 软件可用位 - 新增
    unsigned res : 1;        // 保留位 - 新增，为未来扩展
    unsigned db : 1;         // 操作数大小 - 新增，支持16/32位切换
    unsigned g : 1;          // 粒度位 - 新增，突破界限限制
    unsigned Base3 : 8;      // 基址高8位 - 新增，支持32位基址
}*PSEGDESC;

typedef struct SSDT_INFO {
    ULONG Index;
    PVOID FunctionAddress;
    CHAR FunctionName[64];
}*PSSDT_INFO;
