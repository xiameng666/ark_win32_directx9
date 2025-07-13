// ProcessWnd.h
#pragma once
#include "interface.h"

class ProcessWnd : public ImguiWnd {
public:
    explicit ProcessWnd(Context* ctx);
    void Render(bool* p_open = nullptr) override;
    void RenderProcessWnd();
    void RenderMemWnd(DWORD pid);
private:

    char processIdText_[16] = "1234";   // 进程ID输入框
    char addressText_[16] = "00401000"; // 地址输入框
    char sizeText_[16] = "256";         // 大小输入框
    char writeDataText_[1024] = "";     // 写入数据文本框
    
    // 内存窗口状态
    bool showMemoryWindow_ = false;
    DWORD targetPid_ = 0;

    std::vector<PROCESSENTRY32> processList;
};
