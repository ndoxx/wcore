#include <GL/glew.h>

#include "material_common.h"
#include "logger.h"

namespace wcore
{

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
