#ifndef SKY_H
#define SKY_H

#include <memory>
#include "mesh.hpp"
#include "buffer_unit.hpp"
#include "vertex_array.hpp"

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

    BufferUnit<Vertex3P> buffer_unit_;
    VertexArray<Vertex3P> vertex_array_;
};

}

#endif // SKY_H
