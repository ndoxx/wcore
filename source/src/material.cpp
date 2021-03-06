#include <iostream>
#include <vector>
#include <thread>

#include "material.h"
#include "texture.h"
#include "logger.h"
#include "material_factory.h"

namespace wcore
{

Material::Material(const MaterialDescriptor& descriptor, std::shared_ptr<Texture> texture):
texture_(nullptr),
albedo_(descriptor.albedo),
metallic_(descriptor.metallic),
roughness_(descriptor.roughness),
parallax_height_scale_(descriptor.parallax_height_scale),
alpha_(descriptor.transparency),
textured_(descriptor.is_textured),
use_normal_map_(descriptor.texture_descriptor.has_unit(TextureUnit::NORMAL) && descriptor.enable_normal_mapping),
use_parallax_map_(descriptor.texture_descriptor.has_unit(TextureUnit::DEPTH) && descriptor.enable_parallax_mapping),
use_overlay_(false),
blend_(descriptor.has_transparency)
{
    if(textured_)
        texture_ = texture;
}

Material::Material(const MaterialDescriptor& descriptor):
texture_(nullptr),
albedo_(descriptor.albedo),
metallic_(descriptor.metallic),
roughness_(descriptor.roughness),
parallax_height_scale_(descriptor.parallax_height_scale),
alpha_(descriptor.transparency),
textured_(descriptor.is_textured),
use_normal_map_(descriptor.texture_descriptor.has_unit(TextureUnit::NORMAL) && descriptor.enable_normal_mapping),
use_parallax_map_(descriptor.texture_descriptor.has_unit(TextureUnit::DEPTH) && descriptor.enable_parallax_mapping),
use_overlay_(false),
blend_(descriptor.has_transparency)
{
    if(textured_)
        texture_ = std::make_shared<Texture>(descriptor.texture_descriptor);
}

Material::Material(const math::vec3& tint,
                   float roughness,
                   float metallic,
                   bool blend):
texture_(nullptr),
albedo_(tint),
metallic_(metallic),
roughness_(roughness),
parallax_height_scale_(0.0f),
alpha_(1.0f),
textured_(false),
use_normal_map_(false),
use_parallax_map_(false),
use_overlay_(false),
blend_(blend)
{

}

Material::~Material()
{

}

void Material::bind_texture() const
{
    texture_->bind_all();
}

bool Material::has_texture(TextureUnit unit) const
{
    if(texture_)
    {
        switch(unit)
        {
            case TextureUnit::DEPTH:
                return (texture_->has_unit(unit) && use_parallax_map_);
            case TextureUnit::NORMAL:
                return (texture_->has_unit(unit) && use_normal_map_);
            default:
                return texture_->has_unit(unit);
        }
    }
    else
        return false;
}

}
