#include <map>

#include "gfx_api.h"
#include "platform/opengl/ogl_renderer_api.h"
#include "logger.h"

namespace wcore
{

GfxAPI Gfx::api_ = GfxAPI::OpenGL;
RendererAPI* Gfx::renderer_ = new OGLRendererAPI();

RendererAPI::~RendererAPI()
{

}

void Gfx::set_api(GfxAPI api)
{
    api_ = api;

    delete renderer_;
    if(api_ == GfxAPI::OpenGL)
        renderer_ = new OGLRendererAPI();
}

} // namespace wcore
