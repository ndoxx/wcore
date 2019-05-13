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
render_batch_("sky"_h)
{
    // Upload geometry
    render_batch_.submit(*mesh_);
    render_batch_.upload();
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
    render_batch_.draw(mesh_->get_buffer_token());
}


}
