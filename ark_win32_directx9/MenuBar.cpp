#include "MenuBar.h"
#include "imgui.h"


#define MENUTOP

#ifdef MENUTOP
void MenuBar::Render(bool* p_open)
{
    (void)p_open;//用不到 报未使用很难受

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu(u8"视图"))
        {
            ImGui::MenuItem(u8"进程面板", nullptr, &ctx_->show_process_wnd);
            ImGui::MenuItem(u8"模块面板", nullptr, &ctx_->show_module_wnd);
            ImGui::MenuItem(u8"内核面板", nullptr, &ctx_->show_kernel_wnd);
            ImGui::MenuItem(u8"注册表面板", nullptr, &ctx_->show_regedit_wnd);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(u8"驱动"))
        {
            if (ImGui::MenuItem(u8"安装驱动")) {
                ctx_->arkR3.SetPath();
                ctx_->arkR3.Load();
                ctx_->arkR3.Start();
            }
            if (ImGui::MenuItem(u8"连接")) {
                ctx_->arkR3.Open();
            }
            if (ImGui::MenuItem(u8"卸载驱动")) {
                ctx_->arkR3.Stop();
                ctx_->arkR3.Unload();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu(u8"主题")) {
            if (ImGui::MenuItem(u8"Dark")) ImGui::StyleColorsDark();
            if (ImGui::MenuItem(u8"Light")) ImGui::StyleColorsLight();
            if (ImGui::MenuItem(u8"Classic")) ImGui::StyleColorsClassic();
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

#endif // TOP

#ifdef MENULEFT
void MenuBar::Render(bool* p_open)
{
    // 固定在左侧，宽度200
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(200, ImGui::GetIO().DisplaySize.y), ImGuiCond_Always);

    ImGui::Begin("控制面板", p_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    ImGui::Text("窗口切换");
    ImGui::Separator();
    ImGui::Checkbox("进程面板", &ctx_->show_process_wnd);
    ImGui::Checkbox("模块面板", &ctx_->show_module_wnd);
    ImGui::Checkbox("内核面板", &ctx_->show_kernel_wnd);
    ImGui::Checkbox("注册表面板", &ctx_->show_regedit_wnd);
    ImGui::Checkbox("文件面板", &ctx_->show_file_wnd);
    ImGui::Checkbox("网络面板", &ctx_->show_net_wnd);

    ImGui::Separator();
    ImGui::Text("驱动操作");
    // 这里可以加驱动相关按钮
    // if (ImGui::Button("加载驱动")) { ... }

    ImGui::End();
}

#endif // LEFT


