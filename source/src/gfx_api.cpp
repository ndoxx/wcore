#include <map>

#include "gfx_api.h"
#include "platform/opengl/ogl_renderer_api.h"
#include "logger.h"

namespace wcore
{

GfxAPI Gfx::api_ = GfxAPI::OpenGL;
RenderDevice* Gfx::device_ = new OGLRenderDevice();

RenderDevice::~RenderDevice()
{

}

void Gfx::set_api(GfxAPI api)
{
    api_ = api;

    delete device_;
    if(api_ == GfxAPI::OpenGL)
        device_ = new OGLRenderDevice();
}

} // namespace wcore
