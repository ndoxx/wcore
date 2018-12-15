#include <cmath>

#include "gaussian.h"
#include "numeric.h"

#define SIMPSON_SUBDIV 6

namespace wcore::math
{

float gaussian_distribution(float x, float mu, float sigma)
{
    float d = x - mu;
    float n = 1.0f / (sqrt(2.0f * M_PI) * sigma);
    return exp(-d*d/(2 * sigma * sigma)) * n;
};

GaussianKernel::GaussianKernel(uint32_t size, float sigma)
{
    update_kernel(size, sigma);
}

void GaussianKernel::update_kernel(uint32_t size, float sigma)
{
    // Sanitize
    assert(size%2==1 && "Gaussian kernel size must bu odd.");
    assert(sigma>0.0f && "Gaussian kernel sigma must be strictly positive.");

    // Initialize
    half_kernel_size_ = (size+1)/2;
    weights_.resize(half_kernel_size_);

    // Compute weights by numerical integration of distribution over each kernel tap
    float sum = 0.0f;
    for(int ii=0; ii<half_kernel_size_; ++ii)
    {
        float x_lb = ii - 0.5f;
        float x_ub = ii + 0.5f;
        weights_[ii] = integrate_simpson([&](float x){ return gaussian_distribution(x, 0.0f, sigma); }, x_lb, x_ub, SIMPSON_SUBDIV);
        sum += ((ii==0)?1.0f:2.0f) * weights_[ii];
    }

    // Renormalize weights to unit
    for(int ii=0; ii<half_kernel_size_; ++ii)
        weights_[ii] /= sum;
}

#ifdef __DEBUG__
std::ostream& operator<<(std::ostream& stream, const GaussianKernel& gk)
{
    stream << "[";
    for(int ii=0; ii<gk.half_kernel_size_; ++ii)
    {
        stream << gk[ii];
        if(ii!=gk.half_kernel_size_-1)
            stream << " ";
    }
    stream << "]";
    return stream;
}
#endif

} // namespace wcore::math
