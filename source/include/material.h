#ifndef MATERIAL_H
#define MATERIAL_H

#include <string>
#include <vector>
#include <map>

#include "math3d.h"
#include "utils.h"

struct MaterialDescriptor;
class Texture;
class Material
{
private:
    // Multi-unit image texture
    Texture* texture_;
    std::map<hashstr_t, bool> has_tex_map_;

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
    Material(const MaterialDescriptor& descriptor);
    Material(const math::vec3& tint,
             float roughness = 0.5f,
             float metallic = 0.0f,
             bool blend = false);
    ~Material();

    inline bool has_texture(hashstr_t sampler) const { return has_tex_map_.at(sampler); }

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

    inline bool has_normal_map() const             { return use_normal_map_; }
    inline void set_normal_map(bool value)         { use_normal_map_ = value; }

    inline bool has_parallax_map() const           { return use_parallax_map_; }
    inline void set_parallax_map(bool value)       { use_parallax_map_ = use_normal_map_ && value; }

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

#endif // MATERIAL_H
