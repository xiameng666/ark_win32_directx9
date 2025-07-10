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
#include <map>
#include <functional>

#define COLOR_RED       ImVec4(1.0f, 0.0f, 0.0f, 1.0f)    // 红色
#define COLOR_GREEN     ImVec4(0.0f, 1.0f, 0.0f, 1.0f)    // 绿色
#define COLOR_YELLOW    ImVec4(1.0f, 1.0f, 0.0f, 1.0f)    // 黄色
#define COLOR_GRAY      ImVec4(0.8f, 0.8f, 0.8f, 1.0f)    // 灰色
#define COLOR_LIGHT_GRAY ImVec4(0.5f, 0.5f, 0.5f, 1.0f)   // 浅灰
#define COLOR_WHITE     ImVec4(1.0f, 1.0f, 1.0f, 1.0f)    // 白色
#define COLOR_CYAN      ImVec4(0.0f, 0.8f, 1.0f, 1.0f)    // 青色
#define COLOR_ORANGE    ImVec4(1.0f, 0.6f, 0.0f, 1.0f)    // 橙色
#define COLOR_PURPLE    ImVec4(1.0f, 0.0f, 1.0f, 1.0f)    // 紫色
#define COLOR_INFO      ImVec4(0.7f, 0.7f, 0.7f, 1.0f)    // 信息灰色

// 单个视图
#define VIEW_ITEM(text, view) \
    if (ImGui::Selectable(text, ctx_->currentView == view)) { \
        ctx_->currentView = view; \
    }

// 导航栏框架
#define NAV_SECTION(group_name, ...) \
    if (ImGui::TreeNodeEx(group_name, ImGuiTreeNodeFlags_DefaultOpen)) { \
        __VA_ARGS__ \
        ImGui::TreePop(); \
    }

enum SubView {
    GDT,
    IDT
};


struct Context {
    bool show_process_wnd = true;
    bool show_module_wnd = true;
    bool show_regedit_wnd = true;
    bool show_menu_bar = true;
    bool show_file_wnd = true;
    bool show_net_wnd = true;
    bool show_log_wnd = true;
    bool show_kernel_wnd = true;

    ArkR3 arkR3;
    SubView currentView;
};



class ImguiWnd {
protected:
    Context* ctx_;  
    std::map<SubView, std::function<void()>> viewRenderers_;

public:
    explicit ImguiWnd(Context* ctx) : ctx_(ctx) {}
    virtual void Render(bool* p_open = nullptr) = 0;
    virtual ~ImguiWnd() = default;
};

