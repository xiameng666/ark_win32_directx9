#include "App.h"

void App::Render()
{

    SetDockingWnd(0);

    if (ctx_.show_menu_bar)
        menuBar_.Render(&ctx_.show_menu_bar);
    if (ctx_.show_process_wnd)
        processWnd_.Render(&ctx_.show_process_wnd);
    if (ctx_.show_module_wnd)
        moduleWnd_.Render(&ctx_.show_module_wnd);
    if (ctx_.show_kernel_wnd)
        kernelWnd_.Render(&ctx_.show_kernel_wnd);
    if (ctx_.show_regedit_wnd)
        regeditWnd_.Render(&ctx_.show_regedit_wnd);
    if (ctx_.show_net_wnd)
        netWnd_.Render(&ctx_.show_net_wnd);
    if (ctx_.show_file_wnd)
        fileWnd_.Render(&ctx_.show_file_wnd);
    if (ctx_.show_log_wnd)
        logWnd_.Render(&ctx_.show_log_wnd);

    ImGui::ShowStyleEditor();

    ImGui::End();
}

void App::SetDockingWnd(bool* p_open)
{
    static bool opt_fullscreen = true;
    static bool opt_padding = false;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }
    else
    {
        dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
    }

    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    if (!opt_padding)
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin(u8"驱动级任务管理器", p_open, window_flags);
    if (!opt_padding)
        ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    // DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("TaskManagerDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }


}

void App::test() {

    int count =60; 
    auto* pInfo = ctx_.arkR3.GetProcessInfo(count);

    ctx_.list.clear();
    for (int i = 0; i < count; ++i) {
        PROCESS_INFO info;
        strcpy_s(info.Name, pInfo[i].Name);
        info.Id = pInfo[i].Id;
        info.ParentId = pInfo[i].ParentId;
        info.Cpu = pInfo[i].Cpu;
        strcpy_s(info.Path, pInfo[i].Path);
        ctx_.list.push_back(info);
    }
    free(pInfo); 
}



