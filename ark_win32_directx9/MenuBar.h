#pragma once
#include "interface.h"


struct Context;

class MenuBar:public ImguiWnd {
public:
    explicit MenuBar(Context* ctx) : ImguiWnd(ctx) {}
    void Render(bool* p_open = nullptr);
private:

}; 
