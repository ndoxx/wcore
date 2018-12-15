#include "numeric.h"

namespace wcore::math
{

float integrate_simpson(std::function<float (float)> f, float lb, float ub, uint32_t subintervals)
{
    // * Simpson's rule is more accurate if we subdivide the interval of integration
    float h        = (ub-lb)/subintervals, // width of subintervals
          sum_odd  = 0.0f,                 // sum of odd subinterval contributions
          sum_even = 0.0f,                 // sum of even subinterval contributions
          y0       = f(lb),                // f value at lower bound
          yn       = f(ub);                // f value at upper bound

    // loop to evaluate intermediary sums
    for(int ii=1; ii<subintervals; ++ii)
    {
        float yy = f(lb + ii*h); // evaluate y_ii
        // sum of odd terms go into sum_odd and sum of even terms go into sum_even
        ((ii%2)?sum_odd:sum_even) += yy;
    }

    // h/3*[y0+yn+4*(y1+y3+y5+...+yn-1)+2*(y2+y4+y6+...+yn-2)]
    return h/3.0f*(y0 + yn + 4.0f*sum_odd + 2.0f*sum_even);
}

} // namespace wcore::math
