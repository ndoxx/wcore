#include <sstream>

#include "ray_caster.h"
#include "pipeline.h"
#include "scene.h"
#include "camera.h"
#include "ray.h"
#include "logger.h"

namespace wcore
{

RayCaster::RayCaster(RenderPipeline& pipeline):
pipeline_(pipeline)
{

}

void RayCaster::onMouseEvent(const WData& data)
{
    const MouseData& md = static_cast<const MouseData&>(data);

    DLOG(md.to_string(), "core", Severity::LOW);
    cast_ray_from_screen(math::vec2(md.dx, md.dy));
}

void RayCaster::update(const GameClock& clock)
{
    // Get camera view-projection matrix for this frame and invert it
    pCamera cam = SCENE.get_camera();
    const math::mat4& view = cam->get_view_matrix();
    //const math::mat4& model = cam->get_model_matrix();
    const math::mat4& projection = cam->get_projection_matrix();
    eye_pos_world_ = cam->get_position();
    eye_pos_world_[3] = 1.0f;
    math::mat4 VP(projection*view);
    math::inverse(VP, unproj_);

    /*math::mat4 pinv;
    math::inverse(projection, pinv);
    unproj_ = model * pinv;*/
}

void RayCaster::cast_ray_from_screen(const math::vec2& screen_coords)
{
    std::stringstream ss;

    math::vec4 coords(screen_coords);
    coords *= 2.0f;
    coords -= 1.0f;
    coords[2] = -1.0f;
    coords[3] = 1.0f;

    ss << "NDC: " << coords;
    DLOGI(ss.str(), "core", Severity::LOW);
    ss.str("");

    math::vec4 wcoords(unproj_*coords);
    wcoords /= wcoords.w();
    math::vec4 direction(wcoords-eye_pos_world_);
    direction.normalize();

    ss << "World: " << wcoords;
    DLOGI(ss.str(), "core", Severity::LOW);

    pipeline_.debug_draw_segment(wcoords.xyz(), (wcoords+100*direction).xyz(), 100*60, math::vec3(0,1,0));
}



} // namespace wcore
