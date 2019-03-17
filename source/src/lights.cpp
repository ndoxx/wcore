#include <GL/glew.h>
#include <numeric>

#include "lights.h"
#include "logger.h"
#include "shader.h"
#include "camera.h"

namespace wcore
{

Light::Light(const math::vec3& position,
             const math::vec3& color,
             float brightness):
position_(position),
color_(color),
ambient_strength_(0.03f),
brightness_(brightness),
reference_(0),
has_reference_(false)
{

}

Light::~Light()
{

}

DirectionalLight::DirectionalLight(const math::vec3& position,
                                   const math::vec3& color,
                                   float brightness):
Light(position, color, brightness){}

DirectionalLight::~DirectionalLight(){}

void DirectionalLight::update_uniforms(const Shader& shader) const
{
    //shader.send_uniform("lt.v3_lightPosition"_h, position_);
    shader.send_uniform("lt.v3_lightColor"_h, color_*brightness_);
    shader.send_uniform("lt.f_ambientStrength"_h, ambient_strength_);
}

math::mat4 DirectionalLight::get_model_matrix(bool scaled) const
{
    math::mat4 M;
    M.init_identity();
    return M;
}

size_t DirectionalLight::get_geometry() const
{
    return Light::geom::QUAD;
}

bool DirectionalLight::is_in_frustum(const Camera& camera) const
{
    return true;
}

bool DirectionalLight::surrounds_camera(const Camera& camera) const
{
    return true;
}

uint32_t PointLight::N_INST_ = 0;

PointLight::PointLight(const math::vec3& position,
                       const math::vec3& color,
                       float radius,
                       float brightness):
Light(position, color, brightness),
radius_(radius),
index_(N_INST_++)
{

}

PointLight::~PointLight()
{
    --N_INST_;
}

#include <string>
void PointLight::update_uniforms(const Shader& shader) const
{
    //shader.send_uniform("lt.v3_lightPosition"_h, position_);
    shader.send_uniform("lt.v3_lightColor"_h, color_*brightness_);
    shader.send_uniform("lt.f_light_radius"_h, radius_);
    shader.send_uniform("lt.f_ambientStrength"_h, ambient_strength_);
}

math::mat4 PointLight::get_model_matrix(bool scaled) const
{
    math::mat4 T;
    T.init_translation(position_);
    if(scaled)
    {
        math::mat4 S;
        S.init_scale(radius_);
        return T*S;
    }
    return T;
}

size_t PointLight::get_geometry() const
{
    return Light::geom::SPHERE;
}

bool PointLight::is_in_frustum(const Camera& camera) const
{
    return camera.frustum_collides_sphere(position_, radius_);
}

bool PointLight::surrounds_camera(const Camera& camera) const
{
    return norm(camera.get_position()-position_)<radius_;
}

}
