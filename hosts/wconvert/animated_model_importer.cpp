#include "animated_model_importer.h"
#include "logger.h"
#include "config.h"

using namespace wcore;

namespace wconvert
{

static inline wcore::math::mat4 to_mat4(const aiMatrix4x4& ai_mat)
{
    wcore::math::mat4 ret;
    for(int ii=0; ii<4; ++ii)
        for(int jj=0; jj<4; ++jj)
            ret[4*ii+jj] = ai_mat[jj][ii];
    return ret;
}

static inline wcore::math::vec3 to_vec3(const aiVector3D& ai_vec3)
{
    return wcore::math::vec3(ai_vec3.x, ai_vec3.y, ai_vec3.z);
}

static inline wcore::math::vec2 to_vec2(const aiVector3D& ai_vec3)
{
    return wcore::math::vec2(ai_vec3.x, ai_vec3.y);
}

AnimatedModelImporter::AnimatedModelImporter():
n_bones_(0)
{
    wcore::CONFIG.get("root.folders.modelswork"_h, workdir_);
}

AnimatedModelImporter::~AnimatedModelImporter()
{

}

bool AnimatedModelImporter::load_model(const std::string& filename, ModelInfo& model_info)
{
    // Clear intermediate data from previous import
    reset();

    // Import file in an Assimp scene object
    pscene_ = importer_.ReadFile(workdir_ / filename,
                                 aiProcess_Triangulate |
                                 aiProcess_GenSmoothNormals |
                                 aiProcess_CalcTangentSpace |
                                 aiProcess_FlipUVs);

    // Sanity check
    if(!pscene_ || pscene_->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !pscene_->mRootNode)
    {
        DLOGE("Animated model import error: ", "wconvert");
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

    if(!read_mesh(pscene_->mMeshes[0], model_info))
        return false;

    // * Read bone hierarchy
    // Skeleton root is the node named "Armature"
    auto* arm_node  = pscene_->mRootNode->FindNode("Armature");

    if(arm_node)
    {
        // Armature transform
        model_info.root_transform = to_mat4(arm_node->mTransformation);

        // Read hierarchy recursively and construct skelettal tree
        auto* skel_root_node = arm_node->mChildren[0];
        read_bone_hierarchy(skel_root_node, model_info);
    }
    else
    {
        DLOGE("Cannot find <h>Armature</h> node in scene hierarchy.", "wconvert");
        return false;
    }

    // * Read animation data

    // TODO

    return true;
}

void AnimatedModelImporter::reset()
{
    bone_map_.clear();
    bone_info_.clear();
    n_bones_ = 0;
}

bool AnimatedModelImporter::read_mesh(const aiMesh* pmesh, ModelInfo& model_info)
{
    // * Sanity check
    if(!pmesh->HasBones() || !pmesh->HasPositions())
    {
        DLOGE("Mesh lacks bones or position data.", "wconvert");
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

    // * Register bone-related per-vertex data
    uint32_t n_bones_tot = pmesh->mNumBones;
    DLOGI("#bones= <v>" + std::to_string(n_bones_tot) + "</v>", "wconvert");

    // Vector of indices to keep track of the location of per-vertex bone data
    std::vector<uint32_t> weight_pos(model_info.vertex_data.indices.size(), 0);

    // For each bone in mesh
    for(uint32_t ii=0; ii<n_bones_tot; ++ii)
    {
        // Register bone data
        uint32_t bone_index = 0;
        std::string bone_name(pmesh->mBones[ii]->mName.data);

        DLOGN("Detected bone: <n>" + bone_name + "</n>", "wconvert");

        auto b_it = bone_map_.find(bone_name);
        if(b_it == bone_map_.end())
        {
            bone_index = n_bones_++;
            bone_map_[bone_name] = bone_index;

            // Offset matrix
            math::mat4 offset_matrix = to_mat4(pmesh->mBones[ii]->mOffsetMatrix);
            bone_info_.push_back(BoneInfo( { bone_name, offset_matrix } ));

            DLOGN("New bone: <n>" + bone_name + "</n>", "wconvert");
        }
        else
            bone_index = b_it->second;

        DLOGI("Index= <v>" + std::to_string(bone_index) + "</v>", "wconvert");

        uint32_t n_weights = pmesh->mBones[ii]->mNumWeights;
        DLOGI("#weights= <v>" + std::to_string(n_weights) + "</v>", "wconvert");

        // Register weight and bone id in mesh vertices
        for(uint32_t jj=0; jj<n_weights; ++jj)
        {
            const auto& weight_data = pmesh->mBones[ii]->mWeights[jj];
            uint32_t vertex_id = weight_data.mVertexId;
            float weight = weight_data.mWeight;

            auto& vertex = model_info.vertex_data.vertices[vertex_id];
            uint32_t pos = weight_pos[vertex_id]++;
            vertex.weight_[pos] = weight;
            vertex.bone_id_[pos] = bone_index;
        }
    }

    return true;
}

int AnimatedModelImporter::read_bone_hierarchy(const aiNode* pnode,
                                               ModelInfo& model_info)
{
    std::string node_name(pnode->mName.data);
    math::mat4 node_offset;
    node_offset.init_identity();

    // Find node name in bone map
    auto b_it = bone_map_.find(node_name);
    if(b_it != bone_map_.end())
        node_offset = bone_info_[b_it->second].offset_matrix;
    else
        node_offset = to_mat4(pnode->mTransformation);

    int parent_index = model_info.bone_hierarchy.add_node(BoneInfo( { node_name, node_offset } ));

    for(uint ii=0; ii<pnode->mNumChildren; ++ii)
    {
        int child_index = read_bone_hierarchy(pnode->mChildren[ii], model_info);
        model_info.bone_hierarchy.set_child(parent_index, child_index);
    }

    return parent_index;
}

} // namespace wconvert
