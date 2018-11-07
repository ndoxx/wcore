#ifndef FORWARD_RENDERER_H
#define FORWARD_RENDERER_H

#include "renderer.hpp"
#include "shader.h"

struct Vertex3P3N3T2U;
class GBuffer;
class LBuffer;
class Camera;
class ForwardRenderer : public Renderer<Vertex3P3N3T2U>
{
private:
    Shader forward_stage_shader_;

public:
    ForwardRenderer();
    virtual ~ForwardRenderer() = default;

    void load_geometry();
    virtual void render() override;
};

#endif // FORWARD_RENDERER_H
