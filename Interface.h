#pragma once
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <GL/GL.h>
#include <tchar.h>

class ImguiWnd {
public:
    virtual void Render(bool* p_open = nullptr) = 0;
    //virtual bool IsVisible() const = 0;
    //virtual void SetVisible(bool visible) = 0;
};

