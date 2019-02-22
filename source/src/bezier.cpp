#include <cassert>
#include "bezier.h"
#include <iostream>

namespace wcore
{
namespace math
{

static constexpr const int factorial[] = {1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880, 3628800};
[[maybe_unused]] static constexpr const uint32_t nfact  = sizeof(factorial)/sizeof(int);

Bezier::Bezier():
control_(),
coeffs_(){}

Bezier::Bezier(const std::vector<vec3>& points):
control_(points),
coeffs_(){ compute_coefficients(); }

Bezier::Bezier(std::vector<vec3>&& points):
control_(std::move(points)),
coeffs_(){ compute_coefficients(); }

Bezier::Bezier(std::initializer_list<vec3> points):
control_(points),
coeffs_(){ compute_coefficients(); }


void Bezier::compute_coefficients()
{
    // Bezier curves have a polynomial form sum_{i=0}^{j} t^j C_j
    // This algorithm computes the C_j coefficients.
    // The advantage is that once computed, an evaluation (interpolation)
    // of the curve becomes a fast vector weighted sum.

    assert((control_.size() <= nfact) && "Factorials not defined this far.");

    coeffs_.resize(control_.size());
    float prod = 1.0f;
    for (int jj = 0; jj < control_.size(); ++jj)
    {
        if(jj>0)
            prod *= (order()-jj+1);

        vec3 sum(0.0f);
        for (int ii = 0; ii <= jj; ++ii)
            sum += control_[ii] * (((ii+jj)%2)?-1.0f:1.0f) / (factorial[ii]*factorial[jj-ii]);
                                  // == pow(-1.0f,ii+jj)
        coeffs_[jj] = prod * sum;
    }
}

vec3 Bezier::interpolate(float alpha)
{
    vec3 sum(0.0f);
    for (int ii = 0; ii < coeffs_.size(); ++ii)
    {
        sum += coeffs_[ii]*pow(alpha, ii);
    }
    return sum;
}

void Bezier::update_control_point(uint32_t index, const vec3& newvalue)
{
    assert(index<control_.size() && "Bezier::update_control_point()-> index>order.");
    control_[index] = newvalue;
    compute_coefficients();
}

void Bezier::update_control_point(uint32_t index, vec3&& newvalue)
{
    assert(index<control_.size() && "Bezier::update_control_point()-> index>order.");
    control_[index] = std::move(newvalue);
    compute_coefficients();
}

vec3 Bezier::deCasteljau(unsigned rr,
                         unsigned ii,
                         float    t,
                         const std::vector<vec3>& points)
{
    if(rr == 0) return points[ii];

    vec3 p1 = deCasteljau(rr - 1, ii,     t, points);
    vec3 p2 = deCasteljau(rr - 1, ii + 1, t, points);

    return p1*(1.0f-t) + p2*t;
}

} // namespace math
} // namespace wcore
