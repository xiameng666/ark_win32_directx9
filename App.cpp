#include "App.h"

void App::Render()
{
    SetDockingWnd(0);

    RenderMenuBar();

    if (Ctx.show_process_wnd)
        processWnd.Render(&Ctx.show_process_wnd);
    if (Ctx.show_module_wnd)
        moduleWnd.Render(&Ctx.show_module_wnd);
    if (Ctx.show_kernel_wnd)
        kernelWnd.Render(&Ctx.show_kernel_wnd);
    if (Ctx.show_regedit_wnd)
        regeditWnd.Render(&Ctx.show_regedit_wnd);
    if (Ctx.show_net_wnd)
        netWnd.Render(&Ctx.show_net_wnd);
    if (Ctx.show_file_wnd)
        fileWnd.Render(&Ctx.show_file_wnd);

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
    ImGui::Begin("驱动级任务管理器", p_open, window_flags);
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

void App::RenderMenuBar()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("视图"))
        {
            ImGui::MenuItem("进程面板", nullptr, &Ctx.show_process_wnd);
            ImGui::MenuItem("模块面板", nullptr, &Ctx.show_module_wnd);
            ImGui::MenuItem("内核面板", nullptr, &Ctx.show_kernel_wnd);
            ImGui::MenuItem("注册表面板", nullptr, &Ctx.show_regedit_wnd);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("驱动"))
        {
            if (ImGui::MenuItem("SetPath")) {
                // 调用 SetPath
            }
            if (ImGui::MenuItem("Load")) {
                // 调用 Load
            }
            if (ImGui::MenuItem("Start")) {
                // 调用 Start
            }
            if (ImGui::MenuItem("Open")) {
                // 调用 Open
            }
            if (ImGui::MenuItem("Stop/Unload")) {
                // 调用 Stop/Unload
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }


}

void App::test() {

    int count =60; 
    auto* pInfo = arkR3.GetProcessInfo(count); 

    Ctx.list.clear();
    for (int i = 0; i < count; ++i) {
        PROCESS_INFO info;
        strcpy_s(info.Name, pInfo[i].Name);
        info.Id = pInfo[i].Id;
        info.ParentId = pInfo[i].ParentId;
        info.Cpu = pInfo[i].Cpu;
        strcpy_s(info.Path, pInfo[i].Path);
        Ctx.list.push_back(info);
    }
    free(pInfo); 
}

void App::RenderProcessWnd()
{
  
}

void App::RenderModuleWnd()
{

    
}

void App::RenderKernelWnd()
{
   
}

void App::RenderRegeditWnd()
{

}

void App::RenderFileWnd()
{

}

void App::RenderNetWnd()
{

}


