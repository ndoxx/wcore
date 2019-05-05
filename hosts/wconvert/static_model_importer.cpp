#include "static_model_importer.h"
#include "assimp_utils.h"
#include "logger.h"
#include "config.h"

using namespace wcore;

namespace wconvert
{

StaticModelImporter::StaticModelImporter()
{
    wcore::CONFIG.get("root.folders.modelswork"_h, workdir_);
}

StaticModelImporter::~StaticModelImporter()
{

}

bool StaticModelImporter::load_model(const std::string& filename,
                                     StaticModelInfo& model_info)
{
    // Import file in an Assimp scene object
    pscene_ = importer_.ReadFile(workdir_ / filename,
                                 aiProcess_Triangulate |
                                 aiProcess_GenSmoothNormals |
                                 aiProcess_CalcTangentSpace |
                                 aiProcess_FlipUVs);

    // Sanity check
    if(!pscene_ || pscene_->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !pscene_->mRootNode)
    {
        DLOGE("Static model import error: ", "wconvert");
        DLOGI(importer_.GetErrorString(), "wconvert");

        return false;
    }

    // Save stripped file name as model name
    model_info.model_name = filename.substr(0, filename.find('.'));

    //auto global_inverse_transform = pscene_->mRootNode->mTransformation;
    //global_inverse_transform.Inverse();

    uint32_t n_meshes = pscene_->mNumMeshes;

    DLOGN("Loaded model :", "wconvert");
    DLOGI("<p>" + filename + "</p>", "wconvert");
    DLOGI("#meshes= <v>" + std::to_string(n_meshes) + "</v>", "wconvert");

    // * Read mesh data
    if(n_meshes==0)
    {
        DLOGE("No mesh detected, skipping model.", "wconvert");
        return false;
    }
    // Just care about the index 0 mesh
    if(n_meshes>1)
        DLOGW("Only single meshes allowed, ignoring all but index 0.", "wconvert");

    return read_mesh(pscene_->mMeshes[0], model_info);
}

bool StaticModelImporter::read_mesh(const aiMesh* pmesh,
                                    StaticModelInfo& model_info)
{
    // * Sanity check
    if(!pmesh->HasPositions())
    {
        DLOGE("Mesh lacks position data.", "wconvert");
        return false;
    }

    // * Register vertex data
    static const aiVector3D v3_zero(0.f, 0.f, 0.f);
    for(unsigned int ii=0; ii<pmesh->mNumVertices; ++ii)
    {
        const aiVector3D& p_pos       = pmesh->mVertices[ii];
        const aiVector3D& p_normal    = pmesh->HasNormals() ? pmesh->mNormals[ii] : v3_zero;
        const aiVector3D& p_tangent   = pmesh->HasTangentsAndBitangents() ? pmesh->mTangents[ii] : v3_zero;
        const aiVector3D& p_tex_coord = pmesh->HasTextureCoords(0) ? pmesh->mTextureCoords[0][ii] : v3_zero;

        model_info.vertex_data.vertices.push_back({to_vec3(p_pos),
                                                   to_vec3(p_normal),
                                                   to_vec3(p_tangent),
                                                   to_vec2(p_tex_coord)});
    }

    DLOGI("#vertices= <v>" + std::to_string(model_info.vertex_data.vertices.size()) + "</v>", "wconvert");

    // Indices
    for(unsigned int ii=0; ii<pmesh->mNumFaces; ++ii)
    {
        const aiFace& face = pmesh->mFaces[ii];
        assert(face.mNumIndices == 3); // Should be the case with aiProcess_Triangulate
        model_info.vertex_data.indices.push_back(face.mIndices[0]);
        model_info.vertex_data.indices.push_back(face.mIndices[1]);
        model_info.vertex_data.indices.push_back(face.mIndices[2]);
    }
    DLOGI("#indices= <v>" + std::to_string(model_info.vertex_data.indices.size()) + "</v>", "wconvert");

    return true;
}

} // namespace wconvert
