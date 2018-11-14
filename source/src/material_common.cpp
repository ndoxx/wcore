#include <GL/glew.h>

#include "material_common.h"

TextureParameters::TextureParameters():
filter(GL_LINEAR_MIPMAP_LINEAR),
internal_format(GL_RGBA),
format(GL_RGBA),
clamp(false),
lazy_mipmap(false)
{

}
