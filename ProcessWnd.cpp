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
    
    
}

void ProcessWnd::RenderProcessWnd() {

    static std::vector<PROCESS_INFO> processUiVec;

    if (ImGui::Button(u8"ˢ��")) {
        // ��ȡ�����б�
       /* processList = ctx_->arkR3.EnumProcesses32();*/
        DWORD count = ctx_->arkR3.ProcessGetCount();
        ctx_->arkR3.Log("����%d", count);
        processUiVec = ctx_->arkR3.ProcessGetVec(count);
    }
    ImGui::Separator();

    if (ImGui::BeginTable("proc_table", 6, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn(u8"������");           // ��0�У���ʾ����+ѡ��
        ImGui::TableSetupColumn(u8"����ID");           // ��1��
        ImGui::TableSetupColumn(u8"��PID");            // ��2��
        ImGui::TableSetupColumn(u8"ҳĿ¼��ַ");       // ��3��
        ImGui::TableSetupColumn(u8"EPROCESS��ַ");     // ��4��
        ImGui::TableSetupColumn(u8"����·��");         // ��5��
        ImGui::TableHeadersRow();

        static int selected_index = -1;
        int row = 0;

        for (const auto& process : processUiVec) {  // �� ȷ����������ȷ
            ImGui::TableNextRow();

            // ��0�У������� + ѡ��
            ImGui::TableSetColumnIndex(0);
            bool is_selected = (selected_index == row);
            char selectableId[64];
            sprintf_s(selectableId, "%s##%d", process.ImageFileName, row);
            if (ImGui::Selectable(selectableId, is_selected, ImGuiSelectableFlags_SpanAllColumns)) {
                selected_index = row;
            }

            // �Ҽ��˵�
            char popupId[64];
            sprintf_s(popupId, "ProcessMenu##%d", row);
            if (ImGui::BeginPopupContextItem(popupId)) {
                if (ImGui::MenuItem(u8"��д�ڴ�")) {
                    targetPid_ = process.ProcessId;
                    sprintf_s(processIdText_, "%u", process.ProcessId);
                    ctx_->showMemoryWindow_ = true;
                }
                ImGui::EndPopup();
            }

            // ��1�У�����ID
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%u", process.ProcessId);

            // ��2�У���PID
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%u", process.ParentProcessId);

            // ��3�У�ҳĿ¼��ַ��ʮ�����ƣ�
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("0x%08X", process.DirectoryTableBase);

            // ��4�У�EPROCESS��ַ
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%p", process.EprocessAddr);

            // ��5�У�����·��
            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%ls", process.FullImagePath);

            row++;
        }
        ImGui::EndTable();
    }

    ImGui::Text(u8"��������: %d", (int)processUiVec.size());

    // ��ʾ�ڴ��������
    if (ctx_->showMemoryWindow_) {
        RenderMemWnd(targetPid_);
    }
}

void ProcessWnd::RenderMemWnd(DWORD pid)
{
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin(u8"�ڴ��д", &ctx_->showMemoryWindow_);
    
    ImGui::Text(u8"Ŀ�����ID: %u", pid);
    ImGui::Separator();

    ImGui::Text(u8"��ַ:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::InputText("##Address", addressText_, sizeof(addressText_));
    ImGui::SameLine();
    ImGui::Text(u8"��С:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    ImGui::InputText("##Size", sizeText_, sizeof(sizeText_));

        if (ImGui::Button(u8"��")) {
        ULONG processId = strtoul(processIdText_, NULL, 10);
        ULONG address = strtoul(addressText_, NULL, 16);
        DWORD size = strtoul(sizeText_, NULL, 10);

        ctx_->arkR3.MemAttachRead(processId, address, size);
    }

    ImGui::SameLine();

    if (ImGui::Button(u8"д")) {
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
    
    // ��ʾ�ڴ�༭��
    PVOID pData = ctx_->arkR3.GetBufferData();
    DWORD dataSize = ctx_->arkR3.GetDataSize();
    
    if (pData && dataSize > 0) {
        ImGui::Text(u8"�ڴ�༭��:");
        ULONG address = strtoul(addressText_, NULL, 16);
        mem_edit.DrawContents(pData, dataSize, address);
    } else {
        ImGui::Text(u8"���ȶ�ȡ�ڴ�����");
    }

    ImGui::End();
}
