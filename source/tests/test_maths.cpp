#include <iostream>
#include "math3d.h"
#include "quaternion.h"

using namespace math;

int main(int argc, char const *argv[])
{
    std::cout << "====[TEST: MATHS]====" << std::endl;

    mat4 mlookat(0.0);
    init_look_at(mlookat, vec3(1.0, 0.0, 0.0),  // eye
                          vec3(0.0, 0.0, 1.0),  // target
                          vec3(0.0, 1.0, 0.0)); // up
    std::cout << "init_look_at(mlookat, eye=[1 0 0], target=[0 0 1], up=[0 1 0]) -> "
              << std::endl << mlookat << std::endl;
    mat4 mpersp(0.0);
    init_perspective(mpersp, 90.0, 4.0/3.0, 0.5, 10.0);
    std::cout << "init_perspective(mpersp, fov=90.0, ar=4.0/3.0, zn=0.5, zf=10.0) -> "
              << std::endl << mpersp << std::endl << std::endl;
    mat4 mortho(0.0);
    init_ortho(mortho, -0.5, 0.5, -0.5, 0.5, 0.5, 10.0);
    std::cout << "init_ortho(mortho, l=-0.5, r=0.5, b=-0.5, t=0.5, n=0.5, f=10.0) -> "
              << std::endl << mortho << std::endl << std::endl;

    return 0;
}
