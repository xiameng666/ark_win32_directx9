#pragma once
#include "interface.h"

class NetWnd : public ImguiWnd {
public:
    NetWnd() = default;
    void Render(bool* p_open = nullptr) override;
}; 