#include "ModuleWnd.h"

void ModuleWnd::Render(bool* p_open)
{
    ImGui::Begin(u8"ģ��", p_open);
    ImGui::Text(u8"������ʾģ�������Ϣ��");
    ImGui::End();
} 
