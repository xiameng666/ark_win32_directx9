// ProcessWnd.h
#pragma once
#include "interface.h"

class ProcessWnd : public ImguiWnd {
public:
    explicit ProcessWnd(Context* ctx) : ImguiWnd(ctx) {}
    void Render(bool* p_open = nullptr) override;
private:

};
