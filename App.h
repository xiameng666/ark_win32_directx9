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
    App() : processWnd(&ctx)
        , moduleWnd(&ctx)
        , kernelWnd(&ctx)
        , regeditWnd(&ctx)
        , fileWnd(&ctx)
        , netWnd(&ctx)
        ,menuBar { &ctx }
        ,logWnd{ &ctx }
    { }

    void Render();
    void SetDockingWnd(bool* p_open);

    void test();


    Context ctx;
    LogWnd logWnd;
    MenuBar menuBar;
    ProcessWnd processWnd;
    ModuleWnd moduleWnd;
    KernelWnd kernelWnd;
    RegeditWnd regeditWnd;
    FileWnd fileWnd;
    NetWnd netWnd;
};


