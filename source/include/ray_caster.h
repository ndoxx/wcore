#ifndef RAY_CASTER_H
#define RAY_CASTER_H

#include "game_system.h"
#include "math3d.h"
#include "ray.h"

namespace wcore
{

class RenderPipeline;

/*
    The RayCaster system is used to cast rays into the scene and return
    objects in the path of these rays.
*/
class RayCaster: public GameSystem
{
public:
    RayCaster();

    virtual void update(const GameClock& clock) override;
    // Initialize event listener
    virtual void init_events(InputHandler& handler) override;

    void onMouseEvent(const WData& data);
    Ray cast_ray_from_screen(const math::vec2& screen_coords);
    void ray_scene_query(const Ray& ray);

private:
    math::mat4 unproj_;
    math::vec4 eye_pos_world_;
};

} // namespace wcore

#endif // RAY_CASTER_H
