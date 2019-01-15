#ifndef SURFACE_MESH_FACTORY_H
#define SURFACE_MESH_FACTORY_H
#include <filesystem>
#include <map>
#include <cstdint>
#include <random>

#include "mesh.hpp"
#include "wtypes.h"
#include "mesh_descriptor.h"

namespace fs = std::filesystem;

namespace wcore
{

class SurfaceMeshFactory
{
public:
    typedef std::pair<hash_t, SurfaceMesh*> CacheEntryT;

    SurfaceMeshFactory();
    ~SurfaceMeshFactory();

    SurfaceMesh* make_procedural(hash_t name, std::mt19937& rng, rapidxml::xml_node<char>* generator_node=nullptr);
    SurfaceMesh* make_obj(const char* filename, bool process_uv=true, bool centered=true);

private:
    std::map<hash_t, SurfaceMesh*> cache_; // Owns loaded meshes
    fs::path models_path_;
};

} // namespace wcore

#endif // SURFACE_MESH_FACTORY_H
