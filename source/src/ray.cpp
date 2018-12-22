#include "ray.h"


namespace wcore
{

Ray Ray::to_model_space(const math::mat4& model_matrix) const
{
    Ray ray(*this);
    math::mat4 model_inv;
    math::inverse_affine(model_matrix, model_inv);
    ray.origin_w  = model_inv*ray.origin_w;
    ray.end_w     = model_inv*ray.end_w;
    ray.direction = (ray.end_w-ray.origin_w).normalized();
    return ray;
}


} // namespace wcore
