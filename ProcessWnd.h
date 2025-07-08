// ProcessWnd.h
#pragma once
#include "interface.h"

class ProcessWnd : public ImguiWnd {
public:
    ProcessWnd() = default;
    void Render(bool* p_open = nullptr) override;
};
