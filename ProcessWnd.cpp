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
    ImGui::Begin(u8"����", p_open);
    
    RenderProcessWnd();
    
    ImGui::End();
    
    // ��ʾ�ڴ��������
    if (showMemoryWindow_) {
        RenderMemWnd(targetPid_);
    }
}

void ProcessWnd::RenderProcessWnd() {


    if (ImGui::Button(u8"ˢ��")) {
        // ��ȡ�����б�
        processList = ctx_->arkR3.EnumProcesses32();
    }
    ImGui::Separator();

    if (ImGui::BeginTable("proc_table", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn(u8"������");
        ImGui::TableSetupColumn(u8"PID");
        ImGui::TableSetupColumn(u8"��PID");
        ImGui::TableSetupColumn(u8"�߳���");
        ImGui::TableSetupColumn(u8"����");
        ImGui::TableHeadersRow();

        static int selected_index = -1;
        int row = 0;

        for (const auto& proc : processList) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            // ����ѡ��
            bool is_selected = (selected_index == row);
            if (ImGui::Selectable(proc.szExeFile, is_selected, ImGuiSelectableFlags_SpanAllColumns)) {
                selected_index = row;
            }

            // �Ҽ��˵�
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem(u8"��д�ڴ�")) {
                    // ����Ŀ��PID����ʾ�ڴ洰��
                    targetPid_ = proc.th32ProcessID;
                    sprintf_s(processIdText_, "%u", proc.th32ProcessID);
                    showMemoryWindow_ = true;
                }
                ImGui::EndPopup();
            }

            // ��ʾ�����е�����
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%u", proc.th32ProcessID);

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%u", proc.th32ParentProcessID);

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%u", proc.cntThreads);


        }
        ImGui::EndTable();
    }

    // ��ʾ��������
    ImGui::Text(u8"��������: %d", (int)processList.size());
}

void ProcessWnd::RenderMemWnd(DWORD pid)
{
    ImGui::Begin(u8"�ڴ��д", &showMemoryWindow_);
    
    ImGui::Text(u8"Ŀ�����ID: %u", pid);
    ImGui::Separator();
    
    // �ڴ��д��������
    ImGui::Text(u8"����ID:");
    ImGui::SameLine();
    ImGui::InputText("##ProcessId", processIdText_, sizeof(processIdText_));
    ImGui::SameLine();
    ImGui::Text(u8"��ַ:");
    ImGui::SameLine();
    ImGui::InputText("##Address", addressText_, sizeof(addressText_));
    ImGui::SameLine();
    ImGui::Text(u8"��С:");
    ImGui::SameLine();
    ImGui::InputText("##Size", sizeText_, sizeof(sizeText_));

    if (ImGui::Button(u8"���Զ�")) {
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
            ctx_->arkR3.Log("��ȡ�ڴ�ɹ���PID=%d, ��ַ=0x%08X, ��С=%d�ֽ�", processId, address, size);

        }

        delete[] memData_;
    }

    ImGui::SameLine();

    if (ImGui::Button(u8"д���ڴ�")) {


    }

    // д�����������
    ImGui::Text(u8"д������(ʮ������):");
    ImGui::InputTextMultiline("##WriteData", writeDataText_, sizeof(writeDataText_), ImVec2(-1, 150));

    ImGui::End();
}
