#include "RegeditWnd.h"

void RegeditWnd::Render(bool* p_open)
{
    ImGui::Begin(u8"ע���", p_open);
    ImGui::Text(u8"������ʾע��������Ϣ��");
    ImGui::End();
} 
