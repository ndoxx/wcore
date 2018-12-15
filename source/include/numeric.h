#ifndef NUMERIC_H
#define NUMERIC_H

#include <cstdint>
#include <functional>

namespace wcore::math
{

extern float integrate_simpson(std::function<float (float)> f, float lb, float ub, uint32_t subintervals=4);


} // namespace wcore::math

#endif // NUMERIC_H
