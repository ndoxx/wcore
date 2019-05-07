#ifndef GEOMETRY_RENDERER_H
#define GEOMETRY_RENDERER_H

#include "renderer.h"
#include "shader.h"

namespace wcore
{

struct Vertex3P3N3T2U;
class Scene;
class Camera;
class GeometryRenderer : public Renderer
{
private:
    Shader geometry_pass_shader_;
    Shader terrain_shader_;
    Shader null_shader_;

    // Rendering data
    float wireframe_mix_;
    float min_parallax_distance_;

    // Configuration overrides
    bool allow_normal_mapping_;
    bool allow_parallax_mapping_;

public:
    GeometryRenderer();
    virtual ~GeometryRenderer() = default;

    virtual void render(Scene* pscene) override;

    inline void set_wireframe_mix(float value) { wireframe_mix_ = value; }
    inline float get_wireframe_mix() const     { return wireframe_mix_; }
    inline float& get_wireframe_mix_nc()       { return wireframe_mix_; }
    inline float get_min_parallax_distance() const { return min_parallax_distance_; }
    inline float& get_min_parallax_distance_nc()   { return min_parallax_distance_; }

    inline void toggle_wireframe();
};

inline void GeometryRenderer::toggle_wireframe()
{
    if(wireframe_mix_<1e-3)
        wireframe_mix_ = 1.0f;
    else
        wireframe_mix_ = 0.0f;
}

}

#endif // GEOMETRY_RENDERER_H
