#pragma once
#include "interface.h"

struct Context;

class NetWnd : public ImguiWnd {
public:
    explicit NetWnd(Context* ctx) : ctx_(ctx) {}
    void Render(bool* p_open = nullptr) override;
private:
    Context* ctx_;
}; 