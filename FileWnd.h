#pragma once
#include "interface.h"

struct Context;

class FileWnd : public ImguiWnd {
public:
    explicit FileWnd(Context* ctx) : ctx_(ctx) {}
    void Render(bool* p_open = nullptr) override;
private:
    Context* ctx_;
}; 