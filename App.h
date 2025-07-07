#pragma once
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <d3d9.h>
#include <tchar.h>
#include "ArkR3.h"
#include <vector>

struct Context {
    bool show_process_wnd = true;
    bool show_module_wnd = true;
    bool show_kernel_wnd = true;
    bool show_regedit_wnd = true;
    bool show_menu_bar = true;
    bool show_file_wnd = true;
    bool show_net_wnd = true;

    std::vector<PROCESS_INFO> list;
};

class App {
public:
    void Render();
    void SetDockingWnd(bool* p_open);

    void RenderProcessWnd();
    void RenderModuleWnd();
    void RenderKernelWnd();
    void RenderRegeditWnd();
    void RenderFileWnd();
    void RenderNetWnd();
    void RefreshProcessList();
    void RenderMenuBar();

    ArkR3* arkR3;
    Context Ctx;

};


