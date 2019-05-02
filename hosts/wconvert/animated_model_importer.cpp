#include "animated_model_importer.h"
#include "logger.h"
#include "config.h"

using namespace wcore;

namespace wconvert
{

static inline wcore::math::mat4 to_mat4(const aiMatrix4x4& ai_mat)
{
    wcore::math::mat4 ret;
    for(int ii=0; ii<16; ++ii)
        ret[ii] = *ai_mat[ii];
    return ret;
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

    // Just care about the index 0 mesh
    if(n_meshes>1)
        DLOGW("Only single meshes allowed, ignoring all but index 0.", "wconvert");

    auto pmesh = pscene_->mMeshes[0];

    uint32_t n_bones_tot = pmesh->mNumBones;
    DLOGI("#bones= <v>" + std::to_string(n_bones_tot) + "</v>", "wconvert");

    // For each bone in mesh, register bone data
    for(uint32_t ii=0; ii<n_bones_tot; ++ii)
    {
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

        // For each weight in bone
        for(uint32_t jj=0; jj<n_weights; ++jj)
        {
            uint32_t vertex_id = pmesh->mBones[ii]->mWeights[jj].mVertexId;
            float weight = pmesh->mBones[ii]->mWeights[jj].mWeight;
            //bones_[vertex_id].add_bone_data(bone_index, weight);
        }
    }

    // * Read bone hierarchy
    // Skeleton root is the node named "Armature"
    auto* root_node = pscene_->mRootNode;
    auto* arm_node  = root_node->FindNode("Armature");

    if(arm_node)
    {
        // Armature transform
        model_info.root_transform = to_mat4(arm_node->mTransformation);

        // Read hierarchy recursively and construct skelettal tree
        auto* skel_root_node = arm_node->mChildren[0];
        read_bone_hierarchy(skel_root_node, model_info);
        return true;
    }
    else
    {
        DLOGE("Cannot find <h>Armature</h> node in scene hierarchy.", "wconvert");
        return false;
    }
}

void AnimatedModelImporter::reset()
{
    bone_map_.clear();
    bone_info_.clear();
    n_bones_ = 0;
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
    {
        // Get bone data
        node_offset = bone_info_[b_it->second].offset_matrix;
    }
    else
    {
        node_offset = to_mat4(pnode->mTransformation);
    }

    int parent_index = model_info.bone_hierarchy.add_node(BoneInfo( { node_name, node_offset } ));

    for(uint ii=0; ii<pnode->mNumChildren; ++ii)
    {
        int child_index = read_bone_hierarchy(pnode->mChildren[ii], model_info);
        model_info.bone_hierarchy.set_child(parent_index, child_index);
    }

    return parent_index;
}

} // namespace wconvert