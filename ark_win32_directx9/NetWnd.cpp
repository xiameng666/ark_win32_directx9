#include "NetWnd.h"

NetWnd::NetWnd(Context* ctx)
    : ImguiWnd(ctx)
{
}

void NetWnd::Render(bool* p_open)
{
    ImGui::Begin(u8"����", p_open);
    ImGui::Text(u8"������ʾNet�����Ϣ��");
    ImGui::End();
} 
