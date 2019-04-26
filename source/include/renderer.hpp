#ifndef RENDERER_HPP
#define RENDERER_HPP

namespace wcore
{

class Scene;
class Renderer
{
public:
    virtual void render(Scene* pscene) = 0;
};

}

#endif // RENDERER_HPP
