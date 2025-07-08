#pragma once
#include "interface.h"

struct Context;

class KernelWnd : public ImguiWnd {
public:
    explicit KernelWnd(Context* ctx) : ctx_(ctx) {}
    void Render(bool* p_open = nullptr) override;
private:
    Context* ctx_;
}; 