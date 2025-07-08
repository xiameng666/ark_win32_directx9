#pragma once
#include <vector>
#include <string>
#include "interface.h"

class LogWnd : public ImguiWnd , ILogObserver {
public:
    explicit LogWnd(Context* ctx) : ctx_(ctx) {
        ctx_->arkR3.SetLogObserver(this);
    }
    void AddLog(const char* log);
    void Clear();
    void Render(bool* p_open = nullptr);
    void OnLog(const char* msg);


private:
    Context* ctx_;
    std::vector<std::string> logs_;
    bool scrollToBottom_ = false;

}; 


