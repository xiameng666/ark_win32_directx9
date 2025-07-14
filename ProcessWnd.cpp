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

    static std::vector<PROCESS_INFO> processUiVec;

    if (ImGui::Button(u8"刷新")) {
        // 获取进程列表
       /* processList = ctx_->arkR3.EnumProcesses32();*/
        DWORD count = ctx_->arkR3.ProcessGetCount();
        ctx_->arkR3.Log("数量%d", count);
        processUiVec = ctx_->arkR3.ProcessGetVec(count);
    }
    ImGui::Separator();

    if (ImGui::BeginTable("proc_table", 6, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn(u8"进程名");           // 第0列：显示名称+选择
        ImGui::TableSetupColumn(u8"进程ID");           // 第1列
        ImGui::TableSetupColumn(u8"父PID");            // 第2列
        ImGui::TableSetupColumn(u8"页目录地址");       // 第3列
        ImGui::TableSetupColumn(u8"EPROCESS地址");     // 第4列
        ImGui::TableSetupColumn(u8"完整路径");         // 第5列
        ImGui::TableHeadersRow();

        static int selected_index = -1;
        int row = 0;

        for (const auto& process : processUiVec) {  // ← 确保变量名正确
            ImGui::TableNextRow();

            // 第0列：进程名 + 选择
            ImGui::TableSetColumnIndex(0);
            bool is_selected = (selected_index == row);
            char selectableId[64];
            sprintf_s(selectableId, "%s##%d", process.ImageFileName, row);
            if (ImGui::Selectable(selectableId, is_selected, ImGuiSelectableFlags_SpanAllColumns)) {
                selected_index = row;
            }

            // 右键菜单
            char popupId[64];
            sprintf_s(popupId, "ProcessMenu##%d", row);
            if (ImGui::BeginPopupContextItem(popupId)) {
                if (ImGui::MenuItem(u8"读写内存")) {
                    targetPid_ = process.ProcessId;
                    sprintf_s(processIdText_, "%u", process.ProcessId);
                    ctx_->showMemoryWindow_ = true;
                }
                ImGui::EndPopup();
            }

            // 第1列：进程ID
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%u", process.ProcessId);

            // 第2列：父PID
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%u", process.ParentProcessId);

            // 第3列：页目录地址（十六进制）
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("0x%08X", process.DirectoryTableBase);

            // 第4列：EPROCESS地址
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%p", process.EprocessAddr);

            // 第5列：完整路径
            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%ls", process.FullImagePath);

            row++;
        }
        ImGui::EndTable();
    }

    ImGui::Text(u8"进程数量: %d", (int)processUiVec.size());

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

        ctx_->arkR3.MemAttachRead(processId, address, size);
    }

    ImGui::SameLine();

    if (ImGui::Button(u8"写")) {
        DWORD processId = strtoul(processIdText_, NULL, 10);
        ULONG address = strtoul(addressText_, NULL, 16);
        
        PVOID pData = ctx_->arkR3.GetBufferData();
        DWORD dataSize = ctx_->arkR3.GetDataSize();
        
        if (processId != 0 && address != 0 && pData && dataSize > 0) {
            ctx_->arkR3.MemAttachWrite(processId, address, dataSize);
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
