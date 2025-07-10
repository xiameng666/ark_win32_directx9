#pragma once
#include "interface.h"

class RegeditWnd : public ImguiWnd {
public:
    explicit RegeditWnd(Context* ctx) : ImguiWnd(ctx) {}
    void Render(bool* p_open = nullptr) override;
private:

}; 