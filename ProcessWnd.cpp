#include "ProcessWnd.h"

void ProcessWnd::Render(bool* p_open)
{
    ImGui::Begin(u8"进程", p_open);
    if (ImGui::BeginTable("proc_table", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn(u8"进程名");
        ImGui::TableSetupColumn(u8"PID");
        ImGui::TableSetupColumn(u8"父PID");
        ImGui::TableSetupColumn(u8"CPU");
        ImGui::TableSetupColumn(u8"路径");
        ImGui::TableHeadersRow();

        static int selected_index = -1;
        int row = 0;
        for (const auto& proc : ctx_->list) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            // 整行选中
            if (ImGui::Selectable(proc.Name, selected_index == row, ImGuiSelectableFlags_SpanAllColumns)) {
                selected_index = row;
            }
            ImGui::TableSetColumnIndex(1); ImGui::Text("%u", proc.Id);
            ImGui::TableSetColumnIndex(2); ImGui::Text("%u", proc.ParentId);
            ImGui::TableSetColumnIndex(3); ImGui::Text("%.2f", proc.Cpu);
            ImGui::TableSetColumnIndex(4); ImGui::Text("%s", proc.Path);
            row++;
        }
        ImGui::EndTable();
    }

    if (ImGui::Button(u8"刷新")) {
        ctx_->arkR3.GetProcessInfo(60);
    }
    ImGui::End();





}
