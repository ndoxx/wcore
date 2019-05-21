#ifndef RENDERER_HPP
#define RENDERER_HPP

#ifdef __PROFILE__
    #include "moving_average.h"
    #include "gpu_query_timer.h"
#endif

namespace wcore
{

class Scene;
class Renderer
{
public:
    Renderer();

    inline void set_enabled(bool value) { enabled_ = value; }
    inline bool is_enabled() const      { return enabled_; }
    inline bool& get_enabled()          { return enabled_; }
    inline void toggle()                { enabled_ = !enabled_; }

#ifdef __PROFILE__
    void Render(Scene* pscene);
    inline float last_dt() const { return dt_fifo_.last_element(); }
    FinalStatistics get_stats()  { return dt_fifo_.get_stats(); }
#else
    inline void Render(Scene* pscene) { if(enabled_) render(pscene); }
#endif

protected:
    virtual void render(Scene* pscene) = 0;

#ifdef __PROFILE__
public:
    static bool PROFILING_ACTIVE;
#endif

protected:
    bool enabled_;

private:
#ifdef __PROFILE__
    GPUQueryTimer query_timer_;
    MovingAverage dt_fifo_;
#endif
};

}

#endif // RENDERER_HPP
