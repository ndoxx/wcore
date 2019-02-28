#include <filesystem>

#include "model_factory.h"
#include "material_factory.h"
#include "surface_mesh_factory.h"
#include "terrain_factory.h"
#include "xml_utils.hpp"
#include "material.h"
#include "texture.h"
#include "surface_mesh.h"
#include "terrain_patch.h"
#include "model.h"
#include "sky.h"
#include "logger.h"
#include "file_system.h"
#include "error.h"

namespace fs = std::filesystem;

namespace wcore
{

ModelFactory::ModelFactory(const char* assetfile):
mesh_factory_(new SurfaceMeshFactory()),
material_factory_(new MaterialFactory()),
terrain_factory_(new TerrainFactory())
{
    parse_asset_file(assetfile);
    rapidxml::xml_node<>* materials_node = xml_parser_.get_root()->first_node("Materials");
    rapidxml::xml_node<>* cubemaps_node  = xml_parser_.get_root()->first_node("Cubemaps");
    rapidxml::xml_node<>* meshes_node    = xml_parser_.get_root()->first_node("Meshes");
    rapidxml::xml_node<>* models_node    = xml_parser_.get_root()->first_node("Models");
    material_factory_->retrieve_material_descriptions(materials_node);
    material_factory_->retrieve_cubemap_descriptions(cubemaps_node);
    mesh_factory_->retrieve_asset_descriptions(meshes_node);
    retrieve_asset_descriptions(models_node);
}

ModelFactory::~ModelFactory()
{
    delete terrain_factory_;
    delete material_factory_;
    delete mesh_factory_;
}

std::shared_ptr<SurfaceMesh> ModelFactory::preload_mesh_instance(hash_t name)
{
    return mesh_factory_->make_instance(name);
}

std::shared_ptr<SurfaceMesh> ModelFactory::preload_mesh_model_instance(hash_t name)
{
    auto it = instance_descriptors_.find(name);
    if(it!=instance_descriptors_.end())
    {
        const ModelInstanceDescriptor& desc = it->second;
        return mesh_factory_->make_instance(desc.mesh_name);
    }
    else
    {
        DLOGW("[ModelFactory] No model instance named: ", "model", Severity::WARN);
        DLOGI(std::to_string(name) + " -> " + HRESOLVE(name), "model", Severity::WARN);
        DLOGI("Skipping.", "model", Severity::WARN);
    }
    return nullptr;
}

void ModelFactory::parse_asset_file(const char* xmlfile)
{
    auto pstream = FILESYSTEM.get_file_as_stream(xmlfile, "root.folders.level"_h, "pack0"_h);
    if(pstream == nullptr)
    {
        DLOGE("[ModelFactory] Unable to open file:", "model", Severity::CRIT);
        DLOGI(xmlfile, "model", Severity::CRIT);
        fatal();
    }
    xml_parser_.load_file_xml(*pstream);
}

void ModelFactory::retrieve_asset_descriptions(rapidxml::xml_node<>* models_node)
{
    for (rapidxml::xml_node<>* mdl_node=models_node->first_node("Model");
         mdl_node;
         mdl_node=mdl_node->next_sibling("Model"))
    {
        std::string model_name, mesh_name, material_name;
        if(xml::parse_attribute(mdl_node, "name", model_name))
        {
#ifdef __DEBUG__
            if(instance_descriptors_.find(H_(model_name.c_str())) != instance_descriptors_.end())
            {
                DLOGW("[ModelFactory] Model redefinition or collision: ", "model", Severity::WARN);
                DLOGI(model_name, "model", Severity::WARN);
            }
#endif

            ModelInstanceDescriptor descriptor;
            xml::parse_attribute(mdl_node->first_node("Mesh"),     "name", mesh_name);
            xml::parse_attribute(mdl_node->first_node("Material"), "name", material_name);
            descriptor.mesh_name     = H_(mesh_name.c_str());
            descriptor.material_name = H_(material_name.c_str());

            instance_descriptors_[H_(model_name.c_str())] = descriptor;
#ifdef __DEBUG__
            HRESOLVE.add_intern_string(model_name);
#endif
        }
    }
}

std::shared_ptr<Model> ModelFactory::make_model_instance(hash_t name)
{
    auto it = instance_descriptors_.find(name);
    if(it!=instance_descriptors_.end())
    {
        const ModelInstanceDescriptor& desc = it->second;

        std::shared_ptr<SurfaceMesh> pmesh = mesh_factory_->make_instance(desc.mesh_name);
        if(!pmesh)
        {
            DLOGE("[ModelFactory] Incomplete mesh declaration.", "parsing", Severity::CRIT);
            return nullptr;
        }

        Material* pmat = material_factory_->make_material(desc.material_name);
        if(!pmat)
        {
            DLOGE("[ModelFactory] Incomplete material declaration.", "parsing", Severity::CRIT);
            delete pmat;
            return nullptr;
        }

        return std::make_shared<Model>(pmesh, pmat);
    }
    else
        return nullptr;
}

std::shared_ptr<Model> ModelFactory::make_model(rapidxml::xml_node<>* mesh_node,
                                                rapidxml::xml_node<>* mat_node,
                                                bool& mesh_is_instance,
                                                OptRngT opt_rng)
{
    std::shared_ptr<SurfaceMesh> pmesh = mesh_factory_->make_surface_mesh(mesh_node, mesh_is_instance, opt_rng);
    if(!pmesh)
    {
        DLOGW("[ModelFactory] Incomplete mesh declaration.", "parsing", Severity::WARN);
        return nullptr;
    }

    Material* pmat = material_factory_->make_material(mat_node, opt_rng);
    if(!pmat)
    {
        DLOGW("[ModelFactory] Incomplete material declaration.", "parsing", Severity::WARN);
        delete pmat;
        return nullptr;
    }

    return std::make_shared<Model>(pmesh, pmat);
}

std::shared_ptr<TerrainChunk> ModelFactory::make_terrain_patch(const TerrainPatchDescriptor& desc,
                                                               OptRngT opt_rng)
{
    // --- Material
    Material* pmat = material_factory_->make_material(desc.material_node);
    if(!pmat)
    {
        DLOGW("[ModelFactory] Incomplete material declaration.", "parsing", Severity::WARN);
        delete pmat;
        return nullptr;
    }

    HeightMap* heightmap = terrain_factory_->make_heightmap(desc);

    auto ret = std::make_shared<TerrainChunk>(
        heightmap,
        pmat,
        desc.lattice_scale,
        desc.texture_scale
    );

    // Try to load splatmap
    std::string splatmap_name("splat_");
    splatmap_name += std::to_string(desc.chunk_x) + "_" + std::to_string(desc.chunk_z) + ".png";

    // Check that splatmap file exists before proceeding
    if(FILESYSTEM.file_exists(splatmap_name.c_str(), "root.folders.texture"_h, "pack0"_h))
    {
        auto pstream = FILESYSTEM.get_file_as_stream(splatmap_name.c_str(), "root.folders.texture"_h, "pack0"_h);
        if(pstream)
        {
            Texture* splatmap = material_factory_->make_texture(*pstream);
            // Try to load alt material for splat mapping
            if(desc.alt_material_node)
            {
                Material* pmat2 = material_factory_->make_material(desc.alt_material_node);
                if(pmat2)
                {
                    DLOGN("[ModelFactory] Using splat map:", "parsing", Severity::LOW);
                    DLOGI("<p>" + splatmap_name + "</p>", "parsing", Severity::LOW);

                    // Add them to the terrain chunk
                    ret->add_alternative_material(pmat2);
                    ret->add_splat_mat(splatmap);
                }
                else
                {
                    DLOGW("[ModelFactory] Incomplete alt-material declaration.", "parsing", Severity::WARN);
                    delete splatmap;
                }
            }
        }
        else
        {
            DLOGE("[ModelFactory] Stream error while loading splat map.", "io", Severity::CRIT);
            DLOGI("Skipping", "io", Severity::CRIT);
        }
    }

    return ret;
}

std::shared_ptr<SkyBox> ModelFactory::make_skybox(hash_t cubemap_name)
{
    Cubemap* cmap = material_factory_->make_cubemap(cubemap_name);
    return std::make_shared<SkyBox>(cmap);
}


} // namespace wcore

