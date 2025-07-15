// ProcessWnd.h
#pragma once
#include "interface.h"
#include <vector>

class ProcessWnd : public ImguiWnd {
public:
    explicit ProcessWnd(Context* ctx);
    void Render(bool* p_open = nullptr) override;
    void RenderProcessWnd();
    void RenderMemWnd(DWORD pid);
    void RenderProcessModuleWnd();  // 进程模块窗口
    
private:


    //std::vector<PROCESSENTRY32> processList;

    MemoryEditor mem_edit;
};
