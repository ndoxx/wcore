#ifndef MODEL_FACTORY_H
#define MODEL_FACTORY_H

#include <random>

#include "wtypes.h"
#include "xml_parser.h"
#include "terrain_common.h"

namespace wcore
{

class Model;
class TerrainChunk;
class SurfaceMeshFactory;
class MaterialFactory;
class TerrainFactory;
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
    std::shared_ptr<TerrainChunk> make_terrain_patch(const TerrainPatchDescriptor& desc,
                                                     OptRngT opt_rng=nullptr);
    // TMP
    inline SurfaceMeshFactory* mesh_factory()  { return mesh_factory_; }
    inline MaterialFactory* material_factory() { return material_factory_; }

private:
    XMLParser xml_parser_;

    SurfaceMeshFactory* mesh_factory_;
    MaterialFactory* material_factory_;
    TerrainFactory* terrain_factory_;
};


} // namespace wcore


#endif // MODEL_FACTORY_H
