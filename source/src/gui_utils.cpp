#include <map>
#include <cmath>
#include <iostream>

#include "gui_utils.h"
#include "imgui/imgui.h"

namespace ImGui
{
struct PlotVarData
{
    ImGuiID        ID;
    ImVector<float>  Data;
    int            DataInsertIdx;
    int            LastFrame;

    PlotVarData() : ID(0), DataInsertIdx(0), LastFrame(-1) {}
};

typedef std::map<ImGuiID, PlotVarData> PlotVarsMap;
static PlotVarsMap  g_PlotVarsMap;

// Plot value over time
// Call with 'value == FLT_MAX' to draw without adding new value to the buffer
void PlotVar(const char* label, float value, float scale_min, float scale_max, size_t buffer_size)
{
    IM_ASSERT(label);
    if (buffer_size == 0)
        buffer_size = 120;

    ImGui::PushID(label);
    ImGuiID id = ImGui::GetID("");

    // Lookup O(log N)
    PlotVarData& pvd = g_PlotVarsMap[id];

    // Setup
    if (pvd.Data.capacity() != buffer_size)
    {
        pvd.Data.resize(buffer_size);
        memset(&pvd.Data[0], 0, sizeof(float) * buffer_size);
        pvd.DataInsertIdx = 0;
        pvd.LastFrame = -1;
    }

    // Insert (avoid unnecessary modulo operator)
    if (pvd.DataInsertIdx == buffer_size)
        pvd.DataInsertIdx = 0;
    int display_idx = pvd.DataInsertIdx;
    if (value != FLT_MAX)
        pvd.Data[pvd.DataInsertIdx++] = value;

    // Draw
    int current_frame = ImGui::GetFrameCount();
    if (pvd.LastFrame != current_frame)
    {
        ImGui::PlotLines("##plot", &pvd.Data[0], buffer_size, pvd.DataInsertIdx, NULL, scale_min, scale_max, ImVec2(0, 40));
        ImGui::SameLine();
        ImGui::Text("%s\n%-3.4f", label, pvd.Data[display_idx]);    // Display last value in buffer
        pvd.LastFrame = current_frame;
    }

    ImGui::PopID();
}

void PlotVarFlushOldEntries()
{
    int current_frame = ImGui::GetFrameCount();
    for (PlotVarsMap::iterator it = g_PlotVarsMap.begin(); it != g_PlotVarsMap.end(); )
    {
        PlotVarData& pvd = it->second;
        if (pvd.LastFrame < current_frame - fmax(400,(int)pvd.Data.size()))
            it = g_PlotVarsMap.erase(it);
        else
            ++it;
    }
}

void WCombo(const char* combo_name, const char* text, int& current_index, int nitems, const char** items)
{
    ImGuiComboFlags flags = ImGuiComboFlags_NoArrowButton;

    ImGuiStyle& style = ImGui::GetStyle();
    float w = ImGui::CalcItemWidth();
    float spacing = style.ItemInnerSpacing.x;
    float button_sz = ImGui::GetFrameHeight();
    ImGui::PushItemWidth(w - spacing * 2.0f - button_sz * 2.0f);
    if (ImGui::BeginCombo(combo_name, items[current_index], ImGuiComboFlags_NoArrowButton))
    {
        for (int n = 0; n < nitems; ++n)
        {
            bool is_selected = (current_index == n);
            if (ImGui::Selectable(items[n], is_selected))
                current_index = n;
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    ImGui::SameLine(0, spacing);
    if (ImGui::ArrowButton("##l", ImGuiDir_Left))
    {
        --current_index;
        if(current_index<0)
            current_index = nitems-1;
    }
    ImGui::SameLine(0, spacing);
    if (ImGui::ArrowButton("##r", ImGuiDir_Right))
    {
        ++current_index;
        if(current_index>nitems-1)
            current_index = 0;
    }
    ImGui::SameLine(0, style.ItemInnerSpacing.x);
    ImGui::Text(text);
}

}
