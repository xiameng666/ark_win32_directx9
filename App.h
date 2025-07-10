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
    App();
    ~App();

    void Render();
    void SetDockingWnd(bool* p_open);




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


