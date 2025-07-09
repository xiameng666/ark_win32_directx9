#pragma once
#include"Interface.h"
#include "ProcessWnd.h"
#include "ModuleWnd.h"
#include "KernelWnd.h"
#include "RegeditWnd.h"
#include "FileWnd.h"
#include "NetWnd.h"
#include "MenuBar.h"
#include "LogWnd.h"



class App {
public:
    App() : processWnd_(&ctx_)
        , moduleWnd_(&ctx_)
        , kernelWnd_(&ctx_)
        , regeditWnd_(&ctx_)
        , fileWnd_(&ctx_)
        , netWnd_(&ctx_)
        , menuBar_ { &ctx_ }
        , logWnd_{ &ctx_ }
    { }

    void Render();
    void SetDockingWnd(bool* p_open);

    void test();


    Context ctx_;
    LogWnd logWnd_;
    MenuBar menuBar_;
    ProcessWnd processWnd_;
    ModuleWnd moduleWnd_;
    KernelWnd kernelWnd_;
    RegeditWnd regeditWnd_;
    FileWnd fileWnd_;
    NetWnd netWnd_;
};


