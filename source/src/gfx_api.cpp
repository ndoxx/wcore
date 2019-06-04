#include <map>

#include "gfx_api.h"
#include "platform/opengl/ogl_renderer_api.h"
#include "logger.h"

namespace wcore
{

GfxAPI Gfx::api_ = GfxAPI::OpenGL;
std::unique_ptr<RenderDevice> Gfx::device = std::make_unique<OGLRenderDevice>();

RenderDevice::~RenderDevice()
{

}

void Gfx::set_api(GfxAPI api)
{
    api_ = api;

    if(api_ == GfxAPI::OpenGL)
        device = std::make_unique<OGLRenderDevice>();
}

} // namespace wcore
