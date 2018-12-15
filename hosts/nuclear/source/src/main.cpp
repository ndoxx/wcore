#include <vector>
#include <cassert>
#include <iostream>
#include <cmath>

#include "gaussian.h"
#include "numeric.h"

using namespace wcore::math;

int main(int argc, char** argv)
{
    std::cout << "Nuclear test application" << std::endl;

    GaussianKernel gkernel(9, 1.8f);
    std::cout << gkernel << std::endl;

    return 0;
}
