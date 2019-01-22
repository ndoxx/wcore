#ifndef CSPLINE_H
#define CSPLINE_H

#include <vector>
#include <array>
#include <cassert>
#include <cmath>

#ifdef __DEBUG__
#include <fstream>
#include "logger.h"
#endif //__DEBUG__

namespace wcore
{
namespace math
{
// Policy classes for tangent initialization of Hermite Splines
namespace CSplineTangentPolicy
{
template <typename T>
class Finite
{
public:
    static void initialize(std::vector<T>& tangents,
                           const std::vector<T>& points,
                           const std::vector<float>& domain)
    {
        for(uint32_t ii=1; ii<tangents.size()-1; ++ii)
        {
            tangents[ii] = 0.5*((points[ii+1]-points[ii]) / (domain[ii+1]-domain[ii])
                              + (points[ii]-points[ii-1]) / (domain[ii]-domain[ii-1]));
        }
    }
};

template <typename T>
class Cardinal
{
public:
    static float C_; // Tension parameter

    static void initialize(std::vector<T>& tangents,
                           const std::vector<T>& points,
                           const std::vector<float>& domain)
    {
        for(uint32_t ii=1; ii<tangents.size()-1; ++ii)
        {
            tangents[ii] = (1-C_) * (points[ii+1]-points[ii-1]) / (domain[ii+1]-domain[ii-1]);
        }
    }
};

template <typename T>
float Cardinal<T>::C_ = 0.0f;

template <typename T>
class CatmullRom
{
public:
    static void initialize(std::vector<T>& tangents,
                           const std::vector<T>& points,
                           const std::vector<float>& domain)
    {
        for(uint32_t ii=1; ii<tangents.size()-1; ++ii)
        {
            tangents[ii] = (points[ii+1]-points[ii-1]) * (1.f/(domain[ii+1]-domain[ii-1]));
        }
    }
};
}

// Hermite Cubic Spline Interpolator
template <typename T, typename Tg = CSplineTangentPolicy::CatmullRom<T>>
class CSpline
{
protected:
    uint32_t size_;                       // Number of control points
    std::vector<float> domain_;           // Loci of control points in parameter space
    std::vector<T> points_;               // Control points
    std::vector<T> tangents_;             // Local tangents at control points
    std::vector<std::array<T,4>> coeffs_; // Coefficients of the spline's power series expression

public:
    typedef T ValueType;

    CSpline(std::initializer_list<float> domain,
            std::initializer_list<T> points,
            std::array<T,2>&& end_tangents = {T(0),T(0)}):
    size_(points.size()),
    domain_(domain),
    points_(points),
    tangents_(size_,T(0)),
    coeffs_(size_-1)
    {
        // Arguments sanity check
        assert(size_>=2             && "[CSpline] At least 2 points must be specified.");
        assert(domain.size()==size_ && "[CSpline] Domain size must match number of points.");
        for(uint32_t ii=0; ii<size_-1; ++ii)
        {
            assert((domain_[ii+1]-domain_[ii])>0 && "[CSpline] Domain must be monotonically increasing.");
        }

        // Compute tangents then coefficients
        tangents_[0] = end_tangents[0];
        tangents_[size_-1] = end_tangents[1];
        Tg::initialize(tangents_, points_, domain_);
        compute_coefficients();
    }

    CSpline(const std::vector<float>& domain,
            const std::vector<T>& points,
            const std::array<T,2>& end_tangents = {T(0),T(0)}):
    size_(points.size()),
    domain_(domain),
    points_(points),
    tangents_(size_,T(0)),
    coeffs_(size_-1)
    {
        // Arguments sanity check
        assert(size_>=2             && "[CSpline] At least 2 points must be specified.");
        assert(domain.size()==size_ && "[CSpline] Domain size must match number of points.");
        for(uint32_t ii=0; ii<size_-1; ++ii)
        {
            assert((domain_[ii+1]-domain_[ii])>0 && "[CSpline] Domain must be monotonically increasing.");
        }

        // Compute tangents then coefficients
        tangents_[0] = end_tangents[0];
        tangents_[size_-1] = end_tangents[1];
        Tg::initialize(tangents_, points_, domain_);
        compute_coefficients();
    }

    // Lazy init
    CSpline(uint32_t size):
    size_(size),
    domain_(size),
    points_(size),
    tangents_(size_,T(0)),
    coeffs_(size_-1)
    {

    }
    void init(const std::vector<float>& domain,
              const std::vector<T>& points,
              const std::array<T,2>& end_tangents = {T(0),T(0)})
    {
        std::copy(domain.begin(), domain.end(), domain_.begin());
        std::copy(points.begin(), points.end(), points_.begin());

        // Sanity check
        assert(size_>=2             && "[CSpline] At least 2 points must be specified.");
        assert(domain.size()==size_ && "[CSpline] Domain size must match number of points.");
        for(uint32_t ii=0; ii<size_-1; ++ii)
        {
            assert((domain_[ii+1]-domain_[ii])>0 && "[CSpline] Domain must be monotonically increasing.");
        }
        // Compute tangents then coefficients
        tangents_[0] = end_tangents[0];
        tangents_[size_-1] = end_tangents[1];
        Tg::initialize(tangents_, points_, domain_);
        compute_coefficients();
    }

    // Return the evaluation of the power series at x
    T interpolate(float x) const
    {
        // Get index of polynomial interpolator given x's location in domain
        uint32_t index = get_index(x);
        // Translate from [x_k, x_{k+1}] to [0, x_{k+1}-x_k]
        // First part of the affine transformation that maps subdomain [x_k, x_{k+1}] to [0, 1]
        float t = x-domain_[index];

        // Compute interpolation as a power series
        T val(0);
        for(uint32_t ii=0; ii<4; ++ii)
        {
            val += coeffs_[index][ii] * pow(t,ii);
        }
        return val;
    }

#ifdef __DEBUG__
    void dbg_sample(const char* logfile, float nstep, float extrapolation_bias=0.0f)
    {
        float xx_min = domain_[0]-extrapolation_bias;
        float xx_max = domain_[size_-1]+extrapolation_bias;
        float step   = (xx_max-xx_min)/(nstep-1);

        std::string logfilePath("../logs/");
        logfilePath += logfile;
        std::ofstream logf(logfilePath.c_str());
        if(logf.is_open())
        {
            DLOGN("[CSpline] Sampling CSpline to log file:", "default", Severity::LOW);
            DLOGI(logfilePath, "default", Severity::LOW);
            for(float xx=xx_min; xx<xx_max+step/2; xx+=step)
            {
                logf << xx << " " << interpolate(xx) << std::endl;
            }
            logf.close();
        }
        else
        {
            DLOGW("[CSpline] Unable to open log file:", "default", Severity::WARN);
            DLOGI(logfilePath, "default", Severity::WARN);
        }
    }
#endif //__DEBUG__

private:
    // Calculate power series coefficients
    void compute_coefficients()
    {
        // For each subdomain
        for(uint32_t ii=0; ii<size_-1; ++ii)
        {
            // From factorization of subdomain interpolator in terms of powers of t:
            float deltax = domain_[ii+1] - domain_[ii];
            coeffs_[ii][0] = points_[ii];
            coeffs_[ii][1] = deltax*tangents_[ii];
            coeffs_[ii][2] = 3*(points_[ii+1]-points_[ii]) - deltax*(2*tangents_[ii]+tangents_[ii+1]);
            coeffs_[ii][3] = -2*(points_[ii+1]-points_[ii]) + deltax*(tangents_[ii]+tangents_[ii+1]);

            // Offput the part of the affine transformation that maps [0, x_{k+1}-x_k] to [0, 1] here
            for(uint32_t jj=0; jj<4; ++jj)
            {
                coeffs_[ii][jj] *= 1.f/pow(deltax, jj);
            }
        }
    }

    uint32_t get_index(float x) const
    {
        // Clamp index if x is out of bounds
        if(x<domain_[0])
            return 0;
        if(x>=domain_[size_-1])
            return size_-2;

        // Find immediate upper bound in domain and return index of lower bound
        for(uint32_t ii=1; ii<size_; ++ii)
        {
            if(x<domain_[ii])
                return ii-1;
        }
        return 0;
    }
};

    template <unsigned N, typename T> class vec;
    using vec3 = vec<3, float>;
    using CSplineCatmullV3  = CSpline<vec3, CSplineTangentPolicy::CatmullRom<vec3>>;
    using CSplineFiniteV3   = CSpline<vec3, CSplineTangentPolicy::Cardinal<vec3>>;
    using CSplineCardinalV3 = CSpline<vec3, CSplineTangentPolicy::Cardinal<vec3>>;
    using CSplineCatmullF   = CSpline<float, CSplineTangentPolicy::CatmullRom<float>>;
    using CSplineFiniteF    = CSpline<float, CSplineTangentPolicy::Finite<float>>;
    using CSplineCardinalF  = CSpline<float, CSplineTangentPolicy::Cardinal<float>>;

} // namsepace math
} // namespace wcore

#endif // CSPLINE_H
