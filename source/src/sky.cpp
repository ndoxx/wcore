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
buffer_unit_(),
vertex_array_(buffer_unit_)
{
    buffer_unit_.submit(*mesh_);
    buffer_unit_.upload();
}

SkyBox::~SkyBox()
{

}

void SkyBox::bind() const
{
    cubemap_->bind();
}

void SkyBox::draw() const
{
    const BufferToken& token = mesh_->get_buffer_token();

    vertex_array_.bind();
    buffer_unit_.draw(token);
}


}
