#pragma once
#include "interface.h"


struct Context;

class MenuBar:public ImguiWnd {
public:
    explicit MenuBar(Context* ctx) : ctx_(ctx) {}
    void Render(bool* p_open = nullptr);
private:
    Context* ctx_;
}; 
