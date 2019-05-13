#ifndef SKY_H
#define SKY_H

#include <memory>
#include "mesh.hpp"
#include "render_batch.hpp"

namespace wcore
{

class Cubemap;
class SkyBox
{
public:
    SkyBox(Cubemap* cubemap);
    ~SkyBox();

    // Bind cubemap
    void bind() const;
    // Draw geometry
    void draw() const;

    inline const Cubemap& get_cubemap() { return *cubemap_; }

private:
    Cubemap* cubemap_;
    std::shared_ptr<MeshP> mesh_;

    RenderBatch<Vertex3P> render_batch_;
};

}

#endif // SKY_H
