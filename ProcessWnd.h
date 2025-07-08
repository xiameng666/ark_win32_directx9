// ProcessWnd.h
#pragma once
#include "interface.h"

struct Context;

class ProcessWnd : public ImguiWnd {
public:
    explicit ProcessWnd(Context* ctx) : ctx_(ctx) {}
    void Render(bool* p_open = nullptr) override;
private:
    Context* ctx_;
};
