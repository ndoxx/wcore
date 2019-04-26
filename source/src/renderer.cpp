#include "renderer.h"
#include "gfx_driver.h"

namespace wcore
{

#ifdef __PROFILE__
#define PROFILING_MAX_SAMPLES 1000
bool Renderer::PROFILING_ACTIVE = false;
nanoClock Renderer::profile_clock_;
#endif

Renderer::Renderer():
enabled_(true)
#ifdef __PROFILE__
, dt_fifo_(PROFILING_MAX_SAMPLES)
#endif
{

}

#ifdef __PROFILE__
void Renderer::Render(Scene* pscene)
{
    if(!enabled_)
        return;

    float dt = 0.0f;
    std::chrono::nanoseconds period;

    if(PROFILING_ACTIVE)
    {
        GFX::finish();
        profile_clock_.restart();
    }

    render(pscene);

    if(PROFILING_ACTIVE)
    {
        GFX::finish();
        period = profile_clock_.get_elapsed_time();
        dt = std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
        dt_fifo_.push(dt);
    }
}
#endif


} // namespace wcore
