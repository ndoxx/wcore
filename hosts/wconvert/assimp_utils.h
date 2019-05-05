#ifndef ASSIMP_UTILS_H
#define ASSIMP_UTILS_H

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "math3d.h"

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

// Post processing flags used by assimp importer
// Reference: http://sir-kimmi.de/assimp/lib_html/postprocess_8h.html
static const int IMPORTER_POST_PROCESS_FLAGS = aiProcess_Triangulate            // Convert n-gons to triangles
                                             | aiProcess_FindDegenerates        // Detect degenerate faces, next flag will ensure they are removed and not simply collapsed
                                             | aiProcess_SortByPType            // Split meshes with different primitive types into submeshes. With previous flag, will remove degenerates.
                                             | aiProcess_FindInvalidData        // Remove/fix zeroed normals / uvs
                                             | aiProcess_OptimizeMeshes         // Reduce the number of input meshes
                                             | aiProcess_ImproveCacheLocality   // Reorder triangles so as to minimize average post-transform vertex cache miss ratio
                                             | aiProcess_ValidateDataStructure  // Validates indices, bones and animations
                                             | aiProcess_RemoveComponent        // Remove parts of input data structure, such as vertex color, to allow for efficient vertex joining
                                             | aiProcess_JoinIdenticalVertices  // Allow vertices to be shared by several faces
                                             | aiProcess_GenSmoothNormals       // Generate smoothed normals if normals aren't present in input data
                                             | aiProcess_CalcTangentSpace       // Generate tangents and bi-tangents
                                             /*| aiProcess_FlipUVs*/;           // Flip UVs vertically and adjust bi-tangents

static const int IMPORTER_REMOVE_COMPONENTS = aiComponent_CAMERAS
                                            | aiComponent_LIGHTS
                                            | aiComponent_COLORS;

static const int IMPORTER_IGNORE_PRIMITIVES = aiPrimitiveType_POINT
                                            | aiPrimitiveType_LINE;

} // namespace wconvert

#endif
