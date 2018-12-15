#include <vector>
#include <cassert>
#include <iostream>
#include <cmath>

#include "gaussian.h"
#include "numeric.h"

using namespace std::placeholders;
using namespace wcore::math;

float x_2(float x) { return x*x; }
float x_3(float x) { return x*x*x; }
float sine(float x) { return sin(x); }

int main(int argc, char** argv)
{
    std::cout << "Nuclear test application" << std::endl;

    GaussianKernel gkernel(9, 1.8f);
    std::cout << gkernel << std::endl;

    return 0;
}
