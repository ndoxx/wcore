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

}
