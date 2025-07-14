#pragma once
#include "interface.h"

class FileWnd : public ImguiWnd {
public:
    explicit FileWnd(Context* ctx) : ImguiWnd(ctx) {}
    void Render(bool* p_open = nullptr) override;
private:

}; 