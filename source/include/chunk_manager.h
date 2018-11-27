#ifndef CHUNK_MANAGER_H
#define CHUNK_MANAGER_H

#include "updatable.h"
#include "math3d.h"
#include "listener.h"

namespace wcore
{

//#define __OPT_CHUNK_LOAD_DIRECTION_HINT__
#define __OPT_CHUNK_LOAD_FULL_DISK__

class SceneLoader;
class InputHandler;

class ChunkManager: public Updatable, public Listener
{
private:
    SceneLoader& loader_;
    bool active_;

    uint8_t view_radius_;
#ifdef __OPT_CHUNK_LOAD_DIRECTION_HINT__
    uint8_t last_quadrant_;
#endif
#ifdef __OPT_CHUNK_LOAD_FULL_DISK__
    uint32_t last_chunk_;
#endif


public:
    ChunkManager(SceneLoader& loader);
    ~ChunkManager();

    void onKeyboardEvent(const WData& data);
    void init();
    virtual void update(const GameClock& clock) override;

    inline void toggle() { active_ = !active_; }

private:
    // calculate binary coding for chunk quadrant partition
    // given normalized centered local coordinates
    inline uint8_t quadrant(const math::vec2& lcp)
    {
        return uint8_t((lcp.x()>0) +
                       ((lcp.y()>0)<<1));
    }
};

}

#endif // CHUNK_MANAGER_H
