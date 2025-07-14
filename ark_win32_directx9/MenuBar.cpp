#include "MenuBar.h"
#include "imgui.h"


#define MENUTOP

#ifdef MENUTOP
void MenuBar::Render(bool* p_open)
{
    (void)p_open;//�ò��� ��δʹ�ú�����

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu(u8"��ͼ"))
        {
            ImGui::MenuItem(u8"�������", nullptr, &ctx_->show_process_wnd);
            ImGui::MenuItem(u8"ģ�����", nullptr, &ctx_->show_module_wnd);
            ImGui::MenuItem(u8"�ں����", nullptr, &ctx_->show_kernel_wnd);
            ImGui::MenuItem(u8"ע������", nullptr, &ctx_->show_regedit_wnd);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(u8"����"))
        {
            if (ImGui::MenuItem(u8"��װ����")) {
                ctx_->arkR3.SetPath();
                ctx_->arkR3.Load();
                ctx_->arkR3.Start();
            }
            if (ImGui::MenuItem(u8"����")) {
                ctx_->arkR3.Open();
            }
            if (ImGui::MenuItem(u8"ж������")) {
                ctx_->arkR3.Stop();
                ctx_->arkR3.Unload();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu(u8"����")) {
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
    // �̶�����࣬���200
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(200, ImGui::GetIO().DisplaySize.y), ImGuiCond_Always);

    ImGui::Begin("�������", p_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    ImGui::Text("�����л�");
    ImGui::Separator();
    ImGui::Checkbox("�������", &ctx_->show_process_wnd);
    ImGui::Checkbox("ģ�����", &ctx_->show_module_wnd);
    ImGui::Checkbox("�ں����", &ctx_->show_kernel_wnd);
    ImGui::Checkbox("ע������", &ctx_->show_regedit_wnd);
    ImGui::Checkbox("�ļ����", &ctx_->show_file_wnd);
    ImGui::Checkbox("�������", &ctx_->show_net_wnd);

    ImGui::Separator();
    ImGui::Text("��������");
    // ������Լ�������ذ�ť
    // if (ImGui::Button("��������")) { ... }

    ImGui::End();
}

#endif // LEFT


