#include "NetWnd.h"

void NetWnd::Render(bool* p_open)
{
    ImGui::Begin("网络", p_open);
    ImGui::Text("这里显示Net相关信息。");
    ImGui::End();
} 
