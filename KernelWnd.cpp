#include "KernelWnd.h"

void KernelWnd::Render(bool* p_open)
{
    ImGui::Begin(u8"�ں�", p_open);
    ImGui::Text(u8"������ʾ�ں������Ϣ��");
    if (ImGui::CollapsingHeader(u8"GDT (ȫ����������)", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button(u8"����GetGDTVec")) {
            std::vector<GDT_INFO> gdtData = ctx_->arkR3.GetGDTVec();
        }

    }
    ImGui::End();
} 
