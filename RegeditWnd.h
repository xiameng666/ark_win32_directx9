#pragma once
#include "interface.h"

struct Context;

class RegeditWnd : public ImguiWnd {
public:
    explicit RegeditWnd(Context* ctx) : ctx_(ctx) {}
    void Render(bool* p_open = nullptr) override;
private:
    Context* ctx_;
}; 