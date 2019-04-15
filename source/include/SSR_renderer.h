#ifndef SSR_RENDERER_H
#define SSR_RENDERER_H

#include "renderer.hpp"
#include "shader.h"

namespace wcore
{

class SSRRenderer : public Renderer<Vertex3P>
{
public:
    SSRRenderer();
    virtual ~SSRRenderer();

    virtual void render(Scene* pscene) override;

    inline void toggle()                { enabled_ = !enabled_; }
    inline void set_enabled(bool value) { enabled_ = value; }
    inline bool is_enabled() const      { return enabled_; }
    inline bool& get_enabled()          { return enabled_; }

private:
    Shader SSR_shader_;
    bool enabled_;
};

} // namespace wcore

#endif
