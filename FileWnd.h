#pragma once
#include "interface.h"

class FileWnd : public ImguiWnd {
public:
    FileWnd() = default;
    void Render(bool* p_open = nullptr) override;
}; 