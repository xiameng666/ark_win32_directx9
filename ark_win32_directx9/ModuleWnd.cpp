#include "ModuleWnd.h"

ModuleWnd::ModuleWnd(Context* ctx)
    : ImguiWnd(ctx) 
{
}

void ModuleWnd::Render(bool* p_open)
{
    ImGui::Begin(u8"�ں�ģ��", p_open);
    
    RenderKernelModule();
    
    ImGui::End();
}

// ��Ⱦȫ��ϵͳģ��
void ModuleWnd::RenderKernelModule()
{
    if (ImGui::Button(u8"ˢ��")) {
        DWORD moduleCount = ctx_->arkR3.ModuleGetCount();
        if (moduleCount > 0) {
            ctx_->globalModuleUiVec = ctx_->arkR3.ModuleGetVec(moduleCount);
        } else {
            ctx_->globalModuleUiVec.clear();
        }
    }
    
    ImGui::SameLine();
    ImGui::Text(u8"ģ������: %d", (int)ctx_->globalModuleUiVec.size());
    
    ImGui::Separator();
    
    RenderModuleTable(ctx_->globalModuleUiVec, u8"�ں�ģ���б�");
}

// ��ProcessWnd�и���
void ModuleWnd::RenderModuleTable(const std::vector<MODULE_INFO>& moduleList, const char* windowTitle)
{
    if (moduleList.empty()) {
        ImGui::Text(u8"��������");
        return;
    }
    
    // ���������ʾģ����Ϣ
    if (ImGui::BeginTable(windowTitle, 5, 
        ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY)) {
        
        // ��ͷ
        ImGui::TableSetupColumn(u8"ģ������", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn(u8"����ַ", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn(u8"��С", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn(u8"����˳��", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn(u8"����·��", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        
        // ������
        for (size_t i = 0; i < moduleList.size(); i++) {
            const auto& module = moduleList[i];
            
            ImGui::TableNextRow();
            
            // ģ������
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", module.Name);
            
            // ����ַ
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%p", module.ImageBase);
            
            // ��С
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("0x%X", module.ImageSize);
            
            // ����˳��
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%d", module.LoadOrderIndex);
            
            // ����·��
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%s", module.FullPath);
        }
        
        ImGui::EndTable();
    }
} 
