#include "FileWnd.h"

void FileWnd::Render(bool* p_open)
{
    ImGui::Begin("文件", p_open);
    ImGui::Text("这里显示文件表相关信息。");
    ImGui::End();
} 
