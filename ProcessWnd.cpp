#include "ProcessWnd.h"

ProcessWnd::ProcessWnd(Context* ctx) 
    :ImguiWnd(ctx), memData_(nullptr)
{
    viewRenderers_[PROCESS] = [this]() { RenderProcessWnd(); };
    viewRenderers_[MEM] = [this]() { RenderProcessWnd(); };
}

ProcessWnd::~ProcessWnd()
{
    if (memData_) {
        delete[] memData_;
        memData_ = nullptr;
    }
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
            if (ImGui::Selectable(proc.szExeFile, is_selected, ImGuiSelectableFlags_SpanAllColumns)) {
                selected_index = row;
            }

            // 右键菜单
            if (ImGui::BeginPopupContextItem()) {
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


        }
        ImGui::EndTable();
    }

    // 显示进程数量
    ImGui::Text(u8"进程数量: %d", (int)processList.size());
}

void ProcessWnd::RenderMemWnd(DWORD pid)
{
    ImGui::Begin(u8"内存读写", &showMemoryWindow_);
    
    ImGui::Text(u8"目标进程ID: %u", pid);
    ImGui::Separator();
    
    // 内存读写参数输入
    ImGui::Text(u8"进程ID:");
    ImGui::SameLine();
    ImGui::InputText("##ProcessId", processIdText_, sizeof(processIdText_));
    ImGui::SameLine();
    ImGui::Text(u8"地址:");
    ImGui::SameLine();
    ImGui::InputText("##Address", addressText_, sizeof(addressText_));
    ImGui::SameLine();
    ImGui::Text(u8"大小:");
    ImGui::SameLine();
    ImGui::InputText("##Size", sizeText_, sizeof(sizeText_));

    if (ImGui::Button(u8"测试读")) {
        ULONG processId = strtoul(processIdText_, NULL, 10);
        ULONG address = strtoul(addressText_, NULL, 16);
        DWORD size = strtoul(sizeText_, NULL, 10);

        if (memData_) {
            delete[] memData_;
            memData_ = nullptr;
        }
        memData_ = new unsigned char[size];
        memset(memData_, 0, size);

        if (ctx_->arkR3.AttachReadMem(processId, address, memData_, size)) {
            ctx_->arkR3.Log("读取内存成功：PID=%d, 地址=0x%08X, 大小=%d字节", processId, address, size);

        }

        delete[] memData_;
    }

    ImGui::SameLine();

    if (ImGui::Button(u8"写入内存")) {


    }

    // 写入数据输入框
    ImGui::Text(u8"写入数据(十六进制):");
    ImGui::InputTextMultiline("##WriteData", writeDataText_, sizeof(writeDataText_), ImVec2(-1, 150));

    ImGui::End();
}
