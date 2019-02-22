#include "surface_mesh_factory.h"
#include "file_system.h"
#include "surface_mesh.h"
#include "mesh_factory.h"
#include "rock_generator.h"
#include "tree_generator.h"
#include "config.h"
#include "obj_loader.h"
#include "cspline.h"
#include "logger.h"
#include "xml_utils.hpp"

namespace wcore
{

using namespace math;

SurfaceMeshFactory::SurfaceMeshFactory():
obj_loader_(new ObjLoader())
{
    models_path_ = CONFIG.get_root_directory();
    models_path_ = models_path_ / "res/models";
}

SurfaceMeshFactory::~SurfaceMeshFactory()
{
    delete obj_loader_;
}

bool SurfaceMeshDescriptor::parse(rapidxml::xml_node<char>* mesh_node)
{
    generator_node = mesh_node->first_node("Generator");

    std::string mesh_type;
    if(!xml::parse_attribute(mesh_node, "type", mesh_type)) return false;
    type = H_(mesh_type.c_str());

    // Normal smoothing func
    std::string smooth_func_str;
    smooth_func = Smooth::NONE;
    if(xml::parse_node(mesh_node, "SmoothNormals", smooth_func_str))
    {
        hash_t smooth_func_h = H_(smooth_func_str.c_str());
        switch(smooth_func_h)
        {
            case "none"_h:
                smooth_func = Smooth::NONE;
                break;
            case "max"_h:
                smooth_func = Smooth::MAX;
                break;
            case "heaviside"_h:
                smooth_func = Smooth::HEAVISIDE;
                break;
            case "linear"_h:
                smooth_func = Smooth::LINEAR;
                break;
            case "compress_linear"_h:
                smooth_func = Smooth::COMPRESS_LINEAR;
                break;
            case "compress_quadratic"_h:
                smooth_func = Smooth::COMPRESS_QUADRATIC;
                break;
            default:
                DLOGW("[SurfaceMeshFactory] Unknown smooth function: " + smooth_func_str, "model", Severity::WARN);
                break;
        }
    }

    // Additional info for obj files
    if(type == "obj"_h)
    {
        if(!xml::parse_node(mesh_node, "Location", file_name))
        {
            DLOGW("[SceneLoader] Ignoring incomplete .obj mesh declaration.", "parsing", Severity::WARN);
            DLOGI("Missing <n>Location</n> node.", "parsing", Severity::WARN);
            return false;
        }

        process_uv = false;
        process_normals = false;
        centered = false;
        xml::parse_node(mesh_node, "ProcessUV", process_uv);
        xml::parse_node(mesh_node, "ProcessNormals", process_normals);
        xml::parse_node(mesh_node, "Centered", centered);
    }
    return true;
}


void SurfaceMeshFactory::retrieve_asset_descriptions(rapidxml::xml_node<>* meshes_node)
{
    for (rapidxml::xml_node<>* mesh_node=meshes_node->first_node("Mesh");
         mesh_node;
         mesh_node=mesh_node->next_sibling("Mesh"))
    {
        std::string mesh_name;
        if(!xml::parse_attribute(mesh_node, "name", mesh_name)) continue;

        SurfaceMeshDescriptor desc;
        if(desc.parse(mesh_node))
        {
            instance_descriptors_.insert(std::pair(H_(mesh_name.c_str()), desc));
            #ifdef __DEBUG__
                HRESOLVE.add_intern_string(mesh_name);
            #endif
        }
    }
}

std::shared_ptr<SurfaceMesh> SurfaceMeshFactory::make_surface_mesh(rapidxml::xml_node<>* mesh_node,
                                                                   bool& mesh_is_instance,
                                                                   OptRngT opt_rng)
{
    if(!mesh_node)
        return nullptr;

    std::shared_ptr<SurfaceMesh> pmesh = nullptr;

    // Has a "name" attribute -> mesh instance
    std::string name;
    if(xml::parse_attribute(mesh_node, "name", name))
    {
        mesh_is_instance = true;
        return make_instance(H_(name.c_str()));
    }
    mesh_is_instance = false;

    std::string mesh;
    if(!xml::parse_attribute(mesh_node, "type", mesh))
        return nullptr;

    if(!mesh.compare("obj"))
    {
        SurfaceMeshDescriptor desc;
        if(desc.parse(mesh_node))
            pmesh = make_obj(desc.file_name.c_str(),
                             desc.process_uv,
                             desc.process_normals,
                             desc.centered,
                             desc.smooth_func);
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
    DLOGN("Instance mesh from name: " + std::to_string(name) + " -> <n>" + HRESOLVE(name) + "</n>", "model", Severity::LOW);
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
            const SurfaceMeshDescriptor& desc = it_d->second;
            if(desc.type=="obj"_h)
            {
                DLOGI("From obj file.", "model", Severity::LOW);
                pmesh = make_obj(desc.file_name.c_str(),
                                 desc.process_uv,
                                 desc.process_normals,
                                 desc.centered,
                                 desc.smooth_func);
            }
            else
            {
                DLOGI("Procedural.", "model", Severity::LOW);
                std::mt19937 rng(0);
                pmesh = make_procedural(desc.type, desc.generator_node, &rng);
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

std::shared_ptr<SurfaceMesh> SurfaceMeshFactory::make_procedural(hash_t mesh_type,
                                                 rapidxml::xml_node<char>* generator_node,
                                                 OptRngT opt_rng)
{
    // Procedural meshes that depend on extra data -> not cached ftm
    if(mesh_type == "icosphere"_h)
    {
        IcosphereProps props;
        if(generator_node)
            props.parse_xml(generator_node);
        else
            props.density = 1;

        return (std::shared_ptr<SurfaceMesh>)factory::make_ico_sphere(props.density);
    }
    else if(mesh_type == "box"_h)
    {
        BoxProps props;
        if(generator_node)
            props.parse_xml(generator_node);
        else
        {
            props.extent = {-1.f,1.f,-1.f,1.f,-1.f,1.f};
            props.texture_scale = 1.0f;
        }

        return (std::shared_ptr<SurfaceMesh>)factory::make_box(props.extent, props.texture_scale);
    }
    else if(mesh_type == "crystal"_h && opt_rng)
    {
        //std::uniform_int_distribution<uint32_t> mesh_seed(0,std::numeric_limits<uint32_t>::max());
        std::uniform_int_distribution<uint32_t> mesh_seed(0,10); // only N different meshes possible
        uint32_t seed = mesh_seed(*opt_rng);

        return (std::shared_ptr<SurfaceMesh>)factory::make_crystal(seed);
    }
    else if(mesh_type == "tree"_h)
    {
        // Procedural tree mesh
        if(!generator_node) return nullptr;

        TreeProps props;
        props.parse_xml(generator_node);

        return TreeGenerator::generate_tree(props);
    }
    else if(mesh_type == "rock"_h && opt_rng)
    {
        // Procedural rock mesh, look for RockGenerator node
        if(!generator_node) return nullptr;

        RockProps props;
        props.parse_xml(generator_node);

        std::uniform_int_distribution<uint32_t> mesh_seed(0,std::numeric_limits<uint32_t>::max());
        props.seed = mesh_seed(*opt_rng);

        return RockGenerator::generate_rock(props);
    }

    // Hard-coded procedural meshes
    std::shared_ptr<SurfaceMesh> pmesh = nullptr;

    if(mesh_type == "cube"_h)
        pmesh = static_cast<std::shared_ptr<SurfaceMesh>>(factory::make_cube());
    else if(mesh_type == "icosahedron"_h)
        pmesh = static_cast<std::shared_ptr<SurfaceMesh>>(factory::make_icosahedron());
    else if(mesh_type == "tentacle"_h) // TMP
    {
        CSplineCatmullV3 spline({0.0f, 0.33f, 0.66f, 1.0f},
                                {vec3(0,0,0),
                                 vec3(0.1,0.33,0.1),
                                 vec3(0.4,0.66,-0.1),
                                 vec3(-0.1,1.2,-0.5)});
        pmesh = static_cast<std::shared_ptr<SurfaceMesh>>(factory::make_tentacle(spline, 50, 25, 0.1, 0.3));
    }

    return pmesh;
}

std::shared_ptr<SurfaceMesh> SurfaceMeshFactory::make_obj(const char* filename,
                                                          bool process_uv,
                                                          bool process_normals,
                                                          bool centered,
                                                          int smooth_func)
{
    auto stream = FILESYSTEM.get_file_as_stream(filename, "root.folders.model"_h, "pack0"_h);
    std::shared_ptr<SurfaceMesh> pmesh = obj_loader_->load(*stream,
                                                           process_uv,
                                                           process_normals,
                                                           smooth_func);
    pmesh->set_centered(centered);

    return pmesh;
}

} // namespace wcore
