#include "surface_mesh_factory.h"
#include "surface_mesh.h"
#include "mesh_factory.h"
#include "rock_generator.h"
#include "tree_generator.h"
#include "config.h"
#include "io_utils.h"
#include "obj_loader.h"
#include "cspline.h"
#include "logger.h"
#include "xml_utils.hpp"

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
    /*for(auto&& [key,meshptr]: cache_)
        delete meshptr;
    for(auto&& [key,meshptr]: proc_cache_)
        delete meshptr;*/
}

void SurfaceMeshFactory::retrieve_asset_descriptions(rapidxml::xml_node<>* meshes_node)
{
    for (rapidxml::xml_node<>* mesh_node=meshes_node->first_node("Mesh");
         mesh_node;
         mesh_node=mesh_node->next_sibling("Mesh"))
    {
        MeshInstanceDescriptor desc;
        desc.generator_node = mesh_node->first_node("Generator");

        std::string mesh_name, mesh_type;
        if(!xml::parse_attribute(mesh_node, "type", mesh_type)) continue;
        if(!xml::parse_attribute(mesh_node, "name", mesh_name)) continue;

        desc.type = H_(mesh_type.c_str());

        // Additional info for obj files
        if(desc.type == H_("obj"))
        {
            std::string file_name;
            xml::parse_node(mesh_node, "Location", file_name);
            desc.file_path = models_path_ / file_name;
            if(!fs::exists(desc.file_path)) continue;
            xml::parse_node(mesh_node, "ProcessUV", desc.process_uv);
            xml::parse_node(mesh_node, "Centered", desc.centered);
        }

        instance_descriptors_.insert(std::pair(H_(mesh_name.c_str()), desc));
        #ifdef __DEBUG__
            HRESOLVE.add_intern_string(mesh_name);
        #endif
    }
}

std::shared_ptr<SurfaceMesh> SurfaceMeshFactory::make_surface_mesh(rapidxml::xml_node<>* mesh_node,
                                                                   OptRngT opt_rng)
{
    if(!mesh_node)
        return nullptr;

    std::shared_ptr<SurfaceMesh> pmesh = nullptr;

    // Has a "name" attribute -> mesh instance
    std::string name;
    if(xml::parse_attribute(mesh_node, "name", name))
        return make_instance(H_(name.c_str()));

    std::string mesh;
    if(!xml::parse_attribute(mesh_node, "type", mesh))
        return nullptr;

    if(!mesh.compare("obj"))
    {
        // Acquire mesh from Wavefront .obj file
        std::string location;
        if(!xml::parse_node(mesh_node, "Location", location))
        {
            DLOGW("[SceneLoader] Ignoring incomplete .obj mesh declaration.", "parsing", Severity::WARN);
            DLOGI("Missing <n>Location</n> node.", "parsing", Severity::WARN);
            return nullptr;
        }
        bool process_uv = false;
        bool centered = false;
        xml::parse_node(mesh_node, "ProcessUV", process_uv);
        xml::parse_node(mesh_node, "Centered", centered);
        pmesh = make_obj(location.c_str(), process_uv, centered);
    }
    else
    {
        rapidxml::xml_node<>* gen_node = mesh_node->first_node("Generator");
        pmesh = make_procedural(H_(mesh.c_str()), gen_node, opt_rng);
    }

    if(pmesh == nullptr)
    {
        DLOGW("Couldn't create mesh: name= ", "parsing", Severity::WARN);
        DLOGI(mesh, "parsing", Severity::WARN);
    }

    return pmesh;
}

std::shared_ptr<SurfaceMesh> SurfaceMeshFactory::make_instance(hash_t name)
{
    DLOGN("Instance mesh from name: " + std::to_string(name) + " -> " + HRESOLVE(name), "model", Severity::LOW);
    // First, try to find in cache
    auto it = cache_.find(name);
    if(it != cache_.end())
    {
        DLOGI("Using cache.", "model", Severity::LOW);
        return it->second;
    }
    else
    {
        // Run instance descriptor table
        auto it_d = instance_descriptors_.find(name);
        if(it_d != instance_descriptors_.end())
        {
            std::shared_ptr<SurfaceMesh> pmesh = nullptr;
            const MeshInstanceDescriptor& desc = it_d->second;
            if(desc.type==H_("obj"))
            {
                DLOGI("From obj file.", "model", Severity::LOW);
                pmesh = make_obj(desc.file_path.string().c_str(), desc.process_uv, desc.centered);
            }
            else
            {
                DLOGI("Procedural.", "model", Severity::LOW);
                std::mt19937 rng(0);
                pmesh = make_procedural(desc.type, desc.generator_node, &rng, false);
            }
            if(pmesh != nullptr)
            {
                cache_.insert(std::pair(name, pmesh));
                return pmesh;
            }
        }
    }
    return nullptr;
}

std::shared_ptr<SurfaceMesh> SurfaceMeshFactory::procedural_cache_lookup(hash_t mesh_type,
                                                         hash_t props,
                                                         std::function<std::shared_ptr<SurfaceMesh>(void)>new_mesh,
                                                         bool owns)
{
    //hash_t hash_comb = HCOMBINE_(mesh_type, props);
    /*auto it = proc_cache_.find(hash_comb);
    if(it!=proc_cache_.end())
    {
        return it->second;
    }
    else
    {*/
        std::shared_ptr<SurfaceMesh> pmesh = new_mesh();

        if(owns)
        {
            pmesh->set_cached(true);
            //proc_cache_.insert(std::pair(hash_comb, pmesh));
        }
        return pmesh;
    //}
}

std::shared_ptr<SurfaceMesh> SurfaceMeshFactory::make_procedural(hash_t mesh_type,
                                                 rapidxml::xml_node<char>* generator_node,
                                                 OptRngT opt_rng,
                                                 bool owns)
{
    // Procedural meshes that depend on extra data -> not cached ftm
    if(mesh_type == H_("icosphere"))
    {
        IcosphereProps props;
        if(generator_node)
            props.parse_xml(generator_node);
        else
            props.density = 1;

        //return procedural_cache_lookup(mesh_type, std::hash<IcosphereProps>{}(props), [&]()
        //{
            return (std::shared_ptr<SurfaceMesh>)factory::make_ico_sphere(props.density);
        //}, owns);
    }
    else if(mesh_type == H_("box"))
    {
        BoxProps props;
        if(generator_node)
            props.parse_xml(generator_node);
        else
        {
            props.extent = {-1.f,1.f,-1.f,1.f,-1.f,1.f};
            props.texture_scale = 1.0f;
        }

        //return procedural_cache_lookup(mesh_type, std::hash<BoxProps>{}(props), [&]()
        //{
            return (std::shared_ptr<SurfaceMesh>)factory::make_box(props.extent, props.texture_scale);
        //}, owns);
    }
    else if(mesh_type == H_("crystal") && opt_rng)
    {
        //std::uniform_int_distribution<uint32_t> mesh_seed(0,std::numeric_limits<uint32_t>::max());
        std::uniform_int_distribution<uint32_t> mesh_seed(0,10); // only N different meshes possible
        uint32_t seed = mesh_seed(*opt_rng);

        //return procedural_cache_lookup(mesh_type, seed, [&]()
        //{
            return (std::shared_ptr<SurfaceMesh>)factory::make_crystal(seed);
        //}, owns);
    }
    else if(mesh_type == H_("tree"))
    {
        // Procedural tree mesh
        if(!generator_node) return nullptr;

        TreeProps props;
        props.parse_xml(generator_node);

        //return procedural_cache_lookup(mesh_type, std::hash<TreeProps>{}(props), [&]()
        //{
            return TreeGenerator::generate_tree(props);
        //}, owns);
    }
    else if(mesh_type == H_("rock") && opt_rng)
    {
        // Procedural rock mesh, look for RockGenerator node
        if(!generator_node) return nullptr;

        RockProps props;
        props.parse_xml(generator_node);

        std::uniform_int_distribution<uint32_t> mesh_seed(0,std::numeric_limits<uint32_t>::max());
        props.seed = mesh_seed(*opt_rng);

        //return procedural_cache_lookup(mesh_type, std::hash<RockProps>{}(props), [&]()
        //{
            return RockGenerator::generate_rock(props);
        //}, owns);
    }

    // Hard-coded procedural meshes
    // Find in cache first
    std::shared_ptr<SurfaceMesh> pmesh = nullptr;
    /*auto it = cache_.find(mesh_type);
    if(it!=cache_.end())
        pmesh = it->second;
    else
    {*/
        if(mesh_type == H_("cube"))
            pmesh = (std::shared_ptr<SurfaceMesh>)factory::make_cube();
        else if(mesh_type == H_("icosahedron"))
            pmesh = (std::shared_ptr<SurfaceMesh>)factory::make_icosahedron();
        else if(mesh_type == H_("tentacle")) // TMP
        {
            CSplineCatmullV3 spline({0.0f, 0.33f, 0.66f, 1.0f},
                                    {vec3(0,0,0),
                                     vec3(0.1,0.33,0.1),
                                     vec3(0.4,0.66,-0.1),
                                     vec3(-0.1,1.2,-0.5)});
            pmesh = (std::shared_ptr<SurfaceMesh>)factory::make_tentacle(spline, 50, 25, 0.1, 0.3);
        }

        if(pmesh && owns)
        {
            pmesh->set_cached(true);
            //cache_.insert(std::pair(mesh_type, pmesh));
        }
    //}

    return pmesh;
}

std::shared_ptr<SurfaceMesh> SurfaceMeshFactory::make_obj(const char* filename, bool process_uv, bool centered)
{
    std::shared_ptr<SurfaceMesh> pmesh;

    // First, check if we have it in the map
    hash_t hname = H_(filename);
    /*auto it = cache_.find(hname);
    if(it!=cache_.end())
    {
        pmesh = it->second;
    }
    // Acquire mesh from Wavefront .obj file
    else
    {*/
        pmesh = LOADOBJ(models_path_ / filename, process_uv);
        pmesh->set_centered(centered);
        pmesh->set_cached(true);
        //cache_.insert(std::pair(hname, pmesh));
    //}

    return pmesh;
}

} // namespace wcore
