#include "surface_mesh_factory.h"
#include "surface_mesh.h"
#include "mesh_factory.h"
#include "rock_generator.h"
#include "tree_generator.h"
#include "config.h"
#include "obj_loader.h"
#include "cspline.h"
#include "logger.h"

namespace wcore
{

using namespace math;

SurfaceMeshFactory::SurfaceMeshFactory()
{
    models_path_ = CONFIG.get_root_directory();
    models_path_ = models_path_ / "res/models";
}

SurfaceMeshFactory::~SurfaceMeshFactory()
{
    for(auto&& [key,meshptr]: cache_)
        delete meshptr;
}

SurfaceMesh* SurfaceMeshFactory::make_procedural(hash_t name, std::mt19937& rng, rapidxml::xml_node<char>* generator_node)
{
    SurfaceMesh* pmesh = nullptr;
    // Procedural meshes that depend on extra data
    if(name == H_("icosphere"))
    {
        IcosphereProps props;
        if(generator_node)
            props.parse_xml(generator_node);
        else
            props.density = 1;
        pmesh = (SurfaceMesh*)factory::make_ico_sphere(props.density);
    }
    else if(name == H_("tree"))
    {
        // Procedural tree mesh
        if(!generator_node) return nullptr;

        TreeProps props;
        props.parse_xml(generator_node);

        pmesh = TreeGenerator::generate_tree(props);
    }
    else if(name == H_("rock"))
    {
        // Procedural rock mesh, look for RockGenerator node
        if(!generator_node) return nullptr;

        RockProps props;
        props.parse_xml(generator_node);

        std::uniform_int_distribution<uint32_t> mesh_seed(0,std::numeric_limits<uint32_t>::max());
        props.seed = mesh_seed(rng);

        pmesh = RockGenerator::generate_rock(props);
    }

    // Hard-coded procedural meshes
    else if(name == H_("cube"))
        pmesh = (SurfaceMesh*)factory::make_cube();
    else if(name == H_("icosahedron"))
        pmesh = (SurfaceMesh*)factory::make_icosahedron();
    else if(name == H_("crystal"))
    {
        std::uniform_int_distribution<uint32_t> mesh_seed(0,std::numeric_limits<uint32_t>::max());
        pmesh = (SurfaceMesh*)factory::make_crystal(mesh_seed(rng));
    }
    else if(name == H_("tentacle"))
    {
        CSplineCatmullV3 spline({0.0f, 0.33f, 0.66f, 1.0f},
                                {vec3(0,0,0),
                                 vec3(0.1,0.33,0.1),
                                 vec3(0.4,0.66,-0.1),
                                 vec3(-0.1,1.2,-0.5)});
        pmesh = (SurfaceMesh*)factory::make_tentacle(spline, 50, 25, 0.1, 0.3);
    }

    return pmesh;
}

SurfaceMesh* SurfaceMeshFactory::make_obj(const char* filename, bool process_uv, bool centered)
{
    SurfaceMesh* pmesh;

    // * First, check if we have it in the map
    hash_t hname = H_(filename);
    auto it = cache_.find(hname);
    if(it!=cache_.end())
    {
        pmesh = it->second;
    }
    else
    {
        // Acquire mesh from Wavefront .obj file
        pmesh = LOADOBJ(models_path_ / filename, process_uv);
        pmesh->set_centered(centered);
        cache_.insert(std::pair(hname, pmesh));
    }

    return pmesh;
}

} // namespace wcore
