#ifndef RENDER_INFO_H
#define RENDER_INFO_H

namespace wcore
{

struct ModelRenderInfo
{
public:
    ModelRenderInfo():
    is_terrain(false)
    {}

    bool is_terrain;
};

}

#endif // RENDER_INFO_H
