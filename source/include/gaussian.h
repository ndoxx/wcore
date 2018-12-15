#ifndef GAUSSIAN_H
#define GAUSSIAN_H

#include <cstdint>
#include <vector>
#include <cassert>
#include <ostream>

namespace wcore::math
{

extern float gaussian_distribution(float x, float mu, float sigma);

// Implements a separable Gaussian kernel, given a kernel size and a sigma
class GaussianKernel
{
public:
    GaussianKernel(uint32_t size, float sigma=1.0f);

    void update_kernel(uint32_t size, float sigma);

    inline float operator[](int index) const
    {
        assert(index<half_kernel_size_ && "[GaussianKernel] index out of bounds.");
        return weights_[index];
    }

#ifdef __DEBUG__
    friend std::ostream& operator<<(std::ostream& stream, const GaussianKernel& gk);
#endif

private:
    std::vector<float> weights_;
    uint32_t half_kernel_size_;
};

} // namespace wcore::math

#endif // GAUSSIAN_H
