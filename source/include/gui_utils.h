#ifndef GUI_UTILS_H
#define GUI_UTILS_H

#include <limits>

namespace ImGui
{
// Plot value over time
// Pass FLT_MAX value to draw without adding a new value
void PlotVar(const char* label,
             float value,
             float scale_min = std::numeric_limits<float>::max(),
             float scale_max = std::numeric_limits<float>::max(),
             size_t buffer_size = 200);

// Call this periodically to discard old/unused data
void PlotVarFlushOldEntries();
}

#endif // GUI_UTILS_H
