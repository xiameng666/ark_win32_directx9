#include "KernelWnd.h"

void KernelWnd::Render(bool* p_open)
{
    ImGui::Begin(u8"内核", p_open);
    ImGui::Text(u8"这里显示内核相关信息。");
    if (ImGui::CollapsingHeader(u8"GDT (全局描述符表)", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button(u8"调用GetGDTVec")) {
            std::vector<GDT_INFO> gdtData = ctx_->arkR3.GetGDTVec();
        }

    }
    ImGui::End();
} 
