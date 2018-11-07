#include <sstream>

#include "model.h"
#include "logger.h"
#include "material.h"
#include "vertex_format.h"

using namespace math;

Model::Model(Mesh<Vertex3P3N3T2U>* pmesh, const char* asset_name):
pmesh_(pmesh),
pmaterial_(new Material(asset_name)),
trans_(),
obb_(*this),
aabb_(*this),
frustum_cull_(true),
is_dynamic_(false),
shadow_cull_face_(0)
{
    #ifdef __DEBUG_MODEL_VERBOSE__
        DLOGN("[MODEL] New model.");
        std::stringstream ss;
        ss << "Mesh: n_indices=" << pmesh->get_ni() << " n_vertices="
           << pmesh->get_nv();
        DLOGI(ss.str());
        ss.str("");
        ss << "Material: from asset: " << asset_name;
        DLOGI(ss.str());
    #endif
}

Model::Model(Mesh<Vertex3P3N3T2U>* pmesh, Material* material):
pmesh_(pmesh),
pmaterial_(material),
trans_(),
obb_(*this),
aabb_(*this),
frustum_cull_(true),
is_dynamic_(false),
shadow_cull_face_(0)
{
    #ifdef __DEBUG_MODEL_VERBOSE__
        DLOGN("[MODEL] New model.");
        std::stringstream ss;
        ss << "Mesh: n_indices=" << pmesh->get_ni() << " n_vertices="
           << pmesh->get_nv();
        DLOGI(ss.str());
    #endif
}

Model::~Model()
{
    delete pmesh_;
    delete pmaterial_;
}

LineModel::LineModel(Mesh<Vertex3P>* pmesh, Material* material):
pmesh_(pmesh),
pmaterial_(material),
trans_()
{
    #ifdef __DEBUG_MODEL_VERBOSE__
        DLOGN("[MODEL] New line model.");
        std::stringstream ss;
        ss << "Mesh: n_indices=" << pmesh->get_ni() << " n_vertices="
           << pmesh->get_nv();
        DLOGI(ss.str());
    #endif
}

LineModel::~LineModel()
{
    delete pmesh_;
    delete pmaterial_;
}
