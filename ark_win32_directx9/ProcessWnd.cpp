#include "ProcessWnd.h"
#include  "ModuleWnd.h"
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
    
    // ��Ⱦ����ģ�鴰�ڣ������Ҫ��ʾ��
    if (ctx_->showProcessModuleWnd) {
        RenderProcessModuleWnd();
    }
}

void ProcessWnd::RenderProcessWnd() {

    if (ImGui::Button(u8"ˢ��")) {
        // ��ȡ�����б�
       /* processList = ctx_->arkR3.EnumProcesses32();*/
        DWORD count = ctx_->arkR3.ProcessGetCount();
        ctx_->arkR3.Log("����%d", count);
        ctx_->processUiVec = ctx_->arkR3.ProcessGetVec(count);
    }
    ImGui::Separator();

    if (ImGui::BeginTable("proc_table", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn(u8"������");           
        ImGui::TableSetupColumn(u8"����ID");           
        ImGui::TableSetupColumn(u8"��PID");            
        ImGui::TableSetupColumn(u8"ҳĿ¼��ַ");       
        ImGui::TableSetupColumn(u8"EPROCESS��ַ");     
        ImGui::TableHeadersRow();

        static int selected_index = -1;
        int row = 0;

        for (const auto& process : ctx_->processUiVec) { 
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
                    ctx_->targetPid_ = process.ProcessId;
                    sprintf_s(ctx_->processIdText_, "%u", process.ProcessId);
                    ctx_->showMemoryWindow_ = true;
                }
                if (ImGui::MenuItem(u8"�鿴ģ��")) {
                    ctx_->moduleTargetPid = process.ProcessId;
                    sprintf_s(ctx_->moduleTargetProcessName, "%s", process.ImageFileName);
                    ctx_->showProcessModuleWnd = true;
                    
                    // ���ؽ���ģ��
                    DWORD moduleCount = ctx_->arkR3.ProcessModuleGetCount(ctx_->moduleTargetPid);
                    ctx_->processModuleUiVec = ctx_->arkR3.ProcessModuleGetVec(ctx_->moduleTargetPid, moduleCount);

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

            row++;
        }
        ImGui::EndTable();
    }

    ImGui::Text(u8"��������: %d", (int)ctx_->processUiVec.size());

    // ��ʾ�ڴ��������
    if (ctx_->showMemoryWindow_) {
        RenderMemWnd(ctx_->targetPid_);
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
    ImGui::InputText("##Address", ctx_->addressText_, sizeof(ctx_->addressText_));
    ImGui::SameLine();
    ImGui::Text(u8"��С:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    ImGui::InputText("##Size", ctx_->sizeText_, sizeof(ctx_->sizeText_));

        if (ImGui::Button(u8"��")) {
        ULONG processId = strtoul(ctx_->processIdText_, NULL, 10);
        ULONG address = strtoul(ctx_->addressText_, NULL, 16);
        DWORD size = strtoul(ctx_->sizeText_, NULL, 10);

        ctx_->arkR3.MemAttachRead(processId, address, size);
    }

    ImGui::SameLine();

    if (ImGui::Button(u8"д")) {
        DWORD processId = strtoul(ctx_->processIdText_, NULL, 10);
        ULONG address = strtoul(ctx_->addressText_, NULL, 16);
        
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
        ULONG address = strtoul(ctx_->addressText_, NULL, 16);
        mem_edit.DrawContents(pData, dataSize, address);
    } else {
        ImGui::Text(u8"���ȶ�ȡ�ڴ�����");
    }

    ImGui::End();
}

// ��Ⱦ����ģ�鴰��
void ProcessWnd::RenderProcessModuleWnd()
{
    char windowTitle[128];
    sprintf_s(windowTitle, u8"����ģ�� - [%d] %s###ProcessModule", ctx_->moduleTargetPid, ctx_->moduleTargetProcessName);
    
    if (ImGui::Begin(windowTitle, &ctx_->showProcessModuleWnd)) {
        // ˢ�°�ť
        if (ImGui::Button(u8"ˢ��ģ��")) {
            DWORD moduleCount = ctx_->arkR3.ProcessModuleGetCount(ctx_->moduleTargetPid);
            ctx_->processModuleUiVec = ctx_->arkR3.ProcessModuleGetVec(ctx_->moduleTargetPid, moduleCount);
        }
        
        ImGui::SameLine();
        ImGui::Text(u8"ģ������: %d", (int)ctx_->processModuleUiVec.size());
        
        ImGui::Separator();
        
        // ֱ�ӵ��þ�̬����
        char tableTitle[64];
        sprintf_s(tableTitle, u8"����%dģ���б�", ctx_->moduleTargetPid);
        ModuleWnd::RenderModuleTable(ctx_->processModuleUiVec, tableTitle);
    }
    
    ImGui::End();
}
