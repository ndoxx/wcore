#include "renderer.h"
#include "gfx_driver.h"

namespace wcore
{

#ifdef __PROFILE__
#define PROFILING_MAX_SAMPLES 1000
bool Renderer::PROFILING_ACTIVE = false;
#endif

Renderer::Renderer():
enabled_(true)
#ifdef __PROFILE__
, query_timer_()
, dt_fifo_(PROFILING_MAX_SAMPLES)
#endif
{

}

#ifdef __PROFILE__
void Renderer::Render(Scene* pscene)
{
    if(!enabled_)
        return;

    if(PROFILING_ACTIVE)
        query_timer_.start();

    render(pscene);

    if(PROFILING_ACTIVE)
        dt_fifo_.push(query_timer_.stop());
}
#endif


} // namespace wcore
