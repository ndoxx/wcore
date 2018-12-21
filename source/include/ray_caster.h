#ifndef RAY_CASTER_H
#define RAY_CASTER_H

#include "updatable.h"
#include "listener.h"
#include "math3d.h"

namespace wcore
{

class RenderPipeline;

/*
    The RayCaster system is used to cast rays into the scene and return
    objects in the path of these rays.
*/
class RayCaster: public Updatable, public Listener
{
public:
    RayCaster(RenderPipeline& pipeline);

    virtual void update(const GameClock& clock) override;

    void onMouseEvent(const WData& data);
    void cast_ray_from_screen(const math::vec2& screen_coords);

private:
    RenderPipeline& pipeline_;
    math::mat4 unproj_;
    math::vec4 eye_pos_world_;
};

} // namespace wcore

#endif // RAY_CASTER_H
