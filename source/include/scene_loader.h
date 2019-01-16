#ifndef SCENE_LOADER_H
#define SCENE_LOADER_H

#include <vector>
#include <map>
#include <random>
#include <memory>

#include "xml_parser.h"
#include "math3d.h"
#include "game_system.h"

#ifdef __PROFILING_CHUNKS__
    #include "clock.hpp"
#endif

namespace wcore
{

class ModelFactory;
class Scene;
class Model;
class LineModel;
class Light;
class Transformation;
class Material;
struct Vertex3P3N3T2U;
struct Vertex3P;
template <typename VertexT> class Mesh;
class HeightMap;
class PositionUpdater;
class DaylightSystem;

typedef std::shared_ptr<Model> pModel;
typedef std::shared_ptr<Light> pLight;

class InputHandler;
namespace math
{
    class Bezier;
    template <typename T, typename TangentPolicy> class CSpline;
    namespace CSplineTangentPolicy
    {
        template <typename T> class CatmullRom;
        template <typename T> class Finite;
        template <typename T> class Cardinal;
    }
}

class SceneLoader: public GameSystem
{
private:
    XMLParser xml_parser_;
    ModelFactory* model_factory_;

    std::map<uint32_t, rapidxml::xml_node<>*> chunk_nodes_;
    std::map<uint32_t, rapidxml::xml_node<>*> chunk_patches_;

    uint32_t chunk_size_m_;
    uint32_t chunk_size_;
    float    lattice_scale_;
    float    texture_scale_;

    std::string current_map_;
    Scene* pscene_;
#ifdef __PROFILING_CHUNKS__
    nanoClock profile_clock_;
#endif

public:
    SceneLoader();
    ~SceneLoader();

    // Initialize event listener
    virtual void init_events(InputHandler& handler) override;

    void load_level(const char* level_name);
    inline void load_level(const std::string& level_name)
    {
        load_level(level_name.c_str());
    }

    void load_global(DaylightSystem& daylight);
    // Load a chunk, send geometry to driver if finalize set to true
    uint32_t load_chunk(const math::i32vec2& chunk_coords, bool finalize=true);

    void load_model_instance(hash_t name, uint32_t chunk_index);

    void reload_chunks();
    void reload_map();

    bool onKeyboardEvent(const WData& data);

    inline uint32_t get_chunk_size_meters() const { return chunk_size_m_; }
    inline uint32_t get_chunk_size() const        { return chunk_size_; }

    // HIGHER LEVEL PARSERS --------------------------------------------
    // Chunk related
    void parse_terrain(const math::i32vec2& chunk_coords);
    //void parse_line_models(rapidxml::xml_node<>* chunk_node, uint32_t chunk_index);
    void parse_models(rapidxml::xml_node<>* chunk_node, uint32_t chunk_index);
    void parse_model_batches(rapidxml::xml_node<>* chunk_node, uint32_t chunk_index);
    void parse_lights(rapidxml::xml_node<>* chunk_node, uint32_t chunk_index);

    // Global
    void parse_patches(rapidxml::xml_node<>* node);
    void parse_camera(rapidxml::xml_node<>* node);
    void parse_directional_light(rapidxml::xml_node<>* node);
    void parse_ambient(DaylightSystem& daylight, rapidxml::xml_node<>* node);
    // -----------------------------------------------------------------

private:
    // LOWER LEVEL PARSERS ---------------------------------------------
    void parse_transformation(rapidxml::xml_node<>* trn_node, Transformation& trans);
    void parse_transformation(rapidxml::xml_node<>* trn_node,
                              uint32_t n_instances,
                              std::vector<Transformation>& trans_vector,
                              std::mt19937& rng);
    Mesh<Vertex3P>* parse_line_mesh(rapidxml::xml_node<>* mesh_node);
    void parse_bezier_interpolator(rapidxml::xml_node<>* bez_node,
                                   std::vector<math::vec3>& controls,
                                   uint32_t chunk_index);
    void parse_motion(rapidxml::xml_node<>* mot_node, pModel target_model, uint32_t chunk_index);
    void parse_motion(rapidxml::xml_node<>* mot_node, pLight target_light, uint32_t chunk_index);
    PositionUpdater* parse_position_updater(rapidxml::xml_node<>* pu_node,
                                            math::vec3& target,
                                            uint32_t chunk_index);
    // Splines
    typedef math::CSpline<math::vec3, math::CSplineTangentPolicy::CatmullRom<math::vec3>> CSplineCatmullV3;
    typedef math::CSpline<float, math::CSplineTangentPolicy::CatmullRom<float>> CSplineCatmullF;
    typedef math::CSpline<float, math::CSplineTangentPolicy::Finite<float>> CSplineFiniteF;
    typedef math::CSpline<float, math::CSplineTangentPolicy::Cardinal<float>> CSplineCardinalF;
    template <typename CS> void parse_cspline(rapidxml::xml_node<>* cspline_node, CS& cspline);
    // -----------------------------------------------------------------


    // Helper functions
    void ground_model(std::shared_ptr<Model> pmdl, const HeightMap& hm);
    void ground_model(std::shared_ptr<Model> pmdl, uint32_t chunk_index);
    void ground_model(std::shared_ptr<LineModel> pmdl, uint32_t chunk_index);
    bool is_pos_relative(rapidxml::xml_node<>* parent);
    uint32_t get_num_controls(rapidxml::xml_node<>* cspline_node);
};

template <typename CS>
void SceneLoader::parse_cspline(rapidxml::xml_node<>* cspline_node, CS& cspline)
{
    std::vector<float> domain;
    std::vector<typename CS::ValueType> values;
    for (rapidxml::xml_node<>* control=cspline_node->first_node("Control");
         control; control=control->next_sibling("Control"))
    {
        float locus;
        typename CS::ValueType value;
        bool success = true;
        success &= xml::parse_attribute(control, "locus", locus);
        success &= xml::parse_attribute(control, "value", value);
        if(!success)
            continue;
        domain.push_back(locus);
        values.push_back(value);
    }
    cspline.init(domain, values);
}

}

#endif // SCENE_LOADER_H
