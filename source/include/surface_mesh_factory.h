#ifndef SURFACE_MESH_FACTORY_H
#define SURFACE_MESH_FACTORY_H
#include <filesystem>
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

    SurfaceMeshFactory(const char* xml_file);
    ~SurfaceMeshFactory();

    void retrieve_asset_descriptions();

    SurfaceMesh* make_procedural(hash_t mesh_type, std::mt19937& rng, rapidxml::xml_node<char>* generator_node=nullptr);
    SurfaceMesh* make_obj(const char* filename, bool process_uv=true, bool centered=true);
    SurfaceMesh* make_instance(hash_t name);

private:
    XMLParser xml_parser_;
    std::map<hash_t, MeshInstanceDescriptor> instance_descriptors_;
    std::map<hash_t, SurfaceMesh*> cache_; // Owns loaded meshes
    std::list<SurfaceMesh*> procedural_meshes_; // TMP Owns loaded procedural meshes
    fs::path models_path_;
};

} // namespace wcore

#endif // SURFACE_MESH_FACTORY_H
