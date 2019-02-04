#ifndef MODEL_FACTORY_H
#define MODEL_FACTORY_H

#include <random>
#include <map>

#include "wtypes.h"
#include "xml_parser.h"
#include "terrain_common.h"

namespace wcore
{

struct ModelInstanceDescriptor
{
    hash_t mesh_name;
    hash_t material_name;
};

class Model;
class TerrainChunk;
class SurfaceMeshFactory;
class MaterialFactory;
class TerrainFactory;

struct Vertex3P3N3T2U;
template <typename VertexT> class Mesh;
using SurfaceMesh = Mesh<Vertex3P3N3T2U>;

class ModelFactory
{
public:
    typedef std::mt19937* OptRngT;

    ModelFactory(const char* assetfile);
    ~ModelFactory();

    void parse_asset_file(const char* xmlfile);

    std::shared_ptr<Model> make_model(rapidxml::xml_node<>* mesh_node,
                                      rapidxml::xml_node<>* mat_node,
                                      OptRngT opt_rng);
    std::shared_ptr<Model> make_model_instance(hash_t name);
    std::shared_ptr<TerrainChunk> make_terrain_patch(const TerrainPatchDescriptor& desc,
                                                     OptRngT opt_rng=nullptr);

    void retrieve_asset_descriptions(rapidxml::xml_node<>* models_node);

    // Preload mesh instance by model instance name
    std::shared_ptr<SurfaceMesh> preload_mesh_model_instance(hash_t name);
    // Preload mesh instance by name
    std::shared_ptr<SurfaceMesh> preload_mesh_instance(hash_t name);

private:
    XMLParser xml_parser_;

    SurfaceMeshFactory* mesh_factory_;
    MaterialFactory* material_factory_;
    TerrainFactory* terrain_factory_;

    std::map<hash_t, ModelInstanceDescriptor> instance_descriptors_;
};


} // namespace wcore


#endif // MODEL_FACTORY_H
