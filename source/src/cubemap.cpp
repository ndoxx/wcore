#include <GL/glew.h>
#include <cassert>

#include "cubemap.h"
#include "material_common.h"
#include "pixel_buffer.h"
#include "png_loader.h"
#include "file_system.h"
#include "logger.h"

namespace wcore
{

static PngLoader PNG_LOADER;

Cubemap::Cubemap(const CubemapDescriptor& descriptor)
{
    // Sanity check
    assert(descriptor.locations.size() == 6 && "CubemapDescriptor is not initialized.");

#ifdef __DEBUG__
    DLOGN("[Cubemap] New cubemap.", "texture", Severity::LOW);
    for(GLuint ii=0; ii<6; ++ii)
    {
        DLOGI("<p>" + descriptor.locations[ii] + "</p>", "texture", Severity::LOW);
    }
#endif

    // Generate cubemap texture and bind it
    glGenTextures(1, &texture_id_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id_);

    // Array of 6 pixel buffers to hold texture data for each face
    PixelBuffer** px_bufs = new PixelBuffer*[6];
    // Load texture data
    for(GLuint ii=0; ii<6; ++ii)
    {
        auto stream = FILESYSTEM.get_file_as_stream(descriptor.locations[ii].c_str(), "root.folders.texture"_h, "pack0"_h);
        px_bufs[ii] = PNG_LOADER.load_png(*stream);

#ifdef __DEBUG__
        DLOGN("[PixelBuffer] <z>[" + std::to_string(ii) + "]</z>", "texture", Severity::DET);
        if(dbg::LOG.get_channel_verbosity("texture"_h) == 3u)
            px_bufs[ii]->debug_display();
#endif

        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + ii,
            0,
            GL_RGB,
            px_bufs[ii]->get_width(),
            px_bufs[ii]->get_height(),
            0,
            GL_RGB,
            GL_UNSIGNED_BYTE,
            px_bufs[ii]->get_data_pointer()
        );
    }

    // Texture parameters
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    delete [] px_bufs;
}

Cubemap::~Cubemap()
{

}

void Cubemap::bind()
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id_);
}

} // namespace wcore
