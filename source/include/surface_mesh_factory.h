#ifndef SURFACE_MESH_FACTORY_H
#define SURFACE_MESH_FACTORY_H
#include <filesystem>
#include <functional>
#include <map>
#include <list>
#include <cstdint>
#include <random>

#include "mesh.hpp"
#include "wtypes.h"
#include "mesh_descriptor.h"
#include "xml_parser.h"

namespace fs = std::filesystem;

namespace wcore
{

struct MeshInstanceDescriptor
{
    rapidxml::xml_node<char>* generator_node;
    hash_t type;
};

class SurfaceMeshFactory
{
public:
    typedef std::pair<hash_t, SurfaceMesh*> CacheEntryT;

    SurfaceMeshFactory();
    ~SurfaceMeshFactory();

    void retrieve_asset_descriptions(rapidxml::xml_node<>* meshes_node);

    SurfaceMesh* make_procedural(hash_t mesh_type,
                                 std::mt19937& rng,
                                 rapidxml::xml_node<char>* generator_node=nullptr,
                                 bool owns=true);
    SurfaceMesh* make_obj(const char* filename, bool process_uv=true, bool centered=true);
    SurfaceMesh* make_instance(hash_t name);

private:
    SurfaceMesh* procedural_cache_lookup(hash_t mesh_type,
                                         hash_t props,
                                         std::function<SurfaceMesh*(void)>new_mesh,
                                         bool owns=true);

    std::map<hash_t, MeshInstanceDescriptor> instance_descriptors_;
    std::map<hash_t, SurfaceMesh*> cache_; // Owns loaded meshes
    std::map<hash_t, SurfaceMesh*> proc_cache_; // Owns loaded procedural meshes
    fs::path models_path_;
};

} // namespace wcore

#endif // SURFACE_MESH_FACTORY_H
