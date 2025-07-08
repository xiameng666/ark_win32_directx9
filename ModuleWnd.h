#pragma once
#include "interface.h"

struct Context;

class ModuleWnd : public ImguiWnd {
public:
    explicit ModuleWnd(Context* ctx) : ctx_(ctx) {}
    void Render(bool* p_open = nullptr) override;
private:
    Context* ctx_;
}; 