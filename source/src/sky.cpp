#include <iostream>
#include "sky.h"
#include "cubemap.h"
#include "mesh_factory.h"
#include "vertex_format.h"

namespace wcore
{

SkyBox::SkyBox(Cubemap* cubemap):
cubemap_(cubemap),
mesh_(factory::make_skybox_3P()),
buffer_unit_()
{
    // Upload geometry
    buffer_unit_.submit(*mesh_);
    buffer_unit_.upload();
}

SkyBox::~SkyBox()
{
    delete cubemap_;
}

void SkyBox::bind() const
{
    cubemap_->bind();
}

void SkyBox::draw() const
{
    // Bind vertex array and draw geometry
    buffer_unit_.draw(mesh_->get_buffer_token());
}


}
