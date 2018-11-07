#include <iostream>
#include <vector>
#include <stdexcept>

#include "material.h"
#include "texture.h"
#include "logger.h"


Material::Material(const char* assetName):
texture_(new Texture(assetName)),
roughness_(0.5),
metallic_(0.0),
parallax_height_scale_(0.0f),
alpha_(1.0f),
tint_(0.0,0.0,0.0),
textured_(true),
use_normal_map_(false),
use_parallax_map_(false),
use_overlay_(false),
blend_(false)
{

}

Material::Material(const math::vec3& tint,
                   float roughness,
                   float metallic,
                   bool blend):
texture_(nullptr),
roughness_(roughness),
metallic_(metallic),
parallax_height_scale_(0.0f),
alpha_(1.0f),
tint_(tint),
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
