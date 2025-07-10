#pragma once
#include "interface.h"


class KernelWnd : public ImguiWnd {
public:
    explicit KernelWnd(Context* ctx);
    void Render(bool* p_open = nullptr) override;
private:


    void RenderGDTTable();
    void RenderIDTTable();
    void RenderLeftBar();
};
