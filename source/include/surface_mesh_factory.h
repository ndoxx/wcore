#ifndef SURFACE_MESH_FACTORY_H
#define SURFACE_MESH_FACTORY_H
#include <filesystem>
#include <functional>
#include <map>
#include <list>
#include <cstdint>
#include <variant>
#include <random>

#include "mesh.hpp"
#include "wtypes.h"
#include "mesh_descriptor.h"
#include "xml_parser.h"

namespace fs = std::filesystem;

namespace wcore
{

struct SurfaceMeshDescriptor
{
    bool parse(rapidxml::xml_node<char>* mesh_node, fs::path models_path);

    rapidxml::xml_node<char>* generator_node;
    hash_t type;
    fs::path file_path;
    bool process_uv;
    bool process_normals;
    bool centered;
    int smooth_func;
};

class SurfaceMeshFactory
{
public:
    //typedef std::pair<hash_t, std::shared_ptr<SurfaceMesh>> CacheEntryT;
    typedef std::mt19937* OptRngT;

    SurfaceMeshFactory();
    ~SurfaceMeshFactory();

    // Parse XML Meshes node for mesh descriptions
    void retrieve_asset_descriptions(rapidxml::xml_node<>* meshes_node);
    // Create mesh procedurally
    std::shared_ptr<SurfaceMesh> make_procedural(hash_t mesh_type,
                                                 rapidxml::xml_node<char>* generator_node=nullptr,
                                                 OptRngT opt_rng=nullptr);
    // Create mesh from .obj file
    std::shared_ptr<SurfaceMesh> make_obj(const char* filename,
                                          bool process_uv=true,
                                          bool process_normals=false,
                                          bool centered=true,
                                          int smooth_func=0);
    // Create mesh from mesh instance name
    std::shared_ptr<SurfaceMesh> make_instance(hash_t name);
    // Create mesh from XML node and an optional random engine
    std::shared_ptr<SurfaceMesh> make_surface_mesh(rapidxml::xml_node<>* mesh_node,
                                                   bool& mesh_is_instance,
                                                   OptRngT opt_rng=nullptr);

private:
    std::map<hash_t, SurfaceMeshDescriptor> instance_descriptors_;
    std::map<hash_t, std::shared_ptr<SurfaceMesh>> cache_; // Owns loaded meshes
    std::map<hash_t, std::shared_ptr<SurfaceMesh>> proc_cache_; // Owns loaded procedural meshes
    fs::path models_path_;
};

} // namespace wcore

#endif // SURFACE_MESH_FACTORY_H
