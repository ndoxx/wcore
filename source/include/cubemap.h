#ifndef CUBEMAP_H
#define CUBEMAP_H

#include <cstdint>

namespace wcore
{

struct CubemapDescriptor;
class Cubemap
{
public:
    Cubemap(const CubemapDescriptor& descriptor);
    ~Cubemap();

    void bind();

private:
    uint32_t texture_id_;
};


} // namespace wcore

#endif // CUBEMAP_H
