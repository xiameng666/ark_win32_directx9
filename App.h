#pragma once
#include"Interface.h"

#include "ArkR3.h"

#include "ProcessWnd.h"
#include "ModuleWnd.h"
#include "KernelWnd.h"
#include "RegeditWnd.h"
#include "FileWnd.h"
#include "NetWnd.h"

struct Context {
    bool show_process_wnd = true;
    bool show_module_wnd = true;
    bool show_kernel_wnd = true;
    bool show_regedit_wnd = true;
    bool show_menu_bar = true;
    bool show_file_wnd = true;
    bool show_net_wnd = true;


};

class App {
public:
    App::App()
        : processWnd()
        , moduleWnd()
        , kernelWnd()
        , regeditWnd()
        , fileWnd()
        , netWnd()
    {}

    void Render();
    void SetDockingWnd(bool* p_open);

    void RenderProcessWnd();
    void RenderModuleWnd();
    void RenderKernelWnd();
    void RenderRegeditWnd();
    void RenderFileWnd();
    void RenderNetWnd();
    void test();
    void RenderMenuBar();

    ArkR3 arkR3;
    Context Ctx;

    ProcessWnd processWnd;
    ModuleWnd moduleWnd;
    KernelWnd kernelWnd;
    RegeditWnd regeditWnd;
    FileWnd fileWnd;
    NetWnd netWnd;
};


