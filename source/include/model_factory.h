#ifndef MODEL_FACTORY_H
#define MODEL_FACTORY_H

#include "wtypes.h"
#include "xml_parser.h"

namespace wcore
{

class SurfaceMeshFactory;
class MaterialFactory;
class ModelFactory
{
public:
    ModelFactory(const char* assetfile);
    ~ModelFactory();

    void parse_asset_file(const char* xmlfile);

    // TMP
    inline SurfaceMeshFactory* mesh_factory()  { return mesh_factory_; }
    inline MaterialFactory* material_factory() { return material_factory_; }

private:
    XMLParser xml_parser_;

    SurfaceMeshFactory* mesh_factory_;
    MaterialFactory* material_factory_;
};


} // namespace wcore


#endif // MODEL_FACTORY_H
