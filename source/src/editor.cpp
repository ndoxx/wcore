#include "editor.h"

#ifndef __DISABLE_EDITOR__
#include "input_handler.h"
#include "model.h"

namespace wcore
{

Editor::Editor():
scene_query_index_(0),
model_selection_(nullptr),
editing_(false),
track_cursor_(false)
{

}

Editor::~Editor()
{

}

void Editor::init_events(InputHandler& handler)
{
    subscribe("input.mouse.click"_h, handler, &Editor::onMouseEvent);
    subscribe("input.keyboard"_h, handler, &Editor::onKeyboardEvent);
}

void Editor::set_model_selection(Model* pmdl)
{
    // If previous selection remove selection reset callback
    if(model_selection_)
        model_selection_->remove_selection_reset_callback();
    // Add selection reset callback
    model_selection_ = pmdl;
        model_selection_->add_selection_reset_callback(this, &Editor::clear_selection);
}

bool Editor::onMouseEvent(const WData& data)
{
    RayCaster* ray_caster = locate<RayCaster>("RayCaster"_h);

    const MouseData& md = static_cast<const MouseData&>(data);

    // Selection
    if(md.button_pressed.test(MouseButton::LMB))
    {
        Ray ray = ray_caster->cast_ray_from_screen(math::vec2(md.dx, md.dy));
        last_scene_query_ = ray_caster->ray_scene_query(ray);

        if(last_scene_query_.hit)
        {
            set_model_selection(last_scene_query_.models[0]);
            return false; // Consume event
        }
        return true; // Do NOT consume event
    }
    // Advance selection to next model in line
    if(md.button_pressed.test(MouseButton::RMB))
    {
        if(!last_scene_query_.hit) return true;
        scene_query_index_ = (scene_query_index_+1)%last_scene_query_.models.size();
        set_model_selection(last_scene_query_.models[scene_query_index_]);
        return false; // Consume event
    }

    return true; // Do NOT consume event
}

bool Editor::onKeyboardEvent(const WData& data)
{
    const KbdData& kbd = static_cast<const KbdData&>(data);

    switch(kbd.key_binding)
    {
        case "k_editor_deselect"_h:
            clear_selection();
            break;
    }

    return true; // Do NOT consume event
}

} // namespace wcore

#endif
