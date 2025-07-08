#pragma once
#include "interface.h"

class RegeditWnd : public ImguiWnd {
public:
    RegeditWnd() = default;
    void Render(bool* p_open = nullptr) override;
}; 