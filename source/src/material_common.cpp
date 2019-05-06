#include <GL/glew.h>

#include "material_common.h"
#include "logger.h"

namespace wcore
{

TextureParameters::TextureParameters():
filter(GL_LINEAR_MIPMAP_LINEAR),
internal_format(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT),
//internal_format(GL_RGBA),
format(GL_RGBA),
clamp(false),
lazy_mipmap(false)
{

}

TextureDescriptor::TextureDescriptor():
units(0),
sampler_group(1),
n_units(0),
parameters(),
resource_id(""_h)
{

}

MaterialDescriptor::MaterialDescriptor():
texture_descriptor(),
albedo(math::vec3(1.0f,0.0f,0.0f)),
transparency(1.0f),
metallic(0.0f),
roughness(0.1f),
has_transparency(false),
is_textured(false),
parallax_height_scale(0.1f),
enable_normal_mapping(true),
enable_parallax_mapping(true),
is_wat(false)
{

}

}
