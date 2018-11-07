#include <random>
#include <limits>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <memory>
#include <vector>

#include "scene_loader.h"
#include "logger.h"
#include "scene.h"
#include "lights.h"
#include "colors.h"
#include "material.h"
#include "math3d.h"
#include "mesh_factory.h"
#include "obj_loader.h"
#include "height_map.h"
#include "heightmap_generator.h"
#include "tree_generator.h"
#include "rock_generator.h"
#include "camera.h"
#include "model.h"
#include "terrain_patch.h"
#include "texture.h"
#include "motion.hpp"
#include "bezier.h"
#include "daylight.h"
#include "input_handler.h"

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
chunk_size_m_(32),
lattice_scale_(1.0f),
texture_scale_(1.0f)
{

}

SceneLoader::~SceneLoader()
{

}

void SceneLoader::load_file_xml(const char* xml_file)
{
    DLOGN("[SceneLoader] Parsing xml scene description:");
    DLOGI("<p>" + std::string(xml_file) + "</p>");

    // Read the xml file into a vector
    std::ifstream scene_file(xml_file);
    buffer_ = std::vector<char>((std::istreambuf_iterator<char>(scene_file)), std::istreambuf_iterator<char>());
    buffer_.push_back('\0');

    // Parse the buffer using the xml file parsing library into DOM
    dom_.parse<0>(&buffer_[0]);

    // Find our root node
    root_ = dom_.first_node("Scene");
    if(!root_)
    {
        DLOGE("[SceneLoader] No Scene node.");
        return;
    }

    // Save terrain node
    terrain_ = root_->first_node("Terrain");
    if(!terrain_)
    {
        DLOGE("[SceneLoader] No Terrain node.");
        return;
    }

    current_map_ = xml_file;
}

void SceneLoader::setup_user_inputs(InputHandler& handler)
{
    handler.register_action(H_("k_reload_chunks"), [&]()
    {
        reload_chunks();
    });
    handler.register_action(H_("k_reload_map"), [&]()
    {
        reload_map();
    });
}


void SceneLoader::load_global(DaylightSystem& daylight)
{
    // Save chunk nodes
    for (xml_node<>* ck_node=root_->first_node("Chunk");
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

    // Parse terrain patches
    parse_patches(terrain_);
    // Parse camera information
    parse_camera(root_->first_node("Camera"));
    // Parse directional light
    parse_directional_light(root_->first_node("Light"));
    // Parse ambient parameters
    parse_ambient(daylight, root_->first_node("Ambient"));
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
        DLOGE("[SceneLoader] Ill-formed directional light declaration.");
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
        SCENE.add_directional_light(dirlight);
    }

    xml_node<>* ambient_node = root_->first_node("Ambient");
    if(!ambient_node) return;

    xml_node<>* dir_node = ambient_node->first_node("Directional");
    if(!dir_node) return;

    float shadow_bias = 0.1f;
    if(xml::parse_node(dir_node, "ShadowBias", shadow_bias))
        SCENE.set_shadow_bias(shadow_bias);
}

void SceneLoader::parse_patches(rapidxml::xml_node<>* node)
{
    // Get global terrain attributes
    xml::parse_attribute(terrain_, "chunkSize", chunk_size_m_);
    xml::parse_attribute(terrain_, "textureScale", texture_scale_);
    xml::parse_attribute(terrain_, "latticeScale", lattice_scale_);
    texture_scale_ *= lattice_scale_;
#ifdef __EXPERIMENTAL_TERRAIN_HEX_MESH__
    // Hex mesh terrain chunks must have an odd size
    if(chunk_size_m_%2==0)
    {
        ++chunk_size_m_;
        DLOGW("[SceneLoader] Chunk size must be <v>odd</v> for hex terrain meshes.");
        DLOGI("Corrected: Chunk Size is now " + std::to_string(chunk_size_m_));
    }
#else
    // Square mesh terrain chunks must have an even size
    if(chunk_size_m_%2)
    {
        ++chunk_size_m_;
        DLOGW("[SceneLoader] Chunk size must be <v>even</v> for square terrain meshes.");
        DLOGI("Corrected: Chunk Size is now " + std::to_string(chunk_size_m_));
    }
#endif
    chunk_size_ = uint32_t(floor(chunk_size_m_/lattice_scale_));
    SCENE.set_chunk_size_meters(chunk_size_m_);
    TerrainChunk::set_chunk_size(chunk_size_);

    // For each terrain patch
    for (xml_node<>* patch=terrain_->first_node("TerrainPatch");
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
            DLOGE("[SceneLoader] TerrainPatch node must define 'origin' and 'size' attributes.");
        }

        // For each chunk within that patch
        for(uint32_t xx=patchStart.x(); xx<patchStart.x()+patchSize.x(); ++xx)
        {
            for(uint32_t yy=patchStart.y(); yy<patchStart.y()+patchSize.y(); ++yy)
            {
                // Compute chunk index
                uint32_t chunk_index = std::hash<i32vec2>{}(i32vec2(xx,yy));
                // Add/Override reference to patch node in table
#ifdef __DEBUG_CHUNKS__
                auto it = chunk_patches_.find(chunk_index);
                if(it!=chunk_patches_.end())
                {
                    DLOGW("[SceneLoader] Silent chunk override:");
                    std::stringstream ss;
                    ss << "Chunk index= <n>" << chunk_index << "</n>, "
                       << "coordinates= <v>" << i32vec2(xx,yy) << "</v>";
                    DLOGI(ss.str());

                    ss.str("");
                    ss << "By patch of origin= <v>" << patchStart << "</v>, "
                       << "size= " << patchSize;
                    DLOGI(ss.str());
                }
#endif
                chunk_patches_[chunk_index] = patch;
            }
        }
    }
}

void SceneLoader::parse_ambient(DaylightSystem& daylight, rapidxml::xml_node<>* node)
{
    if(!node)
    {
        DLOGW("[SceneLoader] No Ambient node.");
        return;
    }
    xml_node<>* dir_node = node->first_node("Directional");
    if(!dir_node)
    {
        DLOGW("[SceneLoader] No Ambient.Directional node.");
        return;
    }
    xml_node<>* pp_node = node->first_node("PostProcessing");
    if(!pp_node)
    {
        DLOGW("[SceneLoader] No Ambient.PostProcessing node.");
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
        DLOGW("[SceneLoader] Ill-formed Ambient hierarchy.");
        return;
    }

    CSplineCatmullV3* color_interpolator            = new CSplineCatmullV3(get_num_controls(c_node->first_node("CSpline")));
    CSplineCatmullV3* pp_gamma_interpolator         = new CSplineCatmullV3(get_num_controls(g_node->first_node("CSpline")));;
    CSplineCatmullF*   brightness_interpolator      = new CSplineCatmullF(get_num_controls(b_node->first_node("CSpline")));;
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


uint32_t SceneLoader::load_chunk(const i32vec2& chunk_coords)
{
    // Compute chunk index, find corresponding node and load content
    uint32_t chunk_index = std::hash<i32vec2>{}(chunk_coords);
    auto it = chunk_nodes_.find(chunk_index);
    if(it == chunk_nodes_.end())
    {
#ifdef __DEBUG_CHUNKS__
        DLOGW("[SceneLoader] Ignoring non existing chunk:");
        std::stringstream ss;
        ss << "at coordinates <v>" << chunk_coords << "</v>";
        DLOGI(ss.str());
#endif
        return 0;
    }
    else
    {
#ifdef __DEBUG_CHUNKS__
        std::stringstream ss;
        ss << "[SceneLoader] Loading chunk: <n>" << chunk_index << "</n>"
           << " at <v>" << chunk_coords << "</v>";
        DLOGN(ss.str());
#endif
    }

#ifdef __PROFILING_CHUNKS__
    float dt_terrain_us,
          dt_models_us,
          dt_batches_us,
          dt_lights_us,
          dt_upload_us;
#endif

    SCENE.add_chunk(chunk_coords);
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
    parse_line_models(chunk_node, chunk_index);
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
    SCENE.load_geometry(chunk_index);
#ifdef __PROFILING_CHUNKS__
    period = profile_clock_.get_elapsed_time();
    dt_upload_us = 1e6*std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
#endif

#ifdef __PROFILING_CHUNKS__
    float dt_total_us = dt_terrain_us + dt_models_us + dt_batches_us + dt_lights_us + dt_upload_us;
    std::stringstream ss;
    ss << "Chunk loaded in <v>" << dt_total_us << "</v> µs total.";
    DLOGI(ss.str());

    ss.str("");
    ss << "Terrain: <v>" << dt_terrain_us << "</v> µs";
    DLOGI(ss.str());

    ss.str("");
    ss << "Models: <v>" << dt_models_us << "</v> µs";
    DLOGI(ss.str());

    ss.str("");
    ss << "Model Batches: <v>" << dt_batches_us << "</v> µs";
    DLOGI(ss.str());

    ss.str("");
    ss << "Lights: <v>" << dt_lights_us << "</v> µs";
    DLOGI(ss.str());

    ss.str("");
    ss << "Geometry upload: <v>" << dt_upload_us << "</v> µs";
    DLOGI(ss.str());
#endif
    return chunk_index;
}

void SceneLoader::reload_chunks()
{
    std::vector<i32vec2> coords;
    SCENE.get_loaded_chunks_coords(coords);

    SCENE.clear_chunks();
    for(uint32_t ii=0; ii<coords.size(); ++ii)
        load_chunk(coords[ii]);
}

void SceneLoader::reload_map()
{
    dom_.clear();
    buffer_.clear();

    load_file_xml(current_map_.c_str());
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
        DLOGE(ss.str());
        return;
    }
    xml_node<>* patch = it->second;

    // Get nodes
    xml_node<>* mat_node = patch->first_node("Material");
    if(!mat_node) return;
    xml_node<>* trn_node = patch->first_node("Transform");
    xml_node<>* aabb_node = patch->first_node("AABB");
    xml_node<>* shadow_node = patch->first_node("Shadow");
    xml_node<>* generator_node = patch->first_node("Generator");
    xml_node<>* height_modifier_node = patch->first_node("HeightModifier");

    // Get terrain patch attributes
    float height = 0.0f;
    xml::parse_attribute(patch, "height", height);

    // Generate material and height map
    Material* pmat = parse_material(mat_node);
#ifdef __EXPERIMENTAL_TERRAIN_HEX_MESH__
    // Add 1 to heightmap length for seamless terrain with hex terrain triangle mesh
    HeightMap* height_map = new HeightMap(chunk_size_, chunk_size_+1, height);
#else
    HeightMap* height_map = new HeightMap(chunk_size_, chunk_size_, height);
#endif
    height_map->set_scale(lattice_scale_);

    // TODO
    // Parse generators first hand (instantiate) then use
    // a table to select the correct generator for current chunk.
    // Hightmap generation
    bool success = true;
    if(generator_node)
    {
        std::string type;
        uint32_t seed = 0;
        success = true;
        success &= xml::parse_attribute(generator_node, "type", type);
        xml::parse_attribute(generator_node, "seed", seed);

        if(!success)
        {
            DLOGE("[SceneLoader] Generator node must have a 'type' attribute initialized.");
            return;
        }

        std::mt19937 rng;
        rng.seed(seed);
        if(!type.compare("simplex"))
        {
            SimplexNoiseProps props;
            props.scale = lattice_scale_;
            props.parse_xml(generator_node);
            props.hiBound /= lattice_scale_;
            // Compute starting coordinates (bottom-rightmost)
            props.startX = chunk_coords.x() * (chunk_size_-1);
            props.startZ = chunk_coords.y() * (chunk_size_-1);

            HeightmapGenerator::init_simplex_generator(rng);
            HeightmapGenerator::heightmap_from_simplex_noise(*height_map, props);
        }
    }

    // Apply modifiers to height map
    if(height_modifier_node)
    {
        for (xml_node<>* modifier=height_modifier_node->first_node();
             modifier; modifier=modifier->next_sibling())
        {
            if(!strcmp(modifier->name(),"Randomizer"))
            {
                uint32_t seed, xmin, xmax, ymin, ymax;
                float height_variance;
                success = true;
                success &= xml::parse_attribute(modifier, "seed", seed);
                success &= xml::parse_attribute(modifier, "xmin", xmin);
                success &= xml::parse_attribute(modifier, "xmax", xmax);
                success &= xml::parse_attribute(modifier, "ymin", ymin);
                success &= xml::parse_attribute(modifier, "ymax", ymax);
                success &= xml::parse_attribute(modifier, "variance", height_variance);
                if(!success) return;

                std::mt19937 rng;
                rng.seed(seed);
                std::uniform_real_distribution<float> var_distrib(-1.0f,1.0f);

                height_map->traverse([&](math::vec2 pos2d, float& height)
                {
                    if(pos2d.x()>=xmin && pos2d.x()<=xmax && pos2d.y()>=ymin && pos2d.y()<=ymax)
                        height += height_variance * var_distrib(rng);
                });
            }
            else if(!strcmp(modifier->name(),"Erosion"))
            {
                std::string type;
                if(!xml::parse_attribute(modifier, "type", type))
                    return;

                if(!type.compare("plateau"))
                {
                    PlateauErosionProps props;
                    props.parse_xml(modifier);
                    HeightmapGenerator::erode(*height_map, props);
                }
                else if(!type.compare("droplets"))
                {
                    DropletErosionProps props;
                    props.parse_xml(modifier);
                    HeightmapGenerator::erode_droplets(*height_map, props);
                }
            }
            else if(!strcmp(modifier->name(),"Offset"))
            {
                float offset;
                success = true;
                success &= xml::parse_attribute(modifier, "y", offset);
                offset /= lattice_scale_;
                if(!success) return;

                height_map->traverse([&](math::vec2 pos2d, float& height){ height += offset; });
            }
        }
    }

    // Make terrain chunk (model) from heightmap, material and scale params
    pTerrain terrain_ = std::make_shared<TerrainChunk>(
        height_map,
        pmat,
        lattice_scale_,
        texture_scale_
    );

    // Fix new terrain edge normals and tangents
    terrain::stitch_terrain_edges(terrain_, chunk_index, chunk_size_);

    // Spacial transformation
    Transformation trans;
    parse_transformation(trn_node, trans);
    // Translate according to chunk coordinates
    trans.translate((chunk_size_m_-lattice_scale_)*chunk_coords.x(),
                    0,
                    (chunk_size_m_-lattice_scale_)*chunk_coords.y());
    terrain_->set_transformation(trans);

    // AABB options
    if(aabb_node)
    {
        vec3 aabb_offset;
        if(xml::parse_node(aabb_node, "Offset", aabb_offset))
        {
            terrain_->set_AABB_offset(aabb_offset);
        }
    }
    else
    {
        auto&& dimensions = terrain_->get_mesh().get_dimensions();
        // By default -> for hex terrain meshes
        // Center bounding boxes on terrain chunk
        terrain_->set_OBB_offset(vec3(0.5f*(chunk_size_m_-1.0f), dimensions[3]*0.5f, 0.5f*(chunk_size_m_-0.5f)));
        terrain_->set_AABB_offset(vec3(0.25f*(chunk_size_m_-1.0f), dimensions[2]*0.5f, 0.25f*(chunk_size_m_-0.5f)));
    }

    // Shadow options
    if(shadow_node)
    {
        uint32_t cull_face=0;
        if(xml::parse_node(shadow_node, "CullFace", cull_face))
            terrain_->set_shadow_cull_face(cull_face);
    }

    terrain_->update_bounding_boxes();
    SCENE.add_terrain(terrain_, chunk_index);
}

void SceneLoader::parse_models(xml_node<>* chunk_node, uint32_t chunk_index)
{
    xml_node<>* mdls_node = chunk_node->first_node("Models");
    if(!mdls_node) return;

    std::mt19937 rng;
    for (xml_node<>* model=mdls_node->first_node("Model"); model; model=model->next_sibling("Model"))
    {
        // Get nodes
        xml_node<>* mat_node = model->first_node("Material");
        xml_node<>* mesh_node = model->first_node("Mesh");
        if(!mat_node || !mesh_node) continue;

        xml_node<>* trn_node = model->first_node("Transform");
        xml_node<>* mot_node = model->first_node("Motion");
        xml_node<>* aabb_node = model->first_node("AABB");
        xml_node<>* shadow_node = model->first_node("Shadow");

        // Do we position the models relative to a heightmap?
        bool relative_positioning = is_pos_relative(model);

        // Generate mesh and material, then construct model
        Material* pmat = parse_material(mat_node);
        SurfaceMesh* pmesh = parse_mesh(mesh_node, rng);
        if(!pmesh)
        {
            DLOGW("[SceneLoader] Skipping incomplete mesh declaration.");
            if(pmat)
                delete pmat;
            continue;
        }

        pModel pmdl = std::make_shared<Model>(pmesh, pmat);

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
                ground_model(pmdl, chunk_index);
            }

            // Translate according to chunk coordinates
            auto chunk_coords = SCENE.get_chunk_coordinates(chunk_index);
            pmdl->translate((chunk_size_m_-1)*chunk_coords.x(),
                            0,
                            (chunk_size_m_-1)*chunk_coords.y());
        }

        // AABB options
        if(aabb_node)
        {
            vec3 aabb_offset;
            if(xml::parse_node(aabb_node, "Offset", aabb_offset))
            {
                pmdl->set_AABB_offset(aabb_offset);
            }
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

        pmdl->update_bounding_boxes();
        SCENE.add_model(pmdl, chunk_index);
    }
}

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
        Material* pmat = parse_material(mat_node);
        Mesh<Vertex3P>* pmesh = parse_line_mesh(mesh_node);
        if(!pmesh)
        {
            DLOGW("[SceneLoader] Skipping incomplete mesh declaration.");
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
                ground_model(pmdl, chunk_index);
            }

            // Translate according to chunk coordinates
            auto chunk_coords = SCENE.get_chunk_coordinates(chunk_index);
            pmdl->translate((chunk_size_m_-1)*chunk_coords.x(),
                            0,
                            (chunk_size_m_-1)*chunk_coords.y());
        }

        SCENE.add_model(pmdl, chunk_index);
    }
}

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
        xml_node<>* aabb_node = batch->first_node("AABB");

        uint32_t instances, seed;
        xml::parse_attribute(batch, "instances", instances);
        xml::parse_attribute(batch, "seed", seed);

        // Do we position the models relative to a heightmap?
        bool relative_positioning = is_pos_relative(batch);

        std::mt19937 rng;
        rng.seed(seed);
        std::uniform_int_distribution<uint32_t> mesh_seed(0,std::numeric_limits<uint32_t>::max());
        std::uniform_real_distribution<float> var_distrib(-1.0f,1.0f);

        std::string color_space;
        std::string asset;
        vec3 color, color_var;
        float roughness;

        // Material
        bool use_asset = xml::parse_node(mat_node, "Asset", asset);
        xml::parse_node(mat_node, "Color", color);
        xml::parse_node(mat_node, "Roughness", roughness);
        xml::parse_attribute(mat_node->first_node("Color"), "variance", color_var);
        xml::parse_attribute(mat_node->first_node("Color"), "space", color_space);

        // AABB options
        vec3 aabb_offset(0.0f);
        if(aabb_node)
            xml::parse_node(aabb_node, "Offset", aabb_offset);

        // Generate batch transformations
        std::vector<Transformation> transforms;
        parse_transformation(trn_node, instances, transforms, rng);

        for(uint32_t ii=0; ii<instances; ++ii)
        {
            SurfaceMesh* pmesh = parse_mesh(mesh_node, rng);
            if(!pmesh)
            {
                DLOGW("[SceneLoader] Skipping incomplete mesh declaration.");
                continue;
            }

            pModel pmdl;
            if(use_asset)
                pmdl = std::make_shared<Model>(pmesh, asset.c_str());
            else
            {
                // Instance color
                vec3 inst_color = color + vec3(color_var.x() * var_distrib(rng),
                                               color_var.y() * var_distrib(rng),
                                               color_var.z() * var_distrib(rng));

                if(!color_space.compare("hsl"))
                    inst_color = color::hsl2rgb(inst_color);

                Material* pmat = new Material(inst_color, roughness);
                pmdl = std::make_shared<Model>(pmesh, pmat);
            }

            // Transform
            pmdl->set_transformation(transforms[ii]);

            // Is the y position specified relative to a height map?
            if(relative_positioning)
            {
                // If so, translate model using chunk height map.
                ground_model(pmdl, chunk_index);
            }
            // Translate according to chunk coordinates
            auto chunk_coords = SCENE.get_chunk_coordinates(chunk_index);
            pmdl->translate((chunk_size_m_-1)*chunk_coords.x(),
                            0,
                            (chunk_size_m_-1)*chunk_coords.y());

            if(shadow_node)
            {
                uint32_t cull_face=0;
                if(xml::parse_node(shadow_node, "CullFace", cull_face))
                    pmdl->set_shadow_cull_face(cull_face);
            }

            if(aabb_node)
            {
                pmdl->set_AABB_offset(aabb_offset);
            }

            pmdl->update_bounding_boxes();
            SCENE.add_model(pmdl, chunk_index);
        }
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
        if(is_pos_relative(light))
        {
            float height = SCENE.get_heightmap(chunk_index).get_height(position.xz());
            position[1] += height;
        }

        if(!light_type.compare("directional"))
        {
            std::shared_ptr<Light> dirlight(new DirectionalLight(position.normalized(),
                                                                 color,
                                                                 brightness));
            dirlight->set_ambient_strength(ambient_strength);
            SCENE.add_directional_light(dirlight);
        }
        else if(!light_type.compare("point"))
        {
            float radius;
            if(xml::parse_node(light, "Radius", radius))
            {
                // Translate according to chunk coordinates
                auto chunk_coords = SCENE.get_chunk_coordinates(chunk_index);
                vec3 offset((chunk_size_m_-1)*chunk_coords.x(),
                            0,
                            (chunk_size_m_-1)*chunk_coords.y());

                std::shared_ptr<Light> pointlight(new PointLight(position+offset,
                                                                 color,
                                                                 radius,
                                                                 brightness));
                pointlight->set_ambient_strength(ambient_strength);
                SCENE.add_light(pointlight, chunk_index);

                // Motion
                xml_node<>* mot_node = light->first_node("Motion");
                if(mot_node)
                {
                    parse_motion(mot_node, pointlight, chunk_index);
                }
            }
        }
    }
}

void SceneLoader::parse_camera(xml_node<>* node)
{
    if(!node) return;

    vec3 position;
    if(xml::parse_node(node, "Position", position))
        SCENE.get_camera()->set_position(position);

    vec2 orientation;
    if(xml::parse_node(node, "Orientation", orientation))
        SCENE.get_camera()->set_orientation(orientation.x(),orientation.y());
}

void SceneLoader::parse_transformation(xml_node<>* trn_node, Transformation& trans)
{
    xml_node<>* pos_node = trn_node->first_node("Position");
    if(pos_node)
    {
        vec3 position;
        xml::str_val(pos_node->value(), position);

        trans.set_position(position);
    }

    xml_node<>* ang_node = trn_node->first_node("Angle");
    if(ang_node)
    {
        vec3 angle;
        xml::str_val(ang_node->value(), angle);

        trans.rotate(angle.x(), angle.y(), angle.z());
    }

    xml_node<>* scl_node = trn_node->first_node("Scale");
    if(scl_node)
    {
        float scale;
        xml::str_val(scl_node->value(), scale);

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
        xml::str_val(pos_node->value(), position);
        pos_has_variance = xml::parse_attribute(pos_node, "variance", position_var);
    }
    if(ang_node)
    {
        xml::str_val(ang_node->value(), angle);
        ang_has_variance = xml::parse_attribute(ang_node, "variance", angle_var);
    }
    if(scl_node)
    {
        xml::str_val(scl_node->value(), scale);
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

Material* SceneLoader::parse_material(rapidxml::xml_node<>* mat_node)
{
    std::string asset;
    bool use_asset = xml::parse_node(mat_node, "Asset", asset);

    Material* pmat = nullptr;

    if(use_asset)
    {
        pmat = new Material(asset.c_str());
    }
    else
    {
        vec3 color;
        float roughness = 0.2f;
        float metallic = 0.0f;
        float alpha = 1.0f;
        xml::parse_node(mat_node, "Color", color);
        xml::parse_node(mat_node, "Roughness", roughness);
        xml::parse_node(mat_node, "Metallic", metallic);
        bool blend = xml::parse_node(mat_node, "Transparency", alpha);

        pmat = new Material(color, roughness, metallic, blend);
        if(blend)
            pmat->set_alpha(alpha);
    }

    float parallax_height_scale = 0.1f;
    bool use_normal_map = false;
    bool use_parallax_map = false;
    bool use_overlay = false;
    if(xml::parse_node(mat_node, "NormalMap", use_normal_map))
        pmat->set_normal_map(use_normal_map);
    if(xml::parse_node(mat_node, "ParallaxMap", use_parallax_map))
        pmat->set_parallax_map(use_parallax_map);
    if(xml::parse_node(mat_node, "ParallaxHeightScale", parallax_height_scale))
        pmat->set_parallax_height_scale(parallax_height_scale);
    if(xml::parse_node(mat_node, "Overlay", use_overlay))
        pmat->set_overlay(use_overlay);

    return pmat;
}

SurfaceMesh* SceneLoader::parse_mesh(rapidxml::xml_node<>* mesh_node, std::mt19937& rng)
{
    if(!mesh_node)
        return nullptr;

    std::string mesh;
    if(!xml::parse_attribute(mesh_node, "type", mesh))
        return nullptr;

    SurfaceMesh* pmesh = nullptr;
    if(!mesh.compare("cube"))
        pmesh = (SurfaceMesh*)factory::make_cube();
    else if(!mesh.compare("icosahedron"))
    {
        pmesh = (SurfaceMesh*)factory::make_icosahedron();
    }
    else if(!mesh.compare("icosphere"))
    {
        uint32_t density = 1;
        xml::parse_node(mesh_node, "Density", density);
        pmesh = (SurfaceMesh*)factory::make_ico_sphere(density);
    }
    else if(!mesh.compare("crystal"))
    {
        std::uniform_int_distribution<uint32_t> mesh_seed(0,std::numeric_limits<uint32_t>::max());
        pmesh = (SurfaceMesh*)factory::make_crystal(mesh_seed(rng));
    }
    else if(!mesh.compare("tentacle"))
    {
        CSplineCatmullV3 spline({0.0f, 0.33f, 0.66f, 1.0f},
                                {vec3(0,0,0),
                                 vec3(0.1,0.33,0.1),
                                 vec3(0.4,0.66,-0.1),
                                 vec3(-0.1,1.2,-0.5)});
        pmesh = (SurfaceMesh*)factory::make_tentacle(spline, 50, 25, 0.1, 0.3);
    }
    else if(!mesh.compare("tree"))
    {
        // Procedural tree mesh, look for TreeGenerator node
        xml_node<>* tg_node = mesh_node->first_node("TreeGenerator");
        if(!tg_node)
            return nullptr;

        TreeProps props;
        props.parse_xml(tg_node);

        pmesh = TreeGenerator::generate_tree(props);
    }
    else if(!mesh.compare("rock"))
    {
        // Procedural rock mesh, look for RockGenerator node
        xml_node<>* rg_node = mesh_node->first_node("RockGenerator");
        if(!rg_node)
            return nullptr;

        RockProps props;
        props.parse_xml(rg_node);

        std::uniform_int_distribution<uint32_t> mesh_seed(0,std::numeric_limits<uint32_t>::max());
        props.seed = mesh_seed(rng);

        pmesh = RockGenerator::generate_rock(props);
    }
    else if(!mesh.compare("obj"))
    {
        // Acquire mesh from Wavefront .obj file
        std::string location;
        if(!xml::parse_node(mesh_node, "Location", location))
        {
            DLOGW("[SceneLoader] Ignoring incomplete .obj mesh declaration.");
            DLOGI("Missing <n>Location</n> node.");
            return nullptr;
        }
        bool process_uv = false;
        xml::parse_node(mesh_node, "ProcessUV", process_uv);
        pmesh = LOADOBJ("../res/models/teapot.obj", process_uv);
    }
    else
    {
        DLOGW("Unknown mesh name: ");
        DLOGI(mesh);
    }
    return pmesh;
}

Mesh<Vertex3P>* SceneLoader::parse_line_mesh(rapidxml::xml_node<>* mesh_node)
{
    if(!mesh_node)
        return nullptr;

    std::string mesh;
    if(!xml::str_val(mesh_node->value(), mesh))
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
}

void SceneLoader::ground_model(std::shared_ptr<Model> pmdl, const HeightMap& hm)
{
    // Find height at model (x,z) position and apply offset to model
    float height = hm.get_height(pmdl->get_position().xz());
    pmdl->translate_y(height);
}

void SceneLoader::ground_model(std::shared_ptr<Model> pmdl, uint32_t chunk_index)
{
    // Find height at model (x,z) position and apply offset to model
    float height = SCENE.get_heightmap(chunk_index).get_height(pmdl->get_position().xz());
    pmdl->translate_y(height);
}

void SceneLoader::ground_model(std::shared_ptr<LineModel> pmdl, uint32_t chunk_index)
{
    // Find height at model (x,z) position and apply offset to model
    float height = SCENE.get_heightmap(chunk_index).get_height(pmdl->get_position().xz());
    pmdl->translate_y(height);
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
        if(xml::str_val(control->value(), control_point))
        {
            if(relative_positioning)
            {
                float height = SCENE.get_heightmap(chunk_index).get_height(control_point.xz());
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
            SCENE.add_position_updater(updater, chunk_index);
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
            SCENE.add_rotator(rotator, chunk_index);
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
            SCENE.add_position_updater(updater, chunk_index);
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
