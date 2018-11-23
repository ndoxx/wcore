#include "debug_info.h"
#include "text_renderer.h"
#include "globals.h"

namespace wcore
{

DebugInfo::DebugInfo():
text_renderer_(nullptr),
active_(false)
{

}

void DebugInfo::register_text_renderer(TextRenderer* prenderer)
{
    text_renderer_ = prenderer;
}

void DebugInfo::register_text_slot(hash_t slot_name, const math::vec3& color)
{
    slots_.insert(std::make_pair(slot_name, slots_.size()));
    colors_.insert(std::make_pair(slot_name, color));
}

void DebugInfo::display(uint8_t index, const std::string& text, const math::vec3& color)
{
    if(text_renderer_ && active_)
    {
        float yy = GLB.SCR_H-20-index*18;
        text_renderer_->schedule_for_drawing(text, H_("arial"), 10, yy, 1.0f, color);
    }
}

}
