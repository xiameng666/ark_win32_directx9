#include "FileWnd.h"

void FileWnd::Render(bool* p_open)
{
    ImGui::Begin(u8"�ļ�", p_open);
    ImGui::Text(u8"������ʾ�ļ��������Ϣ��");
    ImGui::End();
} 
