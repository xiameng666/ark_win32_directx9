#include "KernelWnd.h"

void KernelWnd::Render(bool* p_open)
{
    ImGui::Begin(u8"�ں�", p_open);
    ImGui::Text(u8"������ʾ�ں������Ϣ��");
    ImGui::End();
} 
