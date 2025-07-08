#include "KernelWnd.h"

void KernelWnd::Render(bool* p_open)
{
    ImGui::Begin("内核", p_open);
    ImGui::Text("这里显示内核相关信息。");
    ImGui::End();
} 
