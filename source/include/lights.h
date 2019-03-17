#ifndef LIGHTS_H
#define LIGHTS_H

#include "math3d.h"

namespace wcore
{

class Shader;
class Camera;
class Light
{
protected:
    math::vec3 position_;
    math::vec3 color_;
    float ambient_strength_;
    float brightness_;

    hash_t reference_;
    bool has_reference_;

public:
    Light(const math::vec3& position,
          const math::vec3& color,
          float brightness = 1.0f);

    virtual ~Light();

    enum geom : uint8_t { QUAD, SPHERE, CONE };

    virtual void update_uniforms(const Shader& shader) const = 0;
    virtual math::mat4 get_model_matrix(bool scaled=true) const = 0;
    virtual size_t get_geometry() const = 0;
    virtual bool is_in_frustum(const Camera& camera) const = 0;
    virtual bool surrounds_camera(const Camera& camera) const = 0;

    inline void set_position(const math::vec3 position) { position_ = position ; }
    inline void set_color(const math::vec3 color)       { color_  = color ; }

    inline const math::vec3& get_position() const       { return position_; }
    inline math::vec3& get_position_nc()                { return position_; }
    inline const math::vec3& get_color() const          { return color_; }
    inline math::vec3& get_color_nc()                   { return color_; }

    inline float const* get_color_p() const             { return (float const*)&color_; }
    inline float const* get_position_p() const          { return (float const*)&position_; }

    inline void set_brightness(float brightness)        { brightness_ = brightness; }
    inline float get_brightness() const                 { return brightness_; }
    inline float& get_brightness_nc()                   { return brightness_; }

    inline void set_ambient_strength(float value)       { ambient_strength_ = value; }
    inline float get_ambient_strength() const           { return ambient_strength_; }
    inline float& get_ambient_strength_nc()             { return ambient_strength_; }

    virtual void set_radius(float value) {}

    // Reference
    inline void set_reference(hash_t hname) { reference_ = hname; has_reference_ = true; }
    inline void forget_reference()          { reference_ = 0; has_reference_ = false; }
    inline hash_t get_reference() const     { return reference_; }
    inline bool has_reference() const       { return has_reference_; }

protected:
    inline void send_uniform_float(unsigned int program_id, const char* name, float value) const;
    inline void send_uniform_vec3(unsigned int program_id, const char* name, const math::vec3& value) const;
};

class DirectionalLight : public Light
{
public:
    DirectionalLight(const math::vec3& position,
                     const math::vec3& color,
                     float brightness = 1.0f);

    ~DirectionalLight();

    virtual void update_uniforms(const Shader& shader) const override;
    virtual math::mat4 get_model_matrix(bool scaled=true) const override;
    virtual size_t get_geometry() const override;
    virtual bool is_in_frustum(const Camera& camera) const override;
    virtual bool surrounds_camera(const Camera& camera) const override;
};

class PointLight : public Light
{
private:
    float radius_; // light volume
    uint32_t index_;
    static uint32_t N_INST_;

public:
    PointLight(const math::vec3& position,
               const math::vec3& color,
               float radius     = 7.0f,
               float brightness = 1.0f);

    ~PointLight();

    inline float get_radius() const { return radius_; }
    virtual void set_radius(float value) override { radius_ = value; }

    virtual void update_uniforms(const Shader& shader) const override;
    virtual math::mat4 get_model_matrix(bool scaled=true) const override;
    virtual size_t get_geometry() const override;
    virtual bool is_in_frustum(const Camera& camera) const override;
    virtual bool surrounds_camera(const Camera& camera) const override;
};

}

#endif // LIGHTS_H
