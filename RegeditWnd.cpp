#include "RegeditWnd.h"

void RegeditWnd::Render(bool* p_open)
{
    ImGui::Begin("注册表", p_open);
    ImGui::Text("这里显示注册表相关信息。");
    ImGui::End();
} 
