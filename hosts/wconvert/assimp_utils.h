#ifndef ASSIMP_UTILS_H
#define ASSIMP_UTILS_H

#include <assimp/scene.h>
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

} // namespace wconvert

#endif
