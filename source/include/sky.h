#ifndef SKY_H
#define SKY_H

#include <memory>
#include "mesh.hpp"

namespace wcore
{

class Cubemap;
class SkyBox
{
public:
    SkyBox(Cubemap* cubemap);
    ~SkyBox();

private:
    Cubemap* cubemap_;

    static std::shared_ptr<MeshP> mesh_;
};

}

#endif // SKY_H
