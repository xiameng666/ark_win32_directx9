#include "ProcessWnd.h"

ProcessWnd::ProcessWnd(Context* ctx) 
    :ImguiWnd(ctx)
{
    viewRenderers_[PROCESS] = [this]() { RenderProcessWnd(); };
    viewRenderers_[MEM] = [this]() { RenderProcessWnd(); };
}

void ProcessWnd::Render(bool* p_open)
{
    ImGui::Begin(u8"进程", p_open);
    
    RenderProcessWnd();
    
    ImGui::End();
    
    
}

void ProcessWnd::RenderProcessWnd() {

    if (ImGui::Button(u8"刷新")) {
        // 获取进程列表
        processList = ctx_->arkR3.EnumProcesses32();
    }
    ImGui::Separator();

    if (ImGui::BeginTable("proc_table", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn(u8"进程名");
        ImGui::TableSetupColumn(u8"PID");
        ImGui::TableSetupColumn(u8"父PID");
        ImGui::TableSetupColumn(u8"线程数");
        ImGui::TableSetupColumn(u8"操作");
        ImGui::TableHeadersRow();

        static int selected_index = -1;
        int row = 0;

        for (const auto& proc : processList) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            // 整行选中
            bool is_selected = (selected_index == row);
            char selectableId[64];
            sprintf_s(selectableId, "%s##%d", proc.szExeFile, row);
            if (ImGui::Selectable(selectableId, is_selected, ImGuiSelectableFlags_SpanAllColumns)) {
                selected_index = row;
            }

            // 右键菜单
            char popupId[64];
            sprintf_s(popupId, "ProcessMenu##%d", row);
            if (ImGui::BeginPopupContextItem(popupId)) {
                if (ImGui::MenuItem(u8"读写内存")) {
                    // 设置目标PID并显示内存窗口
                    targetPid_ = proc.th32ProcessID;
                    sprintf_s(processIdText_, "%u", proc.th32ProcessID);
                    ctx_->showMemoryWindow_ = true;
                }
                ImGui::EndPopup();
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%u", proc.th32ProcessID);

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%u", proc.th32ParentProcessID);

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%u", proc.cntThreads);

            row++;
        }
        ImGui::EndTable();
    }

    ImGui::Text(u8"进程数量: %d", (int)processList.size());

    // 显示内存操作窗口
    if (ctx_->showMemoryWindow_) {
        RenderMemWnd(targetPid_);
    }
}

void ProcessWnd::RenderMemWnd(DWORD pid)
{
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin(u8"内存读写", &ctx_->showMemoryWindow_);
    
    ImGui::Text(u8"目标进程ID: %u", pid);
    ImGui::Separator();

    ImGui::Text(u8"地址:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::InputText("##Address", addressText_, sizeof(addressText_));
    ImGui::SameLine();
    ImGui::Text(u8"大小:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    ImGui::InputText("##Size", sizeText_, sizeof(sizeText_));

        if (ImGui::Button(u8"读")) {
        ULONG processId = strtoul(processIdText_, NULL, 10);
        ULONG address = strtoul(addressText_, NULL, 16);
        DWORD size = strtoul(sizeText_, NULL, 10);

        ctx_->arkR3.AttachReadMem(processId, address, size);
    }

    ImGui::SameLine();

    if (ImGui::Button(u8"写")) {
        DWORD processId = strtoul(processIdText_, NULL, 10);
        ULONG address = strtoul(addressText_, NULL, 16);
        
        PVOID pData = ctx_->arkR3.GetBufferData();
        DWORD dataSize = ctx_->arkR3.GetDataSize();
        
        if (processId != 0 && address != 0 && pData && dataSize > 0) {
            ctx_->arkR3.AttachWriteMem(processId, address, dataSize);
        } else {
            ctx_->arkR3.Log("processId != 0 && address != 0 && pData && dataSize > 0");
        }
    }

    ImGui::Separator();
    
    // 显示内存编辑器
    PVOID pData = ctx_->arkR3.GetBufferData();
    DWORD dataSize = ctx_->arkR3.GetDataSize();
    
    if (pData && dataSize > 0) {
        ImGui::Text(u8"内存编辑器:");
        ULONG address = strtoul(addressText_, NULL, 16);
        mem_edit.DrawContents(pData, dataSize, address);
    } else {
        ImGui::Text(u8"请先读取内存数据");
    }

    ImGui::End();
}
