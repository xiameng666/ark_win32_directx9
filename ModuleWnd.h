#pragma once
#include "interface.h"

class ModuleWnd : public ImguiWnd {
public:
    ModuleWnd() = default;
    void Render(bool* p_open = nullptr) override;
}; 