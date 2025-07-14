#pragma once
#include "interface.h"

class ModuleWnd : public ImguiWnd {
public:
    explicit ModuleWnd(Context* ctx) : ImguiWnd(ctx) {}
    void Render(bool* p_open = nullptr) override;
private:

}; 