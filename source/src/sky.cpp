#include "sky.h"
#include "cubemap.h"
#include "mesh_factory.h"

namespace wcore
{

std::shared_ptr<MeshP> SkyBox::mesh_ = factory::make_skybox_3P();

SkyBox::SkyBox(Cubemap* cubemap):
cubemap_(cubemap)
{

}

SkyBox::~SkyBox()
{

}

}
