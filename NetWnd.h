#pragma once
#include "interface.h"

class NetWnd : public ImguiWnd {
public:
    explicit NetWnd(Context* ctx) : ImguiWnd(ctx) {}
    void Render(bool* p_open = nullptr) override;
private:

}; 
