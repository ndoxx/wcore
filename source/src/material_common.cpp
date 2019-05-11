#include <GL/glew.h>

#include "material_common.h"
#include "logger.h"

namespace wcore
{

TextureParameters::TextureParameters():
filter(TextureFilter(TextureFilter::MAG_LINEAR | TextureFilter::MIN_LINEAR_MIPMAP_LINEAR)),
internal_format(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT),
//internal_format(GL_RGBA),
format(GL_RGBA),
wrap(TextureWrap::REPEAT),
lazy_mipmap(false)
{

}

TextureDescriptor::TextureDescriptor():
unit_flags(0),
sampler_group(1),
parameters(),
resource_id(""_h),
is_wat(false),
owns_data(false),
width(0),
height(0),
block0_data(nullptr),
block1_data(nullptr),
block2_data(nullptr)
{

}

TextureDescriptor::~TextureDescriptor()
{
    if(owns_data)
    {
        delete[] block0_data;
        delete[] block1_data;
        delete[] block2_data;
    }
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
enable_parallax_mapping(true)
{

}

}
