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
    void RenderProcessModuleWnd();  // ����ģ�鴰��
    
private:


    //std::vector<PROCESSENTRY32> processList;

    MemoryEditor mem_edit;
};
