#include "ModuleWnd.h"

void ModuleWnd::Render(bool* p_open)
{
    ImGui::Begin("模块", p_open);
    ImGui::Text("这里显示模块相关信息。");
    ImGui::End();
} 
