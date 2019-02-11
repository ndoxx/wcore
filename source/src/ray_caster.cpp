#include <sstream>

#include "ray_caster.h"
#include "pipeline.h"
#include "scene.h"
#include "input_handler.h"
#include "model.h"
#include "camera.h"
#include "bounding_boxes.h"
#include "config.h"
#include "logger.h"
#ifndef __DISABLE_EDITOR__
#include "editor.h"
#endif

namespace wcore
{

#ifdef __DEBUG__
static bool show_ray = false;
static int ray_persistence = 0;
#endif

RayCaster::RayCaster()
{
#ifdef __DEBUG__
    CONFIG.get("root.debug.raycast.geometry.show_on_click"_h, show_ray);
    CONFIG.get("root.debug.raycast.geometry.persistence"_h, ray_persistence);
#endif
}

void RayCaster::update(const GameClock& clock)
{

    // * Get camera view-projection matrix for this frame and invert it
    auto& cam = locate<Scene>("Scene"_h)->get_camera();
    const math::mat4& view = cam.get_view_matrix();
    const math::mat4& projection = cam.get_projection_matrix();
    eye_pos_world_ = cam.get_position(); // Also save camera position
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
    coords_near[2] = 1.0f; // screen is the near plane
    coords_near[3] = 1.0f;
    math::vec4 coords_far(coords_near);
    coords_near[2] = -1.0f; // far plane

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
    ss << "Origin (world): " << ray.origin_w;
    DLOGI(ss.str(), "collision", Severity::LOW);
    ss.str("");

    ss << "End (world): " << ray.end_w;
    DLOGI(ss.str(), "collision", Severity::LOW);
    ss.str("");

    ss << "Direction: " << ray.direction;
    DLOGI(ss.str(), "collision", Severity::LOW);
    ss.str("");

    if(show_ray)
    {
        locate<RenderPipeline>("Pipeline"_h)->debug_draw_segment(ray.origin_w,
                                                ray.end_w,
                                                ray_persistence,
                                                math::vec3(1,0.2,0));
    }
#endif

    return ray;
}

SceneQueryResult RayCaster::ray_scene_query(const Ray& ray)
{
    Scene* pscene             = locate<Scene>("Scene"_h);
    RenderPipeline* ppipeline = locate<RenderPipeline>("Pipeline"_h);

    // * Perform ray/AABB intersection test with objects in view frustum
    //   and return the closest object or nothing
    RayCollisionData data;
    SceneQueryResult result;
    pscene->traverse_models([&](Model& model, uint32_t chunk_id)
    {
        #ifdef __DEBUG__
        if(show_ray)
        {
            math::vec3 near_intersection(ray.origin_w + (ray.direction*data.near));
            math::vec3 far_intersection(ray.origin_w + (ray.direction*data.far));
            ppipeline->debug_draw_cross3(near_intersection,
                                         0.3f,
                                         ray_persistence,
                                         math::vec3(0,0.7f,1));
            ppipeline->debug_draw_cross3(far_intersection,
                                         0.3f,
                                         ray_persistence,
                                         math::vec3(1,0.7f,0));
        }
        #endif

        result.hit = true;
        result.models.push_back(&model);
    },
    [&](const Model& model) // Evaluator -> breaks from traversal loop when return value is false
    {
        // Skip terrains for now
        if(model.is_terrain() || !model.is_visible())
            return false;
        //return ray_collides_AABB(ray, model.get_AABB(), data);
        return ray_collides_OBB(ray, model, data);
    },
    ORDER::FRONT_TO_BACK);
    return result;
}

SceneQueryResult RayCaster::ray_scene_query_first(const Ray& ray)
{
    Scene* pscene             = locate<Scene>("Scene"_h);
    RenderPipeline* ppipeline = locate<RenderPipeline>("Pipeline"_h);

    // * Perform ray/AABB intersection test with objects in view frustum
    //   and return the closest object or nothing
    RayCollisionData data;
    SceneQueryResult result;
    pscene->visit_model_first([&](Model& model, uint32_t chunk_id)
    {
        #ifdef __DEBUG__
        if(show_ray)
        {
            math::vec3 near_intersection(ray.origin_w + (ray.direction*data.near));
            math::vec3 far_intersection(ray.origin_w + (ray.direction*data.far));
            ppipeline->debug_draw_cross3(near_intersection,
                                         0.3f,
                                         ray_persistence,
                                         math::vec3(0,0.7f,1));
            ppipeline->debug_draw_cross3(far_intersection,
                                         0.3f,
                                         ray_persistence,
                                         math::vec3(1,0.7f,0));
        }
        #endif

        result.hit = true;
        result.models.push_back(&model);
    },
    [&](const Model& model) // Evaluator -> breaks from traversal loop when return value is false
    {
        // Skip terrains for now
        if(model.is_terrain() || !model.is_visible())
            return false;
        //return ray_collides_AABB(ray, model.get_AABB(), data);
        return ray_collides_OBB(ray, model, data);
    });
    return result;
}



} // namespace wcore
