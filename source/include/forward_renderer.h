#ifndef FORWARD_RENDERER_H
#define FORWARD_RENDERER_H

#include "renderer.hpp"
#include "shader.h"

namespace wcore
{

class GBuffer;
class LBuffer;
class Camera;
class ForwardRenderer : public Renderer
{
private:
    Shader forward_stage_shader_;
    Shader skybox_shader_;

public:
    bool active_;

    ForwardRenderer();
    virtual ~ForwardRenderer() = default;

    void load_geometry();
    virtual void render(Scene* pscene) override;
};

}

#endif // FORWARD_RENDERER_H
