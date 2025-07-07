#include "App.h"

void App::Render()
{
    SetDockingWnd(0);
    RenderMenuBar();
    RenderProcessWnd();
    RenderModuleWnd();
    RenderKernelWnd();
    RenderRegeditWnd();
    RenderNetWnd();
    RenderFileWnd();
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
        ImGui::EndMenuBar();
    }
}

void App::RefreshProcessList() {
    PROCESS_INFO buf[60] = { 0 };
    if (arkR3->EnumProcess(buf, 60)) {
        Ctx.list.clear();
        Ctx.list.assign(buf, buf + 60);
    }
}

void App::RenderProcessWnd()
{
    ImGui::Begin("进程", &Ctx.show_process_wnd);

    ImGui::Columns(5, "proc_columns");
    ImGui::Text("进程名"); ImGui::NextColumn();
    ImGui::Text("PID");    ImGui::NextColumn();
    ImGui::Text("父PID");  ImGui::NextColumn();
    ImGui::Text("CPU");    ImGui::NextColumn();
    ImGui::Text("路径");   ImGui::NextColumn();
    ImGui::Separator();


    for (const auto& proc : Ctx.list) {
        ImGui::Text("%s", proc.Name); ImGui::NextColumn();
        ImGui::Text("%u", proc.Id);   ImGui::NextColumn();
        ImGui::Text("%u", proc.ParentId); ImGui::NextColumn();
        ImGui::Text("%.2f", proc.Cpu);    ImGui::NextColumn();
        ImGui::Text("%s", proc.Path); ImGui::NextColumn();
    }
    ImGui::Columns(1);

    // 可选：刷新按钮
    if (ImGui::Button("刷新")) {
        RefreshProcessList();
    }
    ImGui::End();
}

void App::RenderModuleWnd()
{

    ImGui::Begin("模块", &Ctx.show_module_wnd);
    ImGui::Text("这里显示模块相关信息。");
    ImGui::End();
}

void App::RenderKernelWnd()
{
    ImGui::Begin("内核", &Ctx.show_kernel_wnd);
    ImGui::Text("这里显示内核相关信息。");
    ImGui::End();
}

void App::RenderRegeditWnd()
{
    ImGui::Begin("注册表", &Ctx.show_regedit_wnd);
    ImGui::Text("这里显示注册表相关信息。");
    ImGui::End();
}

void App::RenderFileWnd()
{
    ImGui::Begin("文件", &Ctx.show_file_wnd);
    ImGui::Text("这里显示文件表相关信息。");
    ImGui::End();
}

void App::RenderNetWnd()
{
    ImGui::Begin("网络", &Ctx.show_net_wnd);
    ImGui::Text("这里显示Net相关信息。");
    ImGui::End();
}


