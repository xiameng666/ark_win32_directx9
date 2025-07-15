#pragma once
#include "interface.h"

class NetWnd : public ImguiWnd {
public:
    explicit NetWnd(Context* ctx);
    void Render(bool* p_open = nullptr) override;
private:

}; 
