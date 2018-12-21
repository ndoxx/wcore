#include <sstream>

#include "ray_caster.h"
#include "pipeline.h"
#include "scene.h"
#include "model.h"
#include "camera.h"
#include "bounding_boxes.h"
#include "config.h"
#include "logger.h"

namespace wcore
{

#ifdef __DEBUG__
static bool show_ray = false;
static int ray_persistence = 0;
#endif

RayCaster::RayCaster(RenderPipeline& pipeline):
pipeline_(pipeline)
{
#ifdef __DEBUG__
    CONFIG.get(H_("root.debug.raycast.geometry.show_on_click"), show_ray);
    CONFIG.get(H_("root.debug.raycast.geometry.persistence"), ray_persistence);
#endif
}

void RayCaster::onMouseEvent(const WData& data)
{
    const MouseData& md = static_cast<const MouseData&>(data);

    Ray ray = cast_ray_from_screen(math::vec2(md.dx, md.dy));
    ray_scene_query(ray);
}

void RayCaster::update(const GameClock& clock)
{
    // * Get camera view-projection matrix for this frame and invert it
    pCamera cam = SCENE.get_camera();
    const math::mat4& view = cam->get_view_matrix();
    const math::mat4& projection = cam->get_projection_matrix();
    eye_pos_world_ = cam->get_position(); // Also save camera position
    eye_pos_world_[3] = 1.0f;
    math::mat4 VP(projection*view);
    math::inverse(VP, unproj_);
}

Ray RayCaster::cast_ray_from_screen(const math::vec2& screen_coords)
{
    // * Compute ray in world space

#ifdef __DEBUG__
    DLOGN("Casting ray:", "collision", Severity::LOW);
    std::stringstream ss;
    ss << "Screen: " << screen_coords;
    DLOGI(ss.str(), "collision", Severity::LOW);
    ss.str("");
#endif

    // Convert to NDC coordinates
    math::vec4 coords_near(screen_coords);
    coords_near *= 2.0f;
    coords_near -= 1.0f;
    coords_near[2] = -1.0f; // screen is the near plane (z=-1)
    coords_near[3] = 1.0f;
    math::vec4 coords_far(coords_near);
    coords_near[2] = 1.0f; // far plane (z=1)

#ifdef __DEBUG__
    ss << "NDC: " << coords_near;
    DLOGI(ss.str(), "collision", Severity::LOW);
    ss.str("");
#endif

    // Unproject
    math::vec4 wcoords_near(unproj_*coords_near);
    wcoords_near /= wcoords_near.w();
    math::vec4 wcoords_far(unproj_*coords_far);
    wcoords_far /= wcoords_far.w();

    // Construct ray
    Ray ray(wcoords_near.xyz(), wcoords_far.xyz());

#ifdef __DEBUG__
    ss << "Near (world): " << wcoords_near;
    DLOGI(ss.str(), "collision", Severity::LOW);
    ss.str("");

    ss << "Far (world): " << wcoords_far;
    DLOGI(ss.str(), "collision", Severity::LOW);
    ss.str("");

    // Compute direction from camera position to this point
    math::vec4 direction(wcoords_near-wcoords_far);
    direction.normalize();

    ss << "Direction: " << direction;
    DLOGI(ss.str(), "collision", Severity::LOW);
    ss.str("");

    if(show_ray)
    {
        pipeline_.debug_draw_segment(ray.start_world,
                                     ray.end_world,
                                     ray_persistence,
                                     math::vec3(1,0,0));
    }
#endif

    return ray;
}

void RayCaster::ray_scene_query(const Ray& ray)
{
    // * Perform ray/OBB collision test with objects in view frustum
    //   and return the closest object or nothing
    uint32_t count = 0;
    SCENE.traverse_models([&](pModel pmdl, uint32_t chunk_id)
    {
        RayCollisionData data;
        if(ray_collides_AABB(ray, pmdl->get_AABB(), data))
        {
            ++count;
            std::cout << data.near << " " << data.far << std::endl;
        }
    });
    std::cout << count << std::endl;
}



} // namespace wcore
