#include "LogWnd.h"

std::string AnsiToUtf8(const char* ansi)
{
    int wlen = MultiByteToWideChar(CP_ACP, 0, ansi, -1, NULL, 0);
    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(CP_ACP, 0, ansi, -1, &wstr[0], wlen);

    int u8len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    std::string u8str(u8len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &u8str[0], u8len, NULL, NULL);

    return u8str;
}


void LogWnd::AddLog(const char* log) {
    logs_.push_back(AnsiToUtf8(log));
    scrollToBottom_ = true;
}

void LogWnd::Clear() {
    logs_.clear();
}

void LogWnd::Render(bool* p_open) {
    ImGui::Begin("Log", p_open);
    if (ImGui::Button("清空")) Clear();
    ImGui::SameLine();
    //...

    ImGui::Separator();
    ImGui::BeginChild("LogScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto& line : logs_) {
        ImGui::TextUnformatted(line.c_str());
    }
    if (scrollToBottom_) {
        ImGui::SetScrollHereY(1.0f);
        scrollToBottom_ = false;
    }
    ImGui::EndChild();
    ImGui::End();
}
void LogWnd::OnLog(const char* msg)
{
    AddLog(msg);
}
