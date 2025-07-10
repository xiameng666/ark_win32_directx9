#include "KernelWnd.h"


KernelWnd::KernelWnd(Context* ctx)
    : ImguiWnd(ctx)
{
    viewRenderers_[GDT] = [this]() { RenderGDTTable(); };
    viewRenderers_[IDT] = [this]() { RenderIDTTable(); };
}

void KernelWnd::Render(bool* p_open)
{
    ImGui::Begin(u8"内核", p_open);

    ImVec2 available = ImGui::GetContentRegionAvail();

    if (ImGui::BeginTable("KernelLayout", 2,
        ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable,
        available)){
        ImGui::TableSetupColumn(u8"导航", ImGuiTableColumnFlags_WidthFixed,200.0f);
        ImGui::TableSetupColumn(u8"内容", ImGuiTableColumnFlags_WidthStretch,800.0f);
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
    static bool filterInvalidSegments = true;  // 默认过滤无效段
    
    if (ImGui::Button(u8"刷新")) {
        gdtData = ctx_->arkR3.GetGDTVec();
    }
    
    ImGui::SameLine();
    ImGui::Checkbox(u8"过滤无效段", &filterInvalidSegments);
    
    if (filterInvalidSegments) {
        ImGui::SameLine();
        ImGui::TextColored(COLOR_INFO, u8"(隐藏 Limit=0 且 NP 的段)");
    }

    if (ImGui::BeginTable(u8"GDT", 8, 
        ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable |
        ImGuiTableFlags_Sortable)) {
    
        // 列表头
        ImGui::TableSetupColumn(u8"CPU");
        ImGui::TableSetupColumn(u8"Sel");
        ImGui::TableSetupColumn(u8"Base");
        ImGui::TableSetupColumn(u8"Limit");
        ImGui::TableSetupColumn(u8"粒度");
        ImGui::TableSetupColumn(u8"特权级");
        ImGui::TableSetupColumn(u8"Present");
        ImGui::TableSetupColumn(u8"Type");

        ImGui::TableHeadersRow();
    
        int validCount = 0;
        int totalCount = 0;
        
        for (const auto& gdt : gdtData) {
            totalCount++;
            
            if (filterInvalidSegments) {
                // 过滤
                if (gdt.limit == 0 && !gdt.p) {
                    continue;
                }
            }
            
            validCount++;
            ImGui::TableNextRow();
    
            // CPU序号
            ImGui::TableNextColumn();
            ImGui::Text("%d", gdt.cpuIndex);
    
            // 段选择子
            ImGui::TableNextColumn();
            ImGui::Text("0x%04X", gdt.selector);
    
            // 基址
            ImGui::TableNextColumn();
            ImGui::Text("0x%08X", gdt.base);
    
            // 界限
            ImGui::TableNextColumn();

            if (gdt.limit == 0) {
                ImGui::TextColored(COLOR_LIGHT_GRAY, "0x%04X", gdt.limit);
            } else {
                ImGui::Text("0x%04X", gdt.limit);
            }
    
            // 段粒度
            ImGui::TableNextColumn();
            ImGui::Text("%s", gdt.g ? u8"PAGE" : u8"BYTE");
    
            // 段特权级
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
        
        // 显示统计信息
        if (filterInvalidSegments) {
            ImGui::Text(u8"显示 %d/%d 个有效段描述符 (过滤了 %d 个无效段)", 
                       validCount, totalCount, totalCount - validCount);
        } else {
            ImGui::Text(u8"显示全部 %d 个段描述符", totalCount);
        }
    }
}

void KernelWnd::RenderIDTTable()
{
}

void KernelWnd::RenderLeftBar()
{
    ImGui::TableSetColumnIndex(0);
    ImGui::Text(u8"内核模块");
    ImGui::Separator();
    
    NAV_SECTION(u8"内存管理",
        VIEW_ITEM(u8"GDT", GDT);
        VIEW_ITEM(u8"IDT", IDT);
    );
}


