#include <iostream>
#include <vector>
#include <stdexcept>

#include "material.h"
#include "texture.h"
#include "logger.h"
#include "material_factory.h"

Material::Material(const MaterialDescriptor& descriptor):
texture_(new Texture(descriptor.texture_descriptor)),
albedo_(descriptor.albedo),
metallic_(descriptor.metallic),
roughness_(descriptor.roughness),
parallax_height_scale_(descriptor.parallax_height_scale),
alpha_(descriptor.transparency),
textured_(true),
use_normal_map_(descriptor.texture_descriptor.has_unit.at(HS_("Normal"))),
use_parallax_map_(descriptor.texture_descriptor.has_unit.at(HS_("Depth"))),
use_overlay_(false),
blend_(false)
{

}

/*
Material::Material(const char* assetName):
texture_(new Texture(assetName)),
albedo_(0.0,0.0,0.0),
metallic_(0.0f),
roughness_(0.5f),
parallax_height_scale_(0.0f),
alpha_(1.0f),
textured_(true),
use_normal_map_(false),
use_parallax_map_(false),
use_overlay_(false),
blend_(false)
{

}*/

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
    if(texture_)
        delete texture_;
}

void Material::bind_texture() const
{
    texture_->bind_all();
}
