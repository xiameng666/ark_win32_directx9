#pragma once
#include"Interface.h"

#include "ProcessWnd.h"
#include "ModuleWnd.h"
#include "KernelWnd.h"
#include "RegeditWnd.h"
#include "FileWnd.h"
#include "NetWnd.h"



class App {
public:
    App() : processWnd(&Ctx), moduleWnd(&Ctx), kernelWnd(&Ctx), regeditWnd(&Ctx), fileWnd(&Ctx), netWnd(&Ctx) {
    }

    void Render();
    void SetDockingWnd(bool* p_open);
    void RenderMenuBar();

    void test();


    Context Ctx;

    ProcessWnd processWnd;
    ModuleWnd moduleWnd;
    KernelWnd kernelWnd;
    RegeditWnd regeditWnd;
    FileWnd fileWnd;
    NetWnd netWnd;
};


