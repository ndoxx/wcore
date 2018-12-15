#include "numeric.h"

namespace wcore::math
{

float integrate_simpson(std::function<float (float)> f, float lb, float ub, uint32_t subintervals)
{
    // * Simpson's rule integration is more accurate if we subdivide the interval of integration
    float x[subintervals+1],
          y[subintervals+1];
    float h = (ub-lb)/subintervals; //calculate width of subintervals

    //loop to evaluate x0,...xn and y0,...yn
    for(int ii=0; ii<subintervals+1; ++ii)
    {
        x[ii]=lb+ii*h;
        y[ii]=f(x[ii]);
    }

    //loop to evaluate (y1+y3+y5+...+yn-1)
    float sum4 = 0.0f;
    for(int ii=1; ii<subintervals; ii+=2)
        sum4 += y[ii];

    //loop to evaluate (y2+y4+y6+...+yn-2)
    float sum2 = 0.0f;
    for(int ii=2; ii<subintervals-1; ii+=2)
        sum2 += y[ii];

    //h/3*[y0+yn+4*(y1+y3+y5+...+yn-1)+2*(y2+y4+y6+...+yn-2)]
    return h/3.0f*(y[0] + y[subintervals] + 2.0f*sum2 + 4.0f*sum4);
}

} // namespace wcore::math
