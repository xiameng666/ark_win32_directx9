#include "KernelWnd.h"


KernelWnd::KernelWnd(Context* ctx)
    : ImguiWnd(ctx)
{
    viewRenderers_[GDT] = [this]() { RenderGDTTable(); };
    viewRenderers_[IDT] = [this]() { RenderIDTTable(); };
}

void KernelWnd::Render(bool* p_open)
{
    ImGui::Begin(u8"�ں�", p_open);

    ImVec2 available = ImGui::GetContentRegionAvail();

    if (ImGui::BeginTable("KernelLayout", 2,
        ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable,
        available)){
        ImGui::TableSetupColumn(u8"����", ImGuiTableColumnFlags_WidthFixed,200.0f);
        ImGui::TableSetupColumn(u8"����", ImGuiTableColumnFlags_WidthStretch,800.0f);
        ImGui::TableNextRow();

        RenderLeftBar();
        ImGui::TableSetColumnIndex(1);
        auto it = viewRenderers_.find(currentView_);
        if (it != viewRenderers_.end()) {
            it->second();
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

void KernelWnd::RenderGDTTable()
{
    static std::vector<GDT_INFO> gdtData;
    static bool filterInvalidSegments = true;  // Ĭ�Ϲ�����Ч��
    
    if (ImGui::Button(u8"ˢ��")) {
        gdtData = ctx_->arkR3.GetGDTVec();
    }
    
    ImGui::SameLine();
    ImGui::Checkbox(u8"������Ч��", &filterInvalidSegments);
    
    if (filterInvalidSegments) {
        ImGui::SameLine();
        ImGui::TextColored(COLOR_INFO, u8"(���� Limit=0 �� NP �Ķ�)");
    }

    if (ImGui::BeginTable(u8"GDT", 8, 
        ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable |
        ImGuiTableFlags_Sortable)) {
    
        // �б�ͷ
        ImGui::TableSetupColumn(u8"CPU");
        ImGui::TableSetupColumn(u8"Sel");
        ImGui::TableSetupColumn(u8"Base");
        ImGui::TableSetupColumn(u8"Limit");
        ImGui::TableSetupColumn(u8"����");
        ImGui::TableSetupColumn(u8"��Ȩ��");
        ImGui::TableSetupColumn(u8"Present");
        ImGui::TableSetupColumn(u8"Type");

        ImGui::TableHeadersRow();
    
        int validCount = 0;
        int totalCount = 0;
        
        for (const auto& gdt : gdtData) {
            totalCount++;
            
            if (filterInvalidSegments) {
                // ����
                if (gdt.limit == 0 && !gdt.p) {
                    continue;
                }
            }
            
            validCount++;
            ImGui::TableNextRow();
    
            // CPU���
            ImGui::TableNextColumn();
            ImGui::Text("%d", gdt.cpuIndex);
    
            // ��ѡ����
            ImGui::TableNextColumn();
            ImGui::Text("0x%04X", gdt.selector);
    
            // ��ַ
            ImGui::TableNextColumn();
            ImGui::Text("0x%08X", gdt.base);
    
            // ����
            ImGui::TableNextColumn();

            if (gdt.limit == 0) {
                ImGui::TextColored(COLOR_LIGHT_GRAY, "0x%04X", gdt.limit);
            } else {
                ImGui::Text("0x%04X", gdt.limit);
            }
    
            // ������
            ImGui::TableNextColumn();
            ImGui::Text("%s", gdt.g ? u8"PAGE" : u8"BYTE");
    
            // ����Ȩ��
            ImGui::TableNextColumn();
            ImVec4 dplColor;
            switch (gdt.dpl) {
            case 0: dplColor = COLOR_RED; break;    
            case 3: dplColor = COLOR_GREEN; break;  
            default: dplColor = COLOR_YELLOW; break;
            }
            ImGui::TextColored(dplColor, "%d", gdt.dpl);

            // Present
            ImGui::TableNextColumn();
            if (gdt.p) {
                ImGui::TextColored(COLOR_GREEN, "P");
            } else {
                ImGui::TextColored(COLOR_GRAY, "NP");
            }

            // Type 
            ImGui::TableNextColumn();
            ImGui::Text("0x%X", gdt.type);
        }
    
        ImGui::EndTable();
        
        // ��ʾͳ����Ϣ
        if (filterInvalidSegments) {
            ImGui::Text(u8"��ʾ %d/%d ����Ч�������� (������ %d ����Ч��)", 
                       validCount, totalCount, totalCount - validCount);
        } else {
            ImGui::Text(u8"��ʾȫ�� %d ����������", totalCount);
        }
    }
}

void KernelWnd::RenderIDTTable()
{
}

void KernelWnd::RenderLeftBar()
{
    ImGui::TableSetColumnIndex(0);
    ImGui::Text(u8"�ں�ģ��");
    ImGui::Separator();
    
    NAV_SECTION(u8"�ڴ����",
        VIEW_ITEM(u8"GDT", GDT);
        VIEW_ITEM(u8"IDT", IDT);
    );
}


