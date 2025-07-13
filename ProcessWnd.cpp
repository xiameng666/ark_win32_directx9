#include "ProcessWnd.h"

ProcessWnd::ProcessWnd(Context* ctx) 
    :ImguiWnd(ctx)
{
    viewRenderers_[PROCESS] = [this]() { RenderProcessWnd(); };
    viewRenderers_[MEM] = [this]() { RenderProcessWnd(); };
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
            char selectableId[64];
            sprintf_s(selectableId, "%s##%d", proc.szExeFile, row);
            if (ImGui::Selectable(selectableId, is_selected, ImGuiSelectableFlags_SpanAllColumns)) {
                selected_index = row;
            }

            // �Ҽ��˵� - ʹ��ΨһID�����ͻ
            char popupId[64];
            sprintf_s(popupId, "ProcessMenu##%d", row);
            if (ImGui::BeginPopupContextItem(popupId)) {
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

            row++;
        }
        ImGui::EndTable();
    }

    // ��ʾ��������
    ImGui::Text(u8"��������: %d", (int)processList.size());
}

void ProcessWnd::RenderMemWnd(DWORD pid)
{
    ImGui::SetNextWindowSize(ImVec2(700, 500), ImGuiCond_FirstUseEver);
    ImGui::Begin(u8"�ڴ��д", &showMemoryWindow_);
    
    ImGui::Text(u8"Ŀ�����ID: %u", pid);
    ImGui::Separator();
    
    // �ڴ��д��������
    ImGui::Text(u8"����ID:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::InputText("##ProcessId", processIdText_, sizeof(processIdText_));
    ImGui::SameLine();
    ImGui::Text(u8"��ַ:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::InputText("##Address", addressText_, sizeof(addressText_));
    ImGui::SameLine();
    ImGui::Text(u8"��С:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    ImGui::InputText("##Size", sizeText_, sizeof(sizeText_));

    if (ImGui::Button(u8"���Զ�")) {
        ULONG processId = strtoul(processIdText_, NULL, 10);
        ULONG address = strtoul(addressText_, NULL, 16);
        DWORD size = strtoul(sizeText_, NULL, 10);

        if (ctx_->arkR3.AttachReadMem(processId, address, size)) {
            ctx_->arkR3.Log("��ȡ�ڴ�ɹ���PID=%d, ��ַ=0x%08X, ��С=%d�ֽ�", processId, address, size);
            
            // ���ڲ���������ȡ����
            PVOID pData = ctx_->arkR3.GetBufferData();
            DWORD dataSize = ctx_->arkR3.GetDataSize();
            
            // ��ʾǰ16�ֽ�����Ԥ��
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
                
                ctx_->arkR3.Log("����Ԥ����%s", previewBuffer);
            }
            
            // ʹ�ø�ʽ����ʮ��������ʾ��16�ֽ�һ�У�
            std::string formattedHexStr = ctx_->arkR3.GetDataAsFormattedHexString();
            if (formattedHexStr.length() > 1020) formattedHexStr = formattedHexStr.substr(0, 1020); 
            strcpy(writeDataText_, formattedHexStr.c_str());
            
        } else {
            ctx_->arkR3.Log("��ȡ�ڴ�ʧ�ܣ�PID=%d, ��ַ=0x%08X, ��С=%d", processId, address, size);
        }
    }

    ImGui::SameLine();

    if (ImGui::Button(u8"д���ڴ�")) {
        DWORD processId = strtoul(processIdText_, NULL, 10);
        ULONG address = strtoul(addressText_, NULL, 16);
        
        if (processId != 0 && address != 0 && strlen(writeDataText_) > 0) {
            // ʹ���µ�API���Ƚ�ʮ�������ַ������õ��ڲ�������
            if (ctx_->arkR3.SetWriteDataFromHex(writeDataText_)) {
                DWORD dataSize = ctx_->arkR3.GetDataSize();
                
                // ִ��д��
                if (ctx_->arkR3.AttachWriteMem(processId, address, dataSize)) {
                    ctx_->arkR3.Log("д���ڴ�ɹ���PID=%d, ��ַ=0x%08X, ��С=%d�ֽ�", processId, address, dataSize);
                } else {
                    ctx_->arkR3.Log("д���ڴ�ʧ�ܣ�PID=%d, ��ַ=0x%08X, ��С=%d", processId, address, dataSize);
                }
            } else {
                ctx_->arkR3.Log("д�����ݸ�ʽ����ʮ�������ַ�����ʽ��Ч");
            }
        } else {
            ctx_->arkR3.Log("д��ʧ�ܣ�����ID����ַ��д������Ϊ��");
        }
    }

    // д�����������
    ImGui::Text(u8"д������(ʮ������):");
    ImGui::InputTextMultiline("##WriteData", writeDataText_, sizeof(writeDataText_), ImVec2(-1, 250));

    ImGui::End();
}
