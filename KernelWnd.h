#pragma once
#include "interface.h"

class KernelWnd : public ImguiWnd {
public:
    KernelWnd() = default;
    void Render(bool* p_open = nullptr) override;
}; 