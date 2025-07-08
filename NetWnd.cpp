#include "NetWnd.h"

void NetWnd::Render(bool* p_open)
{
    ImGui::Begin(u8"网络", p_open);
    ImGui::Text(u8"这里显示Net相关信息。");
    ImGui::End();
} 
