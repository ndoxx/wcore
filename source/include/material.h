#ifndef MATERIAL_H
#define MATERIAL_H

#include <string>
#include <vector>
#include <memory>

#include "math3d.h"
#include "wtypes.h"

namespace wcore
{

enum class TextureUnit: uint16_t;
struct MaterialDescriptor;
class Texture;
class Material
{
private:
    // Multi-unit image texture
#ifdef __TEXTURE_OLD__
    Texture* texture_;
#else
    std::shared_ptr<Texture> texture_;
#endif

    // Alternative uniforms
    math::vec3 albedo_;
    float      metallic_;
    float      roughness_;

    // Shading options
    float parallax_height_scale_;
    float alpha_;

    bool textured_;
    bool use_normal_map_;
    bool use_parallax_map_;
    bool use_overlay_;
    bool blend_;

public:
    Material() = delete;
#ifdef __TEXTURE_OLD__
    Material(const MaterialDescriptor& descriptor);
#else
    Material(const MaterialDescriptor& descriptor, std::shared_ptr<Texture> texture);
#endif
    Material(const math::vec3& tint,
             float roughness = 0.5f,
             float metallic = 0.0f,
             bool blend = false);
    ~Material();

    bool has_texture(TextureUnit unit) const;

    inline bool is_textured() const                { return textured_; }
    inline const Texture& get_texture() const      { return *texture_; }
    void bind_texture() const;

    inline float get_roughness() const             { return roughness_; }
    inline void  set_roughness(float value)        { roughness_ = value; }

    inline float get_metallic() const              { return metallic_; }
    inline void  set_metallic(float value)         { metallic_ = value; }

    inline const math::vec3 get_albedo() const       { return albedo_; }
    inline void set_albedo(const math::vec3& value)  { albedo_ = value; }

    inline bool has_blend() const                  { return blend_; }
    inline void set_blend(bool value)              { blend_ = value; }
    inline float get_alpha() const                 { return alpha_; }
    inline void set_alpha(float value)             { alpha_ = value; }

    inline float get_parallax_height_scale() const { return parallax_height_scale_; }
    inline void set_parallax_height_scale(float value);

    inline void set_overlay(bool value) { use_overlay_ = value; }
    inline bool has_overlay() const { return use_overlay_; }
};

inline void Material::set_parallax_height_scale(float value)
{
    if(use_parallax_map_)
        parallax_height_scale_ = value;
}

}

#endif // MATERIAL_H
