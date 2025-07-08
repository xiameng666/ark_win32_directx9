#pragma once
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <GL/GL.h>
#include <tchar.h>

#include <vector>
#include "../include/proto.h"
#include <string>
#include "ArkR3.h"
struct Context {
    bool show_process_wnd = true;
    bool show_module_wnd = true;
    bool show_kernel_wnd = true;
    bool show_regedit_wnd = true;
    bool show_menu_bar = true;
    bool show_file_wnd = true;
    bool show_net_wnd = true;
    bool show_log_wnd = true;

    ArkR3 arkR3;
    std::vector<PROCESS_INFO> list;

};

class ImguiWnd {
public:
    virtual void Render(bool* p_open = nullptr) = 0;
};

