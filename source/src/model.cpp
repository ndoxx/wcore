#include <sstream>

#include "model.h"
#include "logger.h"
#include "material.h"
#include "vertex_format.h"

namespace wcore
{

using namespace math;

Model::Model(std::shared_ptr<SurfaceMesh> pmesh, Material* material):
pmesh_(pmesh),
pmaterial_(material),
trans_(),
obb_(pmesh_->get_dimensions(), pmesh_->is_centered()),
aabb_(),
frustum_cull_(true),
is_dynamic_(false),
is_terrain_(false),
visible_(false),
shadow_cull_face_(0),
reference_(0),
has_reference_(false)
#ifndef __DISABLE_EDITOR__
,editor_(nullptr)
,selection_reset_(nullptr)
#endif
{
    #ifdef __DEBUG__
        DLOGN("[Model] New static model.", "model");
        std::stringstream ss;
        ss << "Mesh: n_indices=" << pmesh->get_ni() << " n_vertices="
           << pmesh->get_nv();
        DLOGI(ss.str(), "model");
    #endif
}

Model::~Model()
{
#ifndef __DISABLE_EDITOR__
    if(selection_reset_)
        (editor_->*selection_reset_)();
#endif

    delete pmaterial_;
}

void Model::set_material(Material* material)
{
    delete pmaterial_;
    pmaterial_ = material;
    DLOGN("[Model] Swapped material.", "model");
}

void Model::set_mesh(std::shared_ptr<SurfaceMesh> pmesh)
{
    pmesh_ = pmesh;
    DLOGN("[Model] Swapped mesh.", "model");
}

LineModel::LineModel(Mesh<Vertex3P>* pmesh, Material* material):
pmesh_(pmesh),
pmaterial_(material),
trans_()
{
    #ifdef __DEBUG__
        DLOGN("[LineModel] New static line model.", "model");
        std::stringstream ss;
        ss << "Mesh: n_indices=" << pmesh->get_ni() << " n_vertices="
           << pmesh->get_nv();
        DLOGI(ss.str(), "model");
    #endif
}

LineModel::~LineModel()
{
    delete pmesh_;
    delete pmaterial_;
}

}
