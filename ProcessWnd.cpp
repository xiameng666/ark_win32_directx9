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
    
    // 显示内存操作窗口
    if (showMemoryWindow_) {
        RenderMemWnd(targetPid_);
    }
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

            // 右键菜单 - 使用唯一ID避免冲突
            char popupId[64];
            sprintf_s(popupId, "ProcessMenu##%d", row);
            if (ImGui::BeginPopupContextItem(popupId)) {
                if (ImGui::MenuItem(u8"读写内存")) {
                    // 设置目标PID并显示内存窗口
                    targetPid_ = proc.th32ProcessID;
                    sprintf_s(processIdText_, "%u", proc.th32ProcessID);
                    showMemoryWindow_ = true;
                }
                ImGui::EndPopup();
            }

            // 显示其他列的数据
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

    // 显示进程数量
    ImGui::Text(u8"进程数量: %d", (int)processList.size());
}

void ProcessWnd::RenderMemWnd(DWORD pid)
{
    ImGui::SetNextWindowSize(ImVec2(700, 500), ImGuiCond_FirstUseEver);
    ImGui::Begin(u8"内存读写", &showMemoryWindow_);
    
    ImGui::Text(u8"目标进程ID: %u", pid);
    ImGui::Separator();
    
    // 内存读写参数输入
    ImGui::Text(u8"进程ID:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::InputText("##ProcessId", processIdText_, sizeof(processIdText_));
    ImGui::SameLine();
    ImGui::Text(u8"地址:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::InputText("##Address", addressText_, sizeof(addressText_));
    ImGui::SameLine();
    ImGui::Text(u8"大小:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    ImGui::InputText("##Size", sizeText_, sizeof(sizeText_));

    if (ImGui::Button(u8"测试读")) {
        ULONG processId = strtoul(processIdText_, NULL, 10);
        ULONG address = strtoul(addressText_, NULL, 16);
        DWORD size = strtoul(sizeText_, NULL, 10);

        if (ctx_->arkR3.AttachReadMem(processId, address, size)) {
            ctx_->arkR3.Log("读取内存成功：PID=%d, 地址=0x%08X, 大小=%d字节", processId, address, size);
            
            // 从内部缓冲区获取数据
            PVOID pData = ctx_->arkR3.GetBufferData();
            DWORD dataSize = ctx_->arkR3.GetDataSize();
            
            // 显示前16字节数据预览
            if (dataSize > 0) {
                char previewBuffer[64] = {0};
                int previewSize = (dataSize > 16) ? 16 : dataSize;
                char* pos = previewBuffer;
                PUCHAR data = (PUCHAR)pData;
                
                for (int i = 0; i < previewSize; i++) {
                    pos += sprintf(pos, "%02X ", data[i]);
                }
                if (dataSize > 16) {
                    strcat(previewBuffer, "...");
                }
                
                ctx_->arkR3.Log("数据预览：%s", previewBuffer);
            }
            
            // 使用格式化的十六进制显示（16字节一行）
            std::string formattedHexStr = ctx_->arkR3.GetDataAsFormattedHexString();
            if (formattedHexStr.length() > 1020) formattedHexStr = formattedHexStr.substr(0, 1020); 
            strcpy(writeDataText_, formattedHexStr.c_str());
            
        } else {
            ctx_->arkR3.Log("读取内存失败：PID=%d, 地址=0x%08X, 大小=%d", processId, address, size);
        }
    }

    ImGui::SameLine();

    if (ImGui::Button(u8"写入内存")) {
        DWORD processId = strtoul(processIdText_, NULL, 10);
        ULONG address = strtoul(addressText_, NULL, 16);
        
        if (processId != 0 && address != 0 && strlen(writeDataText_) > 0) {
            // 使用新的API：先将十六进制字符串设置到内部缓冲区
            if (ctx_->arkR3.SetWriteDataFromHex(writeDataText_)) {
                DWORD dataSize = ctx_->arkR3.GetDataSize();
                
                // 执行写入
                if (ctx_->arkR3.AttachWriteMem(processId, address, dataSize)) {
                    ctx_->arkR3.Log("写入内存成功：PID=%d, 地址=0x%08X, 大小=%d字节", processId, address, dataSize);
                } else {
                    ctx_->arkR3.Log("写入内存失败：PID=%d, 地址=0x%08X, 大小=%d", processId, address, dataSize);
                }
            } else {
                ctx_->arkR3.Log("写入数据格式错误：十六进制字符串格式无效");
            }
        } else {
            ctx_->arkR3.Log("写入失败：进程ID、地址或写入数据为空");
        }
    }

    // 写入数据输入框
    ImGui::Text(u8"写入数据(十六进制):");
    ImGui::InputTextMultiline("##WriteData", writeDataText_, sizeof(writeDataText_), ImVec2(-1, 250));

    ImGui::End();
}
