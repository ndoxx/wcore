#ifndef BEZIER_H
#define BEZIER_H

#include <vector>
#include <initializer_list>
#include <ostream>

#include "math3d.h"

namespace math{

class Bezier
{
protected:
    std::vector<vec3> control_; // Control points
    std::vector<vec3> coeffs_;  // Polynomial form coefficients

public:
    typedef vec3 ValueType;

    Bezier();

    template <typename... Args>
    Bezier(Args... points):
    control_{points...}
    {
        compute_coefficients();
    }

    Bezier(const std::vector<vec3>& points);
    Bezier(std::vector<vec3>&& points);
    Bezier(std::initializer_list<vec3> points);

    template <typename T>
    void set_control_points(std::initializer_list<T> il)
    {
        std::copy(il.begin(), il.end(), control_);
        compute_coefficients();
    }

    void update_control_point(uint32_t index, const vec3& newvalue);
    void update_control_point(uint32_t index, vec3&& newvalue);

    inline const vec3& get_front() { return control_.front(); }
    inline const vec3& get_back()  { return control_.back(); }
    inline const vec3& get_control_point(uint32_t index)
    {
        assert(index<control_.size() && "Bezier::get_control_point()-> index>order.");
        return control_[index];
    }
    inline const std::vector<vec3>& get_control_points() const { return control_; }
    inline uint32_t order() { return control_.size()-1; }

    // Fast interpolation using coefficients pre-computed during initialization
    vec3 interpolate(float alpha);


    // Stateless interpolation methods using deCasteljau's algorithm
    template <typename... Args>
    static vec3 interpolate(float alpha, Args&&... points);
    template <typename... Args>
    static vec3 interpolate(float alpha, const Args&... points);
    static inline vec3 interpolate(float alpha, const std::vector<vec3>& points);

private:
    void compute_coefficients();

    static vec3 deCasteljau(unsigned rr,   // Recursion level
                            unsigned ii,   // Free index
                            float    t,    // Interpolation parameter
                            const std::vector<vec3>& points);   // Control points
};

template <typename... Args>
vec3 Bezier::interpolate(float alpha, Args&&... points)
{
    std::vector<vec3> vec_points{std::forward<Args>(points)...};
    return deCasteljau(vec_points.size()-1, 0, alpha, vec_points);
}

template <typename... Args>
vec3 Bezier::interpolate(float alpha, const Args&... points)
{
    std::vector<vec3> vec_points{points...};
    return deCasteljau(vec_points.size()-1, 0, alpha, vec_points);
}

vec3 Bezier::interpolate(float alpha, const std::vector<vec3>& points)
{
    return deCasteljau(points.size()-1, 0, alpha, points);
}

}


#endif // BEZIER_H
