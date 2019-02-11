#ifndef RAY_CASTER_H
#define RAY_CASTER_H

#include <memory>

#include "game_system.h"
#include "math3d.h"
#include "ray.h"

namespace wcore
{

class RenderPipeline;
class Model;

struct SceneQueryResult
{
    SceneQueryResult():
    hit(false)
    {

    }

    inline void clear() { models.clear(); hit = false; }

    std::vector<Model*> models; // Will be a variant type when scene includes entities
    bool hit;
};

/*
    The RayCaster system is used to cast rays into the scene and return
    objects in the path of these rays.
*/
class RayCaster: public GameSystem
{
public:
    RayCaster();

    virtual void update(const GameClock& clock) override;

    Ray cast_ray_from_screen(const math::vec2& screen_coords);
    // Returns all scene objects in the path of the ray
    SceneQueryResult ray_scene_query(const Ray& ray);
    // Returns first scene object that the ray hits
    SceneQueryResult ray_scene_query_first(const Ray& ray);

private:
    math::mat4 unproj_;
    math::vec4 eye_pos_world_;
};

} // namespace wcore

#endif // RAY_CASTER_H
