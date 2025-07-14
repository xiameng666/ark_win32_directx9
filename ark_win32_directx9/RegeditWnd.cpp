#include "RegeditWnd.h"

void RegeditWnd::Render(bool* p_open)
{
    ImGui::Begin(u8"注册表", p_open);
    ImGui::Text(u8"这里显示注册表相关信息。");
    ImGui::End();
} 
