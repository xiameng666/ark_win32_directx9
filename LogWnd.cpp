#include "LogWnd.h"

std::string AnsiToUtf8(const char* ansi)
{
    // 输入验证
    if (!ansi || strlen(ansi) == 0) {
        return std::string();
    }

    // 限制输入长度，防止过长字符串导致内存问题
    const size_t MAX_INPUT_LEN = 4096;
    size_t inputLen = strlen(ansi);
    if (inputLen > MAX_INPUT_LEN) {
        inputLen = MAX_INPUT_LEN;
    }

    // 获取所需的宽字符数量
    int wlen = MultiByteToWideChar(CP_ACP, 0, ansi, (int)inputLen, NULL, 0);
    if (wlen <= 0 || wlen > 8192) { // 限制最大宽字符数量
        return std::string(ansi); // 转换失败，返回原字符串
    }

    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(CP_ACP, 0, ansi, (int)inputLen, &wstr[0], wlen);

    int u8len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wlen, NULL, 0, NULL, NULL);
    if (u8len <= 0 || u8len > 16384) { // 限制最大UTF8字符数量
        return std::string(ansi); // 转换失败，返回原字符串
    }

    std::string u8str(u8len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wlen, &u8str[0], u8len, NULL, NULL);

    return u8str;
}


void LogWnd::AddLog(const char* log) {
    logs_.emplace_back(AnsiToUtf8(log));
    //logs_.push_back(log);
    scrollToBottom_ = true;
}

void LogWnd::Clear() {
    logs_.clear();
}

void LogWnd::Render(bool* p_open) {
    ImGui::Begin("Log", p_open);
    if (ImGui::Button(u8"清空")) Clear();
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
