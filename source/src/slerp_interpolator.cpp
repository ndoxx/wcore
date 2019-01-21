#include "slerp_interpolator.h"

namespace wcore
{

SlerpInterpolator::SlerpInterpolator(const std::vector<float>& domain,
                                     const std::vector<math::quat>& points):
domain_(domain),
points_(points)
{

}

math::quat SlerpInterpolator::interpolate(float t)
{
    // Find interval in domain that contains t
    int imax=0;
    for(int ii=0; ii<domain_.size();++ii)
    {
        if(domain_[ii]>t)
        {
            imax = ii;
            break;
        }
    }

    // Compute lerp coeff btw 0 and 1
    float t_max = domain_[imax];
    float t_min = domain_[imax-1];
    float alpha = (t-t_min)/(t_max-t_min);

    // Perform slerp
    const math::quat& q1(points_[imax-1]);
    const math::quat& q2(points_[imax]);

    return math::quat(math::slerp(q1,q2,alpha));
}


} // namespace wcore
