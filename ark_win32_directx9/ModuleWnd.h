#pragma once
#include "interface.h"
#include <vector>

class ModuleWnd : public ImguiWnd {
public:
    explicit ModuleWnd(Context* ctx);
    void Render(bool* p_open = nullptr) override;
    void RenderKernelModule();
    
    // 静态 可复用的模块列表UI渲染
    static void RenderModuleTable(const std::vector<MODULE_INFO>& moduleList, const char* windowTitle = u8"模块列表");
    
private:

}; 
