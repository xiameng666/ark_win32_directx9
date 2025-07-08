#include "KernelWnd.h"

void KernelWnd::Render(bool* p_open)
{
    ImGui::Begin(u8"内核", p_open);
    ImGui::Text(u8"这里显示内核相关信息。");
    ImGui::End();
} 
