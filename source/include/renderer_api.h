#ifndef RENDERER_API_H
#define RENDERER_API_H

namespace wcore
{

enum class GfxAPI
{
    None = 0,
    OpenGL = 1
};

class RendererAPI
{
public:
    inline static GfxAPI get_api() { return api_; }

private:
    static GfxAPI api_;
};

} // namespace wcore

#endif
