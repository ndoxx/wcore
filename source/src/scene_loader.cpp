#include <random>
#include <limits>
#include <algorithm>
#include <sstream>
#include <memory>
#include <vector>
#include <set>

#include "scene_loader.h"
#include "logger.h"
#include "scene.h"
#include "lights.h"
#include "colors.h"
#include "math3d.h"
#include "game_object_factory.h"
#include "height_map.h"
#include "camera.h"
#include "model.h"
#include "terrain_patch.h"
#include "texture.h"
#include "motion.hpp"
#include "bezier.h"
#include "daylight.h"
#include "sound_system.h"
#include "entity_system.h"
#include "input_handler.h"
#include "file_system.h"
#include "basic_components.h"
#include "pipeline.h"

namespace wcore
{

using namespace rapidxml;
using namespace math;

typedef std::shared_ptr<Model>        pModel;
typedef std::shared_ptr<TerrainChunk> pTerrain;
typedef std::shared_ptr<Camera>       pCamera;
typedef std::shared_ptr<Light>        pLight;
typedef std::shared_ptr<const Model>  pcModel;
typedef std::shared_ptr<const Camera> pcCamera;
typedef std::shared_ptr<const Light>  pcLight;

SceneLoader::SceneLoader():
xml_parser_(),
game_object_factory_(nullptr),
chunk_size_m_(32),
lattice_scale_(1.0f),
texture_scale_(1.0f),
pscene_(nullptr)
{

}

SceneLoader::~SceneLoader()
{

}

void SceneLoader::init_self()
{
    // Locate scene game system
    pscene_ = locate<Scene>("Scene"_h);
    game_object_factory_ = locate<GameObjectFactory>("GameObjectFactory"_h);
}

void SceneLoader::init_events(InputHandler& handler)
{
    subscribe("input.keyboard"_h, handler, &SceneLoader::onKeyboardEvent);
}

void SceneLoader::on_unload()
{
/*
    // Save camera position/orientation
    auto pcam = pscene_->get_camera();
    const vec3& position = pcam->get_position();
    float yaw = pcam->get_yaw();
    float pitch = pcam->get_pitch();
    vec2 orientation(yaw, pitch);

    auto* cam_node = xml_parser_.get_root()->first_node("Camera");
    // TODO: xml::set_node()
    auto* pos_node = cam_node->first_node("Position");
    if(pos_node)
    {
        char* value_str = xml_parser_.allocate_string(wcore::to_string(position).c_str());
        pos_node->value(value_str);
    }
    auto* ori_node = cam_node->first_node("Orientation");
    if(ori_node)
    {
        char* value_str = xml_parser_.allocate_string(wcore::to_string(orientation).c_str());
        ori_node->value(value_str);
    }

    xml_parser_.write();
*/
}

static inline std::string level_file(const char* level_name)
{
    return std::string("l_") + level_name + ".xml";
}

void SceneLoader::load_level(const char* level_name)
{
    DLOGS("[SceneLoader] Parsing xml scene description.", "scene", Severity::LOW);

    std::string levelfilename(level_file(level_name));
    auto pstream = FILESYSTEM.get_file_as_stream(levelfilename.c_str(), "root.folders.level"_h, "pack0"_h);
    if(pstream == nullptr)
    {
        DLOGF("Cannot access level file:", "scene");
        DLOGI("<p>" + levelfilename + "</p>", "scene");
        fatal();
    }
    xml_parser_.load_file_xml(*pstream);

    current_map_ = level_name;
    DLOGES("scene", Severity::LOW);
}

bool SceneLoader::onKeyboardEvent(const WData& data)
{
    const KbdData& kbd = static_cast<const KbdData&>(data);

    switch(kbd.key_binding)
    {
        case "k_reload_chunks"_h:
    		reload_chunks();
    		break;
        case "k_reload_map"_h:
    		reload_map();
    		break;
    }

    return true; // Do NOT consume event
}

pModel SceneLoader::load_model_instance(hash_t name, uint32_t chunk_index, hash_t href)
{
    // Create model from instance name
    pModel pmdl = game_object_factory_->make_model_instance(name);

    if(href)
        pmdl->set_reference(href);

    pmdl->update_bounding_boxes();

    // Add model to scene
    pscene_->add_model(pmdl, chunk_index);
    return pmdl;
}


pLight SceneLoader::load_point_light(uint32_t chunk_index, hash_t href)
{
    std::shared_ptr<Light> pointlight(new PointLight(vec3(0,0,0),
                                                     vec3(1,1,1),
                                                     1.f,
                                                     10.f));

    if(href)
        pointlight->set_reference(href);

    pointlight->set_ambient_strength(0.05);
    pscene_->add_light(pointlight, chunk_index);
    return pointlight;
}

void SceneLoader::load_global(DaylightSystem& daylight)
{
    // Save chunk nodes
    for (xml_node<>* ck_node=xml_parser_.get_root()->first_node("Chunk");
         ck_node;
         ck_node=ck_node->next_sibling("Chunk"))
    {
        i32vec2 chunk_coords;
        if(xml::parse_attribute(ck_node, "coords", chunk_coords))
        {
            // Compute hash of chunk coordinates to generate unique chunk index
            uint32_t chunk_index = std::hash<i32vec2>{}(chunk_coords);
            chunk_nodes_.insert(std::make_pair(chunk_index, ck_node));
        }
    }

    // Parse audio data
    parse_audio(xml_parser_.get_root()->first_node("Audio"));
    // Parse terrain patches
    parse_patches(xml_parser_.get_root()->first_node("Terrain"));
    // Parse camera information
    parse_camera(xml_parser_.get_root()->first_node("Camera"));
    // Parse directional light
    parse_directional_light(xml_parser_.get_root()->first_node("Light"));
    // Parse ambient parameters
    parse_ambient(daylight, xml_parser_.get_root()->first_node("Ambient"));
    // Parse skybox declaration if any
    parse_skybox(xml_parser_.get_root()->first_node("Skybox"));
    // Preload mesh instances used in this level
    preload_instances();
}

void SceneLoader::preload_instances()
{
    // * In level file, detect Model and Mesh nodes with name attribute

    // To keep track of already loaded meshes
    std::set<hash_t> loaded_mesh_instances;
    std::set<hash_t> loaded_mesh_model_instances;
    std::set<hash_t> loaded_mesh_entities;

    // For each chunk
    for (xml_node<>* ck_node=xml_parser_.get_root()->first_node("Chunk");
         ck_node;
         ck_node=ck_node->next_sibling("Chunk"))
    {
        xml_node<>* mdls_node = ck_node->first_node("Models");
        if(mdls_node != nullptr)
        {
            // For each model
            for (xml_node<>* mdl_node=mdls_node->first_node("Model");
                 mdl_node;
                 mdl_node=mdl_node->next_sibling("Model"))
            {
                std::shared_ptr<SurfaceMesh> pmesh = nullptr;
                // Look for Model node name attribute
                std::string model_name;
                if(xml::parse_attribute(mdl_node, "name", model_name))
                {
                    hash_t hname = H_(model_name.c_str());
                    if(loaded_mesh_model_instances.find(hname) == loaded_mesh_model_instances.end())
                    {
                        DLOGN("Preloading mesh instance:", "model");
                        DLOGI("Model name: <n>" + model_name + "</n>", "model");
                        pmesh = game_object_factory_->preload_mesh_model_instance(hname);
                        loaded_mesh_model_instances.insert(hname);
                    }
                }
                // Look for child Mesh node with name attribute
                else
                {
                    xml_node<>* mesh_node = mdl_node->first_node("Mesh");
                    if(mesh_node)
                    {
                        std::string mesh_name;
                        if(xml::parse_attribute(mesh_node, "name", mesh_name))
                        {
                            hash_t hname = H_(mesh_name.c_str());
                            if(loaded_mesh_instances.find(hname) == loaded_mesh_instances.end())
                            {
                                DLOGN("Preloading mesh instance:", "model");
                                DLOGI("Mesh name: <n>" + mesh_name + "</n>", "model");
                                pmesh = game_object_factory_->preload_mesh_instance(hname);
                                loaded_mesh_instances.insert(hname);
                            }
                        }
                    }
                }
                // Load mesh in scene
                if(pmesh != nullptr)
                {
                    pscene_->submit_mesh_instance(pmesh);
                }
            }
        }

        xml_node<>* ents_node = ck_node->first_node("Entities");
        if(ents_node != nullptr)
        {
            // For each entity
            for (xml_node<>* ent_node=ents_node->first_node("Entity");
                 ent_node;
                 ent_node=ent_node->next_sibling("Entity"))
            {
                hash_t blueprint = xml::parse_attribute_h(ent_node, "blueprint");
                if(blueprint == 0) continue;

                std::shared_ptr<SurfaceMesh> pmesh = nullptr;
                if(loaded_mesh_entities.find(blueprint) == loaded_mesh_entities.end())
                {
                    pmesh = game_object_factory_->preload_mesh_entity(blueprint);
                    loaded_mesh_entities.insert(blueprint);
                }

                if(pmesh != nullptr)
                {
                    DLOGN("Preloading mesh instance:", "model");
                    DLOGI("Entity blueprint: <n>" + std::to_string(blueprint) + "</n>", "model");
                    pscene_->submit_mesh_instance(pmesh);
                }
            }
        }
    }
    // Upload scene meshes
    pscene_->load_instance_geometry();
}


void SceneLoader::parse_audio(rapidxml::xml_node<>* node)
{
    if(node == nullptr) return;

    // Locate sound system
    auto psound_ = locate<SoundSystem>("SoundSystem"_h);

    // Parse background music
    std::string bgm_name;
    if(xml::parse_node(node, "BGM", bgm_name))
        psound_->play_bgm(H_(bgm_name.c_str()));
}

void SceneLoader::parse_directional_light(rapidxml::xml_node<>* node)
{
    std::string light_type;
    vec3 position;
    vec3 color;
    float brightness;
    bool success = true;
    success &= xml::parse_attribute(node, "type", light_type);
    success &= xml::parse_node(node, "Position", position);
    success &= xml::parse_node(node, "Color", color);
    success &= xml::parse_node(node, "Brightness", brightness);
    if(!success)
    {
        DLOGE("[SceneLoader] Ill-formed directional light declaration.", "parsing");
        return;
    }

    float ambient_strength = 0.03;
    xml::parse_node(node, "AmbientStrength", ambient_strength);

    if(!light_type.compare("directional"))
    {
        std::shared_ptr<Light> dirlight(new DirectionalLight(position.normalized(),
                                                             color,
                                                             brightness));
        dirlight->set_ambient_strength(ambient_strength);
        pscene_->add_directional_light(dirlight);
    }

    xml_node<>* ambient_node = xml_parser_.get_root()->first_node("Ambient");
    if(!ambient_node) return;

    xml_node<>* dir_node = ambient_node->first_node("Directional");
    if(!dir_node) return;

    float shadow_bias = 0.1f;
    if(xml::parse_node(dir_node, "ShadowBias", shadow_bias))
    {
        RenderPipeline* ppipeline_ = locate<RenderPipeline>("Pipeline"_h);
        ppipeline_->set_shadow_bias(shadow_bias);
    }
}

void SceneLoader::parse_patches(rapidxml::xml_node<>* node)
{
    // Get global terrain attributes
    xml::parse_attribute(node, "chunkSize", chunk_size_m_);
    xml::parse_attribute(node, "textureScale", texture_scale_);
    xml::parse_attribute(node, "latticeScale", lattice_scale_);
    texture_scale_ *= lattice_scale_;
    // Hex mesh terrain chunks must have an odd size
    if(chunk_size_m_%2==0)
    {
        ++chunk_size_m_;
        DLOGW("[SceneLoader] Chunk size must be <v>odd</v> for hex terrain meshes.", "chunk");
        DLOGI("<i>Corrected</i>: Chunk Size is now " + std::to_string(chunk_size_m_), "chunk");
    }
    chunk_size_ = uint32_t(floor(chunk_size_m_/lattice_scale_));
    pscene_->set_chunk_size_meters(chunk_size_m_);
    TerrainChunk::set_chunk_size(chunk_size_);

    // For each terrain patch
    for(xml_node<>* patch=node->first_node("TerrainPatch");
        patch;
        patch=patch->next_sibling("TerrainPatch"))
    {
        // Parse size and starting coordinates
        i32vec2 patchStart, patchSize;
        bool success = true;
        success &= xml::parse_attribute(patch, "origin", patchStart);
        success &= xml::parse_attribute(patch, "size", patchSize);
        if(!success)
        {
            DLOGE("[SceneLoader] TerrainPatch node must define 'origin' and 'size' attributes.", "parsing");
        }

        // Check if patch is void (empty, no terrain, zitch)
        bool patch_is_void = false;
        if(xml::parse_attribute(patch, "void", patch_is_void))
        {
            if(patch_is_void)
            {
                std::stringstream ss;
                ss << "Terrain patch at <v>" << patchStart << "</v> is <h>void</h>";
                DLOGI(ss.str(), "chunk");
            }
        }

        // For each chunk within that patch
        for(uint32_t xx=patchStart.x(); xx<patchStart.x()+patchSize.x(); ++xx)
        {
            for(uint32_t yy=patchStart.y(); yy<patchStart.y()+patchSize.y(); ++yy)
            {
                // Compute chunk index
                uint32_t chunk_index = std::hash<i32vec2>{}(i32vec2(xx,yy));
                // Add/Override reference to patch node in table
#ifdef __DEBUG__
                auto it = chunk_patches_.find(chunk_index);
                if(it!=chunk_patches_.end())
                {
                    DLOGW("[SceneLoader] Silent chunk override:", "scene");
                    std::stringstream ss;
                    ss << "Chunk index= <n>" << chunk_index << "</n>, "
                       << "coordinates= <v>" << i32vec2(xx,yy) << "</v>";
                    DLOGI(ss.str(), "scene");

                    ss.str("");
                    ss << "By patch of origin= <v>" << patchStart << "</v>, "
                       << "size= " << patchSize;
                    DLOGI(ss.str(), "scene");
                }
#endif
                chunk_patches_[chunk_index] = patch_is_void ? nullptr : patch;
            }
        }
    }
}

void SceneLoader::parse_ambient(DaylightSystem& daylight, rapidxml::xml_node<>* node)
{
    if(!node)
    {
        DLOGW("[SceneLoader] No Ambient node.", "parsing");
        return;
    }
    xml_node<>* dir_node = node->first_node("Directional");
    if(!dir_node)
    {
        DLOGW("[SceneLoader] No Ambient.Directional node.", "parsing");
        return;
    }
    xml_node<>* pp_node = node->first_node("PostProcessing");
    if(!pp_node)
    {
        DLOGW("[SceneLoader] No Ambient.PostProcessing node.", "parsing");
        return;
    }
    xml_node<>* c_node = dir_node->first_node("Color");
    xml_node<>* b_node = dir_node->first_node("Brightness");
    xml_node<>* a_node = dir_node->first_node("AmbientStrength");
    xml_node<>* g_node = pp_node->first_node("Gamma");
    xml_node<>* s_node = pp_node->first_node("Saturation");
    xml_node<>* f_node = pp_node->first_node("FogDensity");
    if(!c_node || !b_node || !a_node || !g_node || !s_node || !f_node)
    {
        DLOGW("[SceneLoader] Ill-formed Ambient hierarchy.", "parsing");
        return;
    }

    CSplineCatmullV3* color_interpolator            = new CSplineCatmullV3(get_num_controls(c_node->first_node("CSpline")));
    CSplineCatmullV3* pp_gamma_interpolator         = new CSplineCatmullV3(get_num_controls(g_node->first_node("CSpline")));;
    CSplineCatmullF*  brightness_interpolator       = new CSplineCatmullF(get_num_controls(b_node->first_node("CSpline")));;
    CSplineCatmullF*  pp_saturation_interpolator    = new CSplineCatmullF(get_num_controls(s_node->first_node("CSpline")));;
    CSplineCardinalF* pp_fog_density_interpolator   = new CSplineCardinalF(get_num_controls(f_node->first_node("CSpline")));;
    CSplineCardinalF* ambient_strength_interpolator = new CSplineCardinalF(get_num_controls(a_node->first_node("CSpline")));;

    parse_cspline(c_node->first_node("CSpline"), *color_interpolator);
    parse_cspline(g_node->first_node("CSpline"), *pp_gamma_interpolator);
    parse_cspline(b_node->first_node("CSpline"), *brightness_interpolator);
    parse_cspline(s_node->first_node("CSpline"), *pp_saturation_interpolator);
    parse_cspline(f_node->first_node("CSpline"), *pp_fog_density_interpolator);
    parse_cspline(a_node->first_node("CSpline"), *ambient_strength_interpolator);

    daylight.set_color_interp(color_interpolator);
    daylight.set_gamma_interp(pp_gamma_interpolator);
    daylight.set_brightness_interp(brightness_interpolator);
    daylight.set_saturation_interp(pp_saturation_interpolator);
    daylight.set_fog_density_interp(pp_fog_density_interpolator);
    daylight.set_ambient_strength_interp(ambient_strength_interpolator);
}

void SceneLoader::parse_skybox(rapidxml::xml_node<>* node)
{
    // Skybox is optional, if no Skybox node just return
    if(!node) return;

    // Check if skybox is disabled
    bool disabled = false;
    if(xml::parse_attribute(node, "disable", disabled))
        if(disabled) return;

    // Look for a CubemapTexture node and extract name
    xml_node<>* cmap_node = node->first_node("CubemapTexture");
    if(!cmap_node) return;

    std::string cmap_name;
    if(xml::parse_attribute(cmap_node, "name", cmap_name))
    {
        hash_t cmap_id = H_(cmap_name.c_str());
        auto sb = game_object_factory_->make_skybox(cmap_id);
        if(sb)
        {
            DLOGN("[SceneLoader] Using skybox:", "scene");
            DLOGI("cubemap: <n>" + cmap_name + "</n>", "scene");
            pscene_->set_skybox(sb);
        }
    }
}

uint32_t SceneLoader::load_chunk(const i32vec2& chunk_coords, bool finalize)
{
    // Compute chunk index, find corresponding node and load content
    uint32_t chunk_index = std::hash<i32vec2>{}(chunk_coords);
    auto it = chunk_nodes_.find(chunk_index);
    if(it == chunk_nodes_.end())
    {
#ifdef __DEBUG__
        DLOGW("[SceneLoader] Ignoring non existing chunk:", "chunk");
        std::stringstream ss;
        ss << "at coordinates <v>" << chunk_coords << "</v>";
        DLOGI(ss.str(), "chunk");
#endif
        return 0;
    }
    else
    {
#ifdef __DEBUG__
        std::stringstream ss;
        ss << "[SceneLoader] Loading chunk: <n>" << chunk_index << "</n>"
           << " at <v>" << chunk_coords << "</v>";
        DLOGN(ss.str(), "chunk");
#endif
    }

#ifdef __PROFILING_CHUNKS__
    float dt_terrain_us,
          dt_models_us,
          dt_batches_us,
          dt_lights_us,
          dt_entities_us,
          dt_upload_us;
#endif

    pscene_->add_chunk(chunk_coords);
    xml_node<>* chunk_node = it->second;

    // LOADING TERRAIN CHUNK
#ifdef __PROFILING_CHUNKS__
    profile_clock_.restart();
#endif
    parse_terrain(chunk_coords);
#ifdef __PROFILING_CHUNKS__
    auto period = profile_clock_.get_elapsed_time();
    dt_terrain_us = 1e6*std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
#endif

    // LOADING MODELS
#ifdef __PROFILING_CHUNKS__
    profile_clock_.restart();
#endif
    parse_models(chunk_node, chunk_index);
    //parse_line_models(chunk_node, chunk_index);
#ifdef __PROFILING_CHUNKS__
    period = profile_clock_.get_elapsed_time();
    dt_models_us = 1e6*std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
#endif

    // LOADING BATCHES
#ifdef __PROFILING_CHUNKS__
    profile_clock_.restart();
#endif
    parse_model_batches(chunk_node, chunk_index);
#ifdef __PROFILING_CHUNKS__
    period = profile_clock_.get_elapsed_time();
    dt_batches_us = 1e6*std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
#endif

    // LOADING ENTITIES
#ifdef __PROFILING_CHUNKS__
    profile_clock_.restart();
#endif
    // but just once
    if(chunk_loaded_once_.find(chunk_index) == chunk_loaded_once_.end())
        parse_entities(chunk_node, chunk_index);
#ifdef __PROFILING_CHUNKS__
    period = profile_clock_.get_elapsed_time();
    dt_entities_us = 1e6*std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
#endif

    // LOADING LIGHTS
#ifdef __PROFILING_CHUNKS__
    profile_clock_.restart();
#endif
    parse_lights(chunk_node, chunk_index);
#ifdef __PROFILING_CHUNKS__
    period = profile_clock_.get_elapsed_time();
    dt_lights_us = 1e6*std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
#endif

    // UPLOADING TO OPENGL
#ifdef __PROFILING_CHUNKS__
    profile_clock_.restart();
#endif
    if(finalize)
        pscene_->load_geometry(chunk_index);
#ifdef __PROFILING_CHUNKS__
    period = profile_clock_.get_elapsed_time();
    dt_upload_us = 1e6*std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
#endif

    pscene_->populate_static_octree(chunk_index);

#ifdef __PROFILING_CHUNKS__
    float dt_total_us = dt_terrain_us + dt_models_us + dt_batches_us + dt_lights_us + dt_upload_us;
    std::stringstream ss;
    ss << "Chunk loaded in <v>" << dt_total_us << "</v> µs total.";
    DLOGI(ss.str(), "chunk");

    ss.str("");
    ss << "Terrain: <v>" << dt_terrain_us << "</v> µs";
    DLOGI(ss.str(), "chunk");

    ss.str("");
    ss << "Models: <v>" << dt_models_us << "</v> µs";
    DLOGI(ss.str(), "chunk");

    ss.str("");
    ss << "Model Batches: <v>" << dt_batches_us << "</v> µs";
    DLOGI(ss.str(), "chunk");

    ss.str("");
    ss << "Entities: <v>" << dt_entities_us << "</v> µs";
    DLOGI(ss.str(), "chunk");

    ss.str("");
    ss << "Lights: <v>" << dt_lights_us << "</v> µs";
    DLOGI(ss.str(), "chunk");

    ss.str("");
    ss << "Geometry upload: <v>" << dt_upload_us << "</v> µs";
    DLOGI(ss.str(), "chunk");
#endif

    // Add chunk index to set of chunks that were loaded once
    chunk_loaded_once_.insert(chunk_index);

    return chunk_index;
}

void SceneLoader::reload_chunks()
{
    std::vector<i32vec2> coords;
    pscene_->get_loaded_chunks_coords(coords);

    pscene_->clear_chunks();
    for(uint32_t ii=0; ii<coords.size(); ++ii)
        load_chunk(coords[ii]);
}

void SceneLoader::reload_map()
{
    xml_parser_.reset();

    load_level(current_map_.c_str());
    reload_chunks();
}

void SceneLoader::parse_terrain(const i32vec2& chunk_coords)
{
    // Look for correct patch node in chunk/patch table
    // given chunk coordinates
    uint32_t chunk_index = std::hash<i32vec2>{}(chunk_coords);
    auto it = chunk_patches_.find(chunk_index);
    if(it==chunk_patches_.end())
    {
        std::stringstream ss;
        ss << "[SceneLoader] <b>Orphan chunk</b> at <v>" << chunk_coords << "</v>"
           << " (chunk is outside of all terrain patches).";
        DLOGE(ss.str(), "chunk");
        fatal(ss.str());
    }
    xml_node<>* patch = it->second;

    // If patch is null, we have a void patch and should not create a terrain
    if(patch == nullptr)
    {
        DLOG("Void patch", "chunk", Severity::LOW);
        return;
    }

    // Get nodes
    xml_node<>* splat_node = patch->first_node("Splat");
    xml_node<>* mat_node = patch->first_node("Material");
    xml_node<>* trn_node = patch->first_node("Transform");
    xml_node<>* shadow_node = patch->first_node("Shadow");
    xml_node<>* generator_node = patch->first_node("Generator");
    xml_node<>* height_modifier_node = patch->first_node("HeightModifier");

    // Get terrain patch attributes
    float height = 0.0f;
    xml::parse_attribute(patch, "height", height);

    TerrainPatchDescriptor desc;
    desc.chunk_size = chunk_size_;
    desc.chunk_index = chunk_index;
    desc.chunk_x = chunk_coords.x();
    desc.chunk_z = chunk_coords.y();
    desc.lattice_scale = lattice_scale_;
    desc.texture_scale = texture_scale_;
    desc.height = height;
    desc.generator_node = generator_node;
    desc.height_modifier_node = height_modifier_node;

    // Check if terrain uses splat mapping
    if(splat_node)
    {
        // Push all materials in order
        for(xml_node<>* splat_mat_node=splat_node->first_node("Material");
            splat_mat_node;
            splat_mat_node=splat_mat_node->next_sibling("Material"))
        {
            desc.material_nodes.push_back(splat_mat_node);
        }
        // Generate splatmap name
        desc.splatmap_name = "splat_" + current_map_ + "_" + std::to_string(desc.chunk_x) + "_" + std::to_string(desc.chunk_z) + ".png";
    }
    else
    {
        if(mat_node)
            desc.material_nodes.push_back(mat_node);
        else
        {
            DLOGE("[SceneLoader] No Material node detected in TerrainPatch.", "parsing");
            return;
        }
    }

    pTerrain terrain = game_object_factory_->make_terrain_patch(desc);

    // Fix new terrain edge normals and tangents
    terrain::stitch_terrain_edges(pscene_, *terrain, chunk_index, chunk_size_);

    // Spacial transformation
    Transformation trans;
    parse_transformation(trn_node, trans);
    // Translate according to chunk coordinates
    trans.translate((chunk_size_m_-lattice_scale_)*chunk_coords.x(),
                    0,
                    (chunk_size_m_-lattice_scale_)*chunk_coords.y());
    terrain->set_transformation(trans);

    // Shadow options
    if(shadow_node)
    {
        uint32_t cull_face=0;
        if(xml::parse_node(shadow_node, "CullFace", cull_face))
            terrain->set_shadow_cull_face(cull_face);
    }

    terrain->update_bounding_boxes();
    pscene_->add_terrain(terrain, chunk_index);
}

void SceneLoader::parse_models(xml_node<>* chunk_node, uint32_t chunk_index)
{
    xml_node<>* mdls_node = chunk_node->first_node("Models");
    if(!mdls_node) return;

    std::mt19937 rng(0);
    for (xml_node<>* model=mdls_node->first_node("Model"); model; model=model->next_sibling("Model"))
    {
        // Get nodes
        xml_node<>* mat_node = model->first_node("Material");
        xml_node<>* mesh_node = model->first_node("Mesh");
        xml_node<>* trn_node = model->first_node("Transform");
        xml_node<>* mot_node = model->first_node("Motion");
        xml_node<>* shadow_node = model->first_node("Shadow");
        xml_node<>* override_node = model->first_node("Override");

        // Do we position the models relative to a heightmap?
        bool relative_positioning = is_pos_relative(model);
        // Is model an instance?
        bool mesh_is_instance = false;

        pModel pmdl;
        if(!mat_node || !mesh_node)
        {
            // Try to make model by instance name
            std::string name;
            if(xml::parse_attribute(model, "name", name))
            {
                hash_t hname = H_(name.c_str());
                pmdl = game_object_factory_->make_model_instance(hname);
                mesh_is_instance = true;
            }
            else continue;
        }
        else
        {
            pmdl = game_object_factory_->make_model(mesh_node, mat_node, mesh_is_instance, &rng);
        }

        // Spacial transformation
        if(trn_node)
        {
            Transformation trans;
            parse_transformation(trn_node, trans);

            pmdl->set_transformation(trans);
            // Is the y position specified relative to a height map?
            if(relative_positioning)
            {
                // If so, translate model using chunk heightmap
                ground_model(*pmdl, chunk_index);
            }

            // Translate according to chunk coordinates
            auto chunk_coords = pscene_->get_chunk_coordinates(chunk_index);
            pmdl->translate((chunk_size_m_-1)*chunk_coords.x(),
                            0,
                            (chunk_size_m_-1)*chunk_coords.y());
        }

        // Shadow options
        if(shadow_node)
        {
            uint32_t cull_face=0;
            if(xml::parse_node(shadow_node, "CullFace", cull_face))
                pmdl->set_shadow_cull_face(cull_face);
        }

        // Motion
        if(mot_node)
        {
            parse_motion(mot_node, pmdl, chunk_index);
        }

        // Override
        if(override_node)
        {
            bool can_cull = true;
            if(xml::parse_node(override_node, "EnableCulling", can_cull))
                pmdl->set_frustum_cull(can_cull);
        }

        // Should we save a reference so that the model can be accessed via a hash?
#ifdef __DEBUG__
        std::string ref;
        if(xml::parse_attribute(model, "href", ref))
        {
            hash_t href = H_(ref.c_str());
            pmdl->set_reference(href);
            HRESOLVE.add_intern_string(ref);
        }
#else
        if(hash_t href = xml::parse_attribute_h(model, "href"))
            pmdl->set_reference(href);
#endif

        pmdl->update_bounding_boxes();
        if(!mesh_is_instance)
            pscene_->add_model(pmdl, chunk_index);
        else
            pscene_->add_model_instance(pmdl, chunk_index);
    }
}
/*
void SceneLoader::parse_line_models(xml_node<>* chunk_node, uint32_t chunk_index)
{
    xml_node<>* mdls_node = chunk_node->first_node("Models");
    if(!mdls_node) return;

    for (xml_node<>* lmodel=mdls_node->first_node("LineModel"); lmodel; lmodel=lmodel->next_sibling("LineModel"))
    {
        // Get nodes
        xml_node<>* mat_node = lmodel->first_node("Material");
        xml_node<>* mesh_node = lmodel->first_node("Mesh");
        if(!mat_node || !mesh_node) continue;

        xml_node<>* trn_node = lmodel->first_node("Transform");

        // Do we position the model relative to a heightmap?
        bool relative_positioning = is_pos_relative(lmodel);

        // Generate mesh and material, then construct line model
        Material* pmat = game_object_factory_->material_factory()->make_material(mat_node);
        Mesh<Vertex3P>* pmesh = parse_line_mesh(mesh_node);
        if(!pmesh)
        {
            DLOGW("[SceneLoader] Skipping incomplete mesh declaration.", "parsing");
            if(pmat)
                delete pmat;
            continue;
        }
        pLineModel pmdl = std::make_shared<LineModel>(pmesh, pmat);

        // Spacial transformation
        if(trn_node)
        {
            Transformation trans;
            parse_transformation(trn_node, trans);

            pmdl->set_transformation(trans);
            // Is the y position specified relative to a height map?
            if(relative_positioning)
            {
                // If so, translate model using chunk heightmap
                ground_model(*pmdl, chunk_index);
            }

            // Translate according to chunk coordinates
            auto chunk_coords = pscene_->get_chunk_coordinates(chunk_index);
            pmdl->translate((chunk_size_m_-1)*chunk_coords.x(),
                            0,
                            (chunk_size_m_-1)*chunk_coords.y());
        }

        pscene_->add_model(pmdl, chunk_index);
    }
}*/

void SceneLoader::parse_model_batches(xml_node<>* chunk_node, uint32_t chunk_index)
{
    xml_node<>* bat_node = chunk_node->first_node("ModelBatches");
    if(!bat_node) return;

    for (xml_node<>* batch=bat_node->first_node("ModelBatch"); batch; batch=batch->next_sibling())
    {
        // Get nodes
        xml_node<>* mat_node = batch->first_node("Material");
        xml_node<>* mesh_node = batch->first_node("Mesh");
        if(!mat_node || !mesh_node)
            continue;
        xml_node<>* shadow_node = batch->first_node("Shadow");
        xml_node<>* trn_node = batch->first_node("Transform");

        uint32_t instances, seed;
        xml::parse_attribute(batch, "instances", instances);
        xml::parse_attribute(batch, "seed", seed);

        // Do we position the models relative to a heightmap?
        bool relative_positioning = is_pos_relative(batch);

        std::mt19937 rng;
        rng.seed(seed);
        std::uniform_int_distribution<uint32_t> mesh_seed(0,std::numeric_limits<uint32_t>::max());
        std::uniform_real_distribution<float> var_distrib(-1.0f,1.0f);

        // Generate batch transformations
        std::vector<Transformation> transforms;
        parse_transformation(trn_node, instances, transforms, rng);

        for(uint32_t ii=0; ii<instances; ++ii)
        {
            bool mesh_is_instance = false;
            pModel pmdl = game_object_factory_->make_model(mesh_node, mat_node, mesh_is_instance, &rng);

            // Transform
            pmdl->set_transformation(transforms[ii]);

            // Is the y position specified relative to a height map?
            if(relative_positioning)
            {
                // If so, translate model using chunk height map.
                ground_model(*pmdl, chunk_index);
            }
            // Translate according to chunk coordinates
            auto chunk_coords = pscene_->get_chunk_coordinates(chunk_index);
            pmdl->translate((chunk_size_m_-1)*chunk_coords.x(),
                            0,
                            (chunk_size_m_-1)*chunk_coords.y());

            if(shadow_node)
            {
                uint32_t cull_face=0;
                if(xml::parse_node(shadow_node, "CullFace", cull_face))
                    pmdl->set_shadow_cull_face(cull_face);
            }

            pmdl->update_bounding_boxes();

            if(!mesh_is_instance)
                pscene_->add_model(pmdl, chunk_index);
            else
                pscene_->add_model_instance(pmdl, chunk_index);
        }
    }
}

void SceneLoader::parse_entities(rapidxml::xml_node<>* chunk_node, uint32_t chunk_index)
{
    xml_node<>* ents_node = chunk_node->first_node("Entities");
    if(!ents_node) return;

    auto* entity_system = locate<EntitySystem>("EntitySystem"_h);
    for (xml_node<>* ent=ents_node->first_node("Entity"); ent; ent=ent->next_sibling("Entity"))
    {
        // Get blueprint name
        hash_t blueprint = xml::parse_attribute_h(ent, "blueprint");
        if(blueprint == 0) continue;

        // Create entity from blueprint
        auto pEntity = game_object_factory_->make_entity_blueprint(blueprint);
        if(pEntity == nullptr) continue;

        // Get Transform node
        xml_node<>* trn_node = ent->first_node("Transform");
        if(trn_node != nullptr && pEntity->has_component<component::WCModel>())
        {
            // Apply transformation
            Transformation trans;
            parse_transformation(trn_node, trans);

            auto& pmdl = pEntity->get_component<component::WCModel>()->model;
            pmdl->set_transformation(trans);

            if(is_pos_relative(ent))
                ground_model(*pmdl, chunk_index);

            // Translate according to chunk coordinates
            auto chunk_coords = pscene_->get_chunk_coordinates(chunk_index);
            pmdl->translate((chunk_size_m_-1)*chunk_coords.x(),
                            0,
                            (chunk_size_m_-1)*chunk_coords.y());

            pmdl->update_bounding_boxes();
        }

        entity_system->add_entity(pEntity);
    }
}


void SceneLoader::parse_lights(xml_node<>* chunk_node, uint32_t chunk_index)
{
    xml_node<>* lit_nodes = chunk_node->first_node("Lights");
    if(!lit_nodes) return;

    for (xml_node<>* light=lit_nodes->first_node("Light"); light; light=light->next_sibling())
    {
        std::string light_type;
        vec3 position;
        vec3 color;
        float brightness;
        bool success = true;
        success &= xml::parse_attribute(light, "type", light_type);
        success &= xml::parse_node(light, "Position", position);
        success &= xml::parse_node(light, "Color", color);
        success &= xml::parse_node(light, "Brightness", brightness);
        if(!success) continue;

        float ambient_strength = 0.03;
        xml::parse_node(light, "AmbientStrength", ambient_strength);

        // Do we position the light relative to a heightmap?
        if(is_pos_relative(light) && pscene_->has_terrain(chunk_index))
        {
            float height = pscene_->get_heightmap(chunk_index).get_height(position.xz());
            position[1] += height;
        }

        if(!light_type.compare("directional"))
        {
            std::shared_ptr<Light> dirlight(new DirectionalLight(position.normalized(),
                                                                 color,
                                                                 brightness));
            dirlight->set_ambient_strength(ambient_strength);
            pscene_->add_directional_light(dirlight);
        }
        else if(!light_type.compare("point"))
        {
            float radius = 10.f;
            xml::parse_node(light, "Radius", radius);

            // Translate according to chunk coordinates
            auto chunk_coords = pscene_->get_chunk_coordinates(chunk_index);
            vec3 offset((chunk_size_m_-1)*chunk_coords.x(),
                        0,
                        (chunk_size_m_-1)*chunk_coords.y());

            std::shared_ptr<Light> pointlight(new PointLight(position+offset,
                                                             color,
                                                             radius,
                                                             brightness));
            pointlight->set_ambient_strength(ambient_strength);

            // Motion
            xml_node<>* mot_node = light->first_node("Motion");
            if(mot_node)
            {
                parse_motion(mot_node, pointlight, chunk_index);
            }

        // Should we save a reference so that the light can be accessed via a hash?
#ifdef __DEBUG__
            std::string ref;
            if(xml::parse_attribute(light, "href", ref))
            {
                hash_t href = H_(ref.c_str());
                pointlight->set_reference(href);
                HRESOLVE.add_intern_string(ref);
            }
#else
            if(hash_t href = xml::parse_attribute_h(light, "href"))
                pointlight->set_reference(href);
#endif
            pscene_->add_light(pointlight, chunk_index);
        }
    }
}

void SceneLoader::parse_camera(xml_node<>* node)
{
    if(!node) return;

    vec3 position;
    if(xml::parse_node(node, "Position", position))
        pscene_->get_camera().set_position(position);

    vec2 orientation;
    if(xml::parse_node(node, "Orientation", orientation))
        pscene_->get_camera().set_orientation(orientation.x(),orientation.y());
}

void SceneLoader::parse_transformation(xml_node<>* trn_node, Transformation& trans)
{
    xml_node<>* pos_node = trn_node->first_node("Position");
    if(pos_node)
    {
        vec3 position;
        str_val(pos_node->value(), position);

        trans.set_position(position);
    }

    xml_node<>* ang_node = trn_node->first_node("Angle");
    if(ang_node)
    {
        vec3 angle;
        str_val(ang_node->value(), angle);

        trans.rotate(angle.x(), angle.y(), angle.z());
    }

    xml_node<>* scl_node = trn_node->first_node("Scale");
    if(scl_node)
    {
        float scale;
        str_val(scl_node->value(), scale);

        trans.set_scale(scale);
    }
}

void SceneLoader::parse_transformation(rapidxml::xml_node<>* trn_node,
                          uint32_t n_instances,
                          std::vector<Transformation>& trans_vector,
                          std::mt19937& rng)
{
    xml_node<>* pos_node = trn_node->first_node("Position");
    xml_node<>* ang_node = trn_node->first_node("Angle");
    xml_node<>* scl_node = trn_node->first_node("Scale");

    vec3 position, position_var;
    vec3 angle, angle_var;
    float scale, scale_var;

    bool pos_has_variance = false;
    bool ang_has_variance = false;
    bool scl_has_variance = false;

    std::uniform_real_distribution<float> var_distrib(-1.0f,1.0f);

    if(pos_node)
    {
        str_val(pos_node->value(), position);
        pos_has_variance = xml::parse_attribute(pos_node, "variance", position_var);
    }
    if(ang_node)
    {
        str_val(ang_node->value(), angle);
        ang_has_variance = xml::parse_attribute(ang_node, "variance", angle_var);
    }
    if(scl_node)
    {
        str_val(scl_node->value(), scale);
        scl_has_variance = xml::parse_attribute(scl_node, "variance", scale_var);
    }

    for(uint32_t ii=0; ii<n_instances; ++ii)
    {
        Transformation trans;
        if(pos_node)
        {
            vec3 inst_position(position);
            if(pos_has_variance)
                inst_position += vec3(position_var.x() * var_distrib(rng),
                                      position_var.y() * var_distrib(rng),
                                      position_var.z() * var_distrib(rng));
            trans.set_position(inst_position);
        }
        if(ang_node)
        {
            vec3 inst_angle(angle);
            if(ang_has_variance)
                inst_angle += vec3(angle_var.x() * var_distrib(rng),
                                   angle_var.y() * var_distrib(rng),
                                   angle_var.z() * var_distrib(rng));
            trans.rotate(inst_angle.x(), inst_angle.y(), inst_angle.z());
        }
        if(scl_node)
        {
            float inst_scale = scale;;
            if(scl_has_variance)
                inst_scale += scale_var * var_distrib(rng);
            trans.set_scale(inst_scale);
        }
        trans_vector.push_back(trans);
    }
}

/*
Mesh<Vertex3P>* SceneLoader::parse_line_mesh(rapidxml::xml_node<>* mesh_node)
{
    if(!mesh_node)
        return nullptr;

    std::string mesh;
    if(!str_val(mesh_node->value(), mesh))
        return nullptr;

    Mesh<Vertex3P>* pmesh = nullptr;
    if(!mesh.compare("tree"))
    {
        // Procedural tree mesh, look for TreeGenerator node
        xml_node<>* tg_node = mesh_node->first_node("TreeGenerator");
        if(!tg_node)
            return nullptr;

        TreeProps props;
        props.parse_xml(tg_node);

        pmesh = TreeGenerator::generate_spline_tree(props);
    }

    return pmesh;
}*/

void SceneLoader::ground_model(Model& model, const HeightMap& hm)
{
    // Find height at model (x,z) position and apply offset to model
    float height = hm.get_height(model.get_position().xz());
    model.translate_y(height);
}

void SceneLoader::ground_model(Model& model, uint32_t chunk_index)
{
    if(pscene_->has_terrain(chunk_index))
    {
        // Find height at model (x,z) position and apply offset to model
        float height = pscene_->get_heightmap(chunk_index).get_height(model.get_position().xz());
        model.translate_y(height);
    }
}

void SceneLoader::ground_model(LineModel& model, uint32_t chunk_index)
{
    if(pscene_->has_terrain(chunk_index))
    {
        // Find height at model (x,z) position and apply offset to model
        float height = pscene_->get_heightmap(chunk_index).get_height(model.get_position().xz());
        model.translate_y(height);
    }
}

void SceneLoader::parse_bezier_interpolator(rapidxml::xml_node<>* bez_node,
                                            std::vector<math::vec3>& controls,
                                            uint32_t chunk_index)
{
    bool relative_positioning = is_pos_relative(bez_node);

    for (xml_node<>* control=bez_node->first_node("Control");
         control; control=control->next_sibling("Control"))
    {
        vec3 control_point;
        if(str_val(control->value(), control_point))
        {
            if(relative_positioning)
            {
                float height = pscene_->get_heightmap(chunk_index).get_height(control_point.xz());
                control_point[1] += height;
            }
            controls.push_back(control_point);
        }
    }
}

void SceneLoader::parse_motion(rapidxml::xml_node<>* mot_node, pModel target_model, uint32_t chunk_index)
{
    xml_node<>* pu_node = mot_node->first_node("PositionUpdater");
    if(pu_node)
    {
        vec3& target_pos = target_model->get_transformation().get_position_nc();
        PositionUpdater* updater = parse_position_updater(pu_node, target_pos, chunk_index);
        if(updater)
        {
            target_model->set_dynamic(true);
            pscene_->add_position_updater(updater, chunk_index);
        }
    }

    xml_node<>* cr_node = mot_node->first_node("ConstantRotator");
    if(cr_node)
    {
        vec3 angular_rate;
        if(xml::parse_property(cr_node, "angular_rate", angular_rate))
        {
            ConstantRotator* rotator = new ConstantRotator(target_model, angular_rate);
            target_model->set_dynamic(true);
            pscene_->add_rotator(rotator, chunk_index);
        }
    }
}

void SceneLoader::parse_motion(rapidxml::xml_node<>* mot_node, pLight target_light, uint32_t chunk_index)
{
    xml_node<>* pu_node = mot_node->first_node("PositionUpdater");
    if(pu_node)
    {
        vec3& target_pos = target_light->get_position_nc();
        PositionUpdater* updater = parse_position_updater(pu_node, target_pos, chunk_index);
        if(updater)
            pscene_->add_position_updater(updater, chunk_index);
    }
}


PositionUpdater* SceneLoader::parse_position_updater(rapidxml::xml_node<>* pu_node,
                                                     math::vec3& target,
                                                     uint32_t chunk_index)
{
    std::string tSpace("cycle");
    float stepScale = 1.0f;
    float tMax = 1.0f;
    float tMin = 0.0f;

    xml::parse_property(pu_node, "tSpace", tSpace);
    xml::parse_property(pu_node, "stepScale", stepScale);
    xml::parse_property(pu_node, "tMax", tMax);
    xml::parse_property(pu_node, "tMin", tMin);

    timeEvolution::TimeUpdater* tupd = nullptr;
    if(!tSpace.compare("alternate"))
        tupd = new timeEvolution::Alternating();
    else if(!tSpace.compare("cycle"))
        tupd = new timeEvolution::Cyclic();
    else
        return nullptr;

    Interpolator<math::vec3>* interp = nullptr;
    xml_node<>* bez_node = pu_node->first_node("BezierInterpolator");
    if(bez_node)
    {
        std::vector<math::vec3> control_points;
        parse_bezier_interpolator(bez_node, control_points, chunk_index);
        interp = new BezierInterpolator(control_points);
    }

    if(interp)
        return new PositionUpdater(target, interp, tupd, stepScale, tMin, tMax);
    else
    {
        delete tupd;
        delete interp;
    }
    return nullptr;
}

bool SceneLoader::is_pos_relative(rapidxml::xml_node<>* parent)
{
    std::string ypos;
    if(xml::parse_attribute(parent, "ypos", ypos))
        return(!ypos.compare("relative"));
    return false;
}

uint32_t SceneLoader::get_num_controls(rapidxml::xml_node<>* cspline_node)
{
    uint32_t count = 0;
    for (xml_node<>* control=cspline_node->first_node("Control");
         control; control=control->next_sibling("Control"))
        ++count;
    return count;
}

}
