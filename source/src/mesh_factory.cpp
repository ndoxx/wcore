#include "mesh_factory.h"
#include "surface_mesh.h"
#include "math3d.h"
#include "height_map.h"
#include "cspline.h"
#include "vertex_format.h"

#include <random>
#include <unordered_map>

namespace wcore
{

using namespace math;

namespace factory
{

std::shared_ptr<FaceMesh> make_cube(bool finalize)
{
    std::shared_ptr<FaceMesh> pmesh(new FaceMesh);
    //                  /--------POSITION------  /----------UV----------
    //Front 1          |                        |
    pmesh->push_vertex({vec3( 0.5f, 0.0f, 0.5f), vec3(0), vec3(0), vec2(1.0f/3.0f, 1.0f)});
    pmesh->push_vertex({vec3( 0.5f, 1.0f, 0.5f), vec3(0), vec3(0), vec2(1.0f/3.0f, 0.5f)});
    pmesh->push_vertex({vec3(-0.5f, 1.0f, 0.5f), vec3(0), vec3(0), vec2(0.0f, 0.5f)});
    pmesh->push_vertex({vec3(-0.5f, 0.0f, 0.5f), vec3(0), vec3(0), vec2(0.0f, 1.0f)});

    //Right 2
    pmesh->push_vertex({vec3( 0.5f, 0.0f,-0.5f), vec3(0), vec3(0), vec2(2.0f/3.0f, 1.0f)});
    pmesh->push_vertex({vec3( 0.5f, 1.0f,-0.5f), vec3(0), vec3(0), vec2(2.0f/3.0f, 0.5f)});
    pmesh->push_vertex({vec3( 0.5f, 1.0f, 0.5f), vec3(0), vec3(0), vec2(1.0f/3.0f, 0.5f)});
    pmesh->push_vertex({vec3( 0.5f, 0.0f, 0.5f), vec3(0), vec3(0), vec2(1.0f/3.0f, 1.0f)});

    //Back 4
    pmesh->push_vertex({vec3(-0.5f, 0.0f,-0.5f), vec3(0), vec3(0), vec2(1.0f/3.0f, 0.5f)});
    pmesh->push_vertex({vec3(-0.5f, 1.0f,-0.5f), vec3(0), vec3(0), vec2(1.0f/3.0f, 0.0f)});
    pmesh->push_vertex({vec3( 0.5f, 1.0f,-0.5f), vec3(0), vec3(0), vec2(0.0f, 0.0f)});
    pmesh->push_vertex({vec3( 0.5f, 0.0f,-0.5f), vec3(0), vec3(0), vec2(0.0f, 0.5f)});

    //Left 5
    pmesh->push_vertex({vec3(-0.5f, 0.0f, 0.5f), vec3(0), vec3(0), vec2(2.0f/3.0f, 0.5f)});
    pmesh->push_vertex({vec3(-0.5f, 1.0f, 0.5f), vec3(0), vec3(0), vec2(2.0f/3.0f, 0.0f)});
    pmesh->push_vertex({vec3(-0.5f, 1.0f,-0.5f), vec3(0), vec3(0), vec2(1.0f/3.0f, 0.0f)});
    pmesh->push_vertex({vec3(-0.5f, 0.0f,-0.5f), vec3(0), vec3(0), vec2(1.0f/3.0f, 0.5f)});

    //Top 3
    pmesh->push_vertex({vec3( 0.5f, 1.0f, 0.5f), vec3(0), vec3(0), vec2(1.0f,1.0f)});
    pmesh->push_vertex({vec3( 0.5f, 1.0f,-0.5f), vec3(0), vec3(0), vec2(1.0f,0.5f)});
    pmesh->push_vertex({vec3(-0.5f, 1.0f,-0.5f), vec3(0), vec3(0), vec2(2.0f/3.0f, 0.5f)});
    pmesh->push_vertex({vec3(-0.5f, 1.0f, 0.5f), vec3(0), vec3(0), vec2(2.0f/3.0f, 1.0f)});

    //Bottom 6
    pmesh->push_vertex({vec3( 0.5f, 0.0f,-0.5f), vec3(0), vec3(0), vec2(1.0f,0.5f)});
    pmesh->push_vertex({vec3( 0.5f, 0.0f, 0.5f), vec3(0), vec3(0), vec2(1.0f,0.0f)});
    pmesh->push_vertex({vec3(-0.5f, 0.0f, 0.5f), vec3(0), vec3(0), vec2(2.0f/3.0f, 0.0f)});
    pmesh->push_vertex({vec3(-0.5f, 0.0f,-0.5f), vec3(0), vec3(0), vec2(2.0f/3.0f, 0.5f)});

    pmesh->push_triangle(0,  1,  2);
    pmesh->push_triangle(0,  2,  3);
    pmesh->push_triangle(4,  5,  6);
    pmesh->push_triangle(4,  6,  7);
    pmesh->push_triangle(8,  9,  10);
    pmesh->push_triangle(8,  10, 11);
    pmesh->push_triangle(12, 13, 14);
    pmesh->push_triangle(12, 14, 15);
    pmesh->push_triangle(16, 17, 18);
    pmesh->push_triangle(16, 18, 19);
    pmesh->push_triangle(20, 21, 22);
    pmesh->push_triangle(20, 22, 23);

    if(finalize)
    {
        pmesh->build_normals_and_tangents();
        pmesh->compute_dimensions();
    }

    return pmesh;
}

std::shared_ptr<FaceMesh> make_cube_uniface(bool finalize)
{
    std::shared_ptr<FaceMesh> pmesh(new FaceMesh);
    //                  /--------POSITION------  /----------UV----------
    //Front 1          |                        |
    //Front 1          |                        |
    pmesh->push_vertex({vec3( 0.5f, -0.5f, 0.5f), vec3(0), vec3(0), vec2(1.0f, 1.0f)});
    pmesh->push_vertex({vec3( 0.5f,  0.5f, 0.5f), vec3(0), vec3(0), vec2(1.0f, 0.0f)});
    pmesh->push_vertex({vec3(-0.5f,  0.5f, 0.5f), vec3(0), vec3(0), vec2(0.0f, 0.0f)});
    pmesh->push_vertex({vec3(-0.5f, -0.5f, 0.5f), vec3(0), vec3(0), vec2(0.0f, 1.0f)});

    //Right 2
    pmesh->push_vertex({vec3( 0.5f, -0.5f,-0.5f), vec3(0), vec3(0), vec2(1.0f, 0.0f)});
    pmesh->push_vertex({vec3( 0.5f,  0.5f,-0.5f), vec3(0), vec3(0), vec2(1.0f, 1.0f)});
    pmesh->push_vertex({vec3( 0.5f,  0.5f, 0.5f), vec3(0), vec3(0), vec2(0.0f, 1.0f)});
    pmesh->push_vertex({vec3( 0.5f, -0.5f, 0.5f), vec3(0), vec3(0), vec2(0.0f, 0.0f)});

    //Back 4
    pmesh->push_vertex({vec3(-0.5f, -0.5f,-0.5f), vec3(0), vec3(0), vec2(1.0f, 1.0f)});
    pmesh->push_vertex({vec3(-0.5f,  0.5f,-0.5f), vec3(0), vec3(0), vec2(1.0f, 0.0f)});
    pmesh->push_vertex({vec3( 0.5f,  0.5f,-0.5f), vec3(0), vec3(0), vec2(0.0f, 0.0f)});
    pmesh->push_vertex({vec3( 0.5f, -0.5f,-0.5f), vec3(0), vec3(0), vec2(0.0f, 1.0f)});

    //Left 5
    pmesh->push_vertex({vec3(-0.5f, -0.5f, 0.5f), vec3(0), vec3(0), vec2(1.0f, 0.0f)});
    pmesh->push_vertex({vec3(-0.5f,  0.5f, 0.5f), vec3(0), vec3(0), vec2(1.0f, 1.0f)});
    pmesh->push_vertex({vec3(-0.5f,  0.5f,-0.5f), vec3(0), vec3(0), vec2(0.0f, 1.0f)});
    pmesh->push_vertex({vec3(-0.5f, -0.5f,-0.5f), vec3(0), vec3(0), vec2(0.0f, 0.0f)});

    //Top 3
    pmesh->push_vertex({vec3( 0.5f,  0.5f, 0.5f), vec3(0), vec3(0), vec2(1.0f, 1.0f)});
    pmesh->push_vertex({vec3( 0.5f,  0.5f,-0.5f), vec3(0), vec3(0), vec2(1.0f, 0.0f)});
    pmesh->push_vertex({vec3(-0.5f,  0.5f,-0.5f), vec3(0), vec3(0), vec2(0.0f, 0.0f)});
    pmesh->push_vertex({vec3(-0.5f,  0.5f, 0.5f), vec3(0), vec3(0), vec2(0.0f, 1.0f)});

    //Bottom 6
    pmesh->push_vertex({vec3( 0.5f, -0.5f,-0.5f), vec3(0), vec3(0), vec2(1.0f, 1.0f)});
    pmesh->push_vertex({vec3( 0.5f, -0.5f, 0.5f), vec3(0), vec3(0), vec2(1.0f, 0.0f)});
    pmesh->push_vertex({vec3(-0.5f, -0.5f, 0.5f), vec3(0), vec3(0), vec2(0.0f, 0.0f)});
    pmesh->push_vertex({vec3(-0.5f, -0.5f,-0.5f), vec3(0), vec3(0), vec2(0.0f, 1.0f)});

    pmesh->push_triangle(0,  1,  2);
    pmesh->push_triangle(0,  2,  3);
    pmesh->push_triangle(4,  5,  6);
    pmesh->push_triangle(4,  6,  7);
    pmesh->push_triangle(8,  9,  10);
    pmesh->push_triangle(8,  10, 11);
    pmesh->push_triangle(12, 13, 14);
    pmesh->push_triangle(12, 14, 15);
    pmesh->push_triangle(16, 17, 18);
    pmesh->push_triangle(16, 18, 19);
    pmesh->push_triangle(20, 21, 22);
    pmesh->push_triangle(20, 22, 23);

    if(finalize)
    {
        pmesh->build_normals_and_tangents();
        pmesh->compute_dimensions();
    }

    return pmesh;
}

std::shared_ptr<FaceMesh> make_plane(bool finalize)
{
    std::shared_ptr<FaceMesh> pmesh(new FaceMesh);

    pmesh->push_vertex({vec3( 0.5f,  0.f,  0.5f), vec3(0), vec3(0), vec2(1.0f, 1.0f)});
    pmesh->push_vertex({vec3( 0.5f,  0.f, -0.5f), vec3(0), vec3(0), vec2(1.0f, 0.0f)});
    pmesh->push_vertex({vec3(-0.5f,  0.f, -0.5f), vec3(0), vec3(0), vec2(0.0f, 0.0f)});
    pmesh->push_vertex({vec3(-0.5f,  0.f,  0.5f), vec3(0), vec3(0), vec2(0.0f, 1.0f)});

    pmesh->push_triangle(0,  1,  2);
    pmesh->push_triangle(0,  2,  3);

    if(finalize)
    {
        pmesh->build_normals_and_tangents();
        pmesh->compute_dimensions();
    }

    return pmesh;
}

std::shared_ptr<FaceMesh> make_box(const math::extent_t& extent, float texture_scale)
{
    std::shared_ptr<FaceMesh> pmesh(new FaceMesh);

    //Front 1
    pmesh->push_vertex({vec3( extent[1], extent[2], extent[5]), vec3(0), vec3(0), texture_scale*vec2(1.0f, 1.0f)});
    pmesh->push_vertex({vec3( extent[1], extent[3], extent[5]), vec3(0), vec3(0), texture_scale*vec2(1.0f, 0.0f)});
    pmesh->push_vertex({vec3( extent[0], extent[3], extent[5]), vec3(0), vec3(0), texture_scale*vec2(0.0f, 0.0f)});
    pmesh->push_vertex({vec3( extent[0], extent[2], extent[5]), vec3(0), vec3(0), texture_scale*vec2(0.0f, 1.0f)});

    //Right 2
    pmesh->push_vertex({vec3( extent[1], extent[2], extent[4]), vec3(0), vec3(0), texture_scale*vec2(1.0f, 0.0f)});
    pmesh->push_vertex({vec3( extent[1], extent[3], extent[4]), vec3(0), vec3(0), texture_scale*vec2(1.0f, 1.0f)});
    pmesh->push_vertex({vec3( extent[1], extent[3], extent[5]), vec3(0), vec3(0), texture_scale*vec2(0.0f, 1.0f)});
    pmesh->push_vertex({vec3( extent[1], extent[2], extent[5]), vec3(0), vec3(0), texture_scale*vec2(0.0f, 0.0f)});

    //Back 4
    pmesh->push_vertex({vec3( extent[0], extent[2], extent[4]), vec3(0), vec3(0), texture_scale*vec2(1.0f, 1.0f)});
    pmesh->push_vertex({vec3( extent[0], extent[3], extent[4]), vec3(0), vec3(0), texture_scale*vec2(1.0f, 0.0f)});
    pmesh->push_vertex({vec3( extent[1], extent[3], extent[4]), vec3(0), vec3(0), texture_scale*vec2(0.0f, 0.0f)});
    pmesh->push_vertex({vec3( extent[1], extent[2], extent[4]), vec3(0), vec3(0), texture_scale*vec2(0.0f, 1.0f)});

    //Left 5
    pmesh->push_vertex({vec3( extent[0], extent[2], extent[5]), vec3(0), vec3(0), texture_scale*vec2(1.0f, 0.0f)});
    pmesh->push_vertex({vec3( extent[0], extent[3], extent[5]), vec3(0), vec3(0), texture_scale*vec2(1.0f, 1.0f)});
    pmesh->push_vertex({vec3( extent[0], extent[3], extent[4]), vec3(0), vec3(0), texture_scale*vec2(0.0f, 1.0f)});
    pmesh->push_vertex({vec3( extent[0], extent[2], extent[4]), vec3(0), vec3(0), texture_scale*vec2(0.0f, 0.0f)});

    //Top 3
    pmesh->push_vertex({vec3( extent[1], extent[3], extent[5]), vec3(0), vec3(0), texture_scale*vec2(1.0f, 1.0f)});
    pmesh->push_vertex({vec3( extent[1], extent[3], extent[4]), vec3(0), vec3(0), texture_scale*vec2(1.0f, 0.0f)});
    pmesh->push_vertex({vec3( extent[0], extent[3], extent[4]), vec3(0), vec3(0), texture_scale*vec2(0.0f, 0.0f)});
    pmesh->push_vertex({vec3( extent[0], extent[3], extent[5]), vec3(0), vec3(0), texture_scale*vec2(0.0f, 1.0f)});

    //Bottom 6
    pmesh->push_vertex({vec3( extent[1], extent[2], extent[4]), vec3(0), vec3(0), texture_scale*vec2(1.0f, 1.0f)});
    pmesh->push_vertex({vec3( extent[1], extent[2], extent[5]), vec3(0), vec3(0), texture_scale*vec2(1.0f, 0.0f)});
    pmesh->push_vertex({vec3( extent[0], extent[2], extent[5]), vec3(0), vec3(0), texture_scale*vec2(0.0f, 0.0f)});
    pmesh->push_vertex({vec3( extent[0], extent[2], extent[4]), vec3(0), vec3(0), texture_scale*vec2(0.0f, 1.0f)});

    pmesh->push_triangle(0,  1,  2);
    pmesh->push_triangle(0,  2,  3);
    pmesh->push_triangle(4,  5,  6);
    pmesh->push_triangle(4,  6,  7);
    pmesh->push_triangle(8,  9,  10);
    pmesh->push_triangle(8,  10, 11);
    pmesh->push_triangle(12, 13, 14);
    pmesh->push_triangle(12, 14, 15);
    pmesh->push_triangle(16, 17, 18);
    pmesh->push_triangle(16, 18, 19);
    pmesh->push_triangle(20, 21, 22);
    pmesh->push_triangle(20, 22, 23);

    pmesh->build_normals_and_tangents();
    pmesh->compute_dimensions();

    return pmesh;
}

std::shared_ptr<MeshP> make_skybox_3P()
{
    std::shared_ptr<MeshP> pmesh(new MeshP);

    pmesh->_push_vertex({vec3(-1.0f,  1.0f, -1.0f)}); // 0
    pmesh->_push_vertex({vec3(-1.0f, -1.0f, -1.0f)}); // 1
    pmesh->_push_vertex({vec3( 1.0f, -1.0f, -1.0f)}); // 2
    pmesh->_push_vertex({vec3( 1.0f,  1.0f, -1.0f)}); // 3
    pmesh->_push_vertex({vec3(-1.0f, -1.0f,  1.0f)}); // 4
    pmesh->_push_vertex({vec3(-1.0f,  1.0f,  1.0f)}); // 5
    pmesh->_push_vertex({vec3( 1.0f, -1.0f,  1.0f)}); // 6
    pmesh->_push_vertex({vec3( 1.0f,  1.0f,  1.0f)}); // 7

    pmesh->_push_triangle(0, 1, 2);
    pmesh->_push_triangle(2, 3, 0);

    pmesh->_push_triangle(4, 1, 0);
    pmesh->_push_triangle(0, 5, 4);

    pmesh->_push_triangle(2, 6, 7);
    pmesh->_push_triangle(7, 3, 2);

    pmesh->_push_triangle(4, 5, 7);
    pmesh->_push_triangle(7, 6, 4);

    pmesh->_push_triangle(0, 3, 7);
    pmesh->_push_triangle(7, 5, 0);

    pmesh->_push_triangle(1, 4, 2);
    pmesh->_push_triangle(2, 4, 6);

    return pmesh;
}

MeshP* make_cube_3P()
{
    MeshP* pmesh = new MeshP;
    // BOTTOM
    pmesh->_push_vertex({vec3( 0.5f,  -0.5f, 0.5f)});  // 0
    pmesh->_push_vertex({vec3( 0.5f,  -0.5f, -0.5f)}); // 1
    pmesh->_push_vertex({vec3( -0.5f, -0.5f, -0.5f)}); // 2
    pmesh->_push_vertex({vec3( -0.5f, -0.5f, 0.5f)});  // 3
    // TOP
    pmesh->_push_vertex({vec3( 0.5f,  0.5f, 0.5f)});   // 4
    pmesh->_push_vertex({vec3( 0.5f,  0.5f, -0.5f)});  // 5
    pmesh->_push_vertex({vec3( -0.5f, 0.5f, -0.5f)});  // 6
    pmesh->_push_vertex({vec3( -0.5f, 0.5f, 0.5f)});   // 7

    pmesh->push_line(0, 1);
    pmesh->push_line(0, 3);
    pmesh->push_line(0, 4);
    pmesh->push_line(6, 5);
    pmesh->push_line(6, 2);
    pmesh->push_line(6, 7);
    pmesh->push_line(4, 5);
    pmesh->push_line(4, 7);
    pmesh->push_line(2, 1);
    pmesh->push_line(2, 3);
    pmesh->push_line(7, 3);
    pmesh->push_line(5, 1);

    pmesh->set_centered(true);

    return pmesh;
}

MeshPC* make_cube_3P_3C()
{
    MeshPC* pmesh = new MeshPC;
    // BOTTOM
    pmesh->_push_vertex({vec3( 0.5f,  -0.5f, 0.5f), vec3(1.0f)});   // 0
    pmesh->_push_vertex({vec3( 0.5f,  -0.5f, -0.5f), vec3(1.0f)});  // 1
    pmesh->_push_vertex({vec3( -0.5f, -0.5f, -0.5f), vec3(1.0f)}); // 2
    pmesh->_push_vertex({vec3( -0.5f, -0.5f, 0.5f), vec3(1.0f)});  // 3
    // TOP
    pmesh->_push_vertex({vec3( 0.5f,  0.5f, 0.5f), vec3(1.0f)});   // 4
    pmesh->_push_vertex({vec3( 0.5f,  0.5f, -0.5f), vec3(1.0f)});  // 5
    pmesh->_push_vertex({vec3( -0.5f, 0.5f, -0.5f), vec3(1.0f)});  // 6
    pmesh->_push_vertex({vec3( -0.5f, 0.5f, 0.5f), vec3(1.0f)});   // 7

    pmesh->push_line(0, 1);
    pmesh->push_line(0, 3);
    pmesh->push_line(0, 4);
    pmesh->push_line(6, 5);
    pmesh->push_line(6, 2);
    pmesh->push_line(6, 7);
    pmesh->push_line(4, 5);
    pmesh->push_line(4, 7);
    pmesh->push_line(2, 1);
    pmesh->push_line(2, 3);
    pmesh->push_line(7, 3);
    pmesh->push_line(5, 1);

    pmesh->set_centered(true);

    return pmesh;
}

MeshP* make_cube_NDC_3P()
{
    MeshP* pmesh = new MeshP;
    float uu = 0.5f;
    // BOTTOM
    pmesh->_push_vertex({vec3( uu, -uu, uu)});   // 0
    pmesh->_push_vertex({vec3( uu, -uu, -uu)});  // 1
    pmesh->_push_vertex({vec3( -uu, -uu, -uu)}); // 2
    pmesh->_push_vertex({vec3( -uu, -uu, uu)});  // 3
    // TOP
    pmesh->_push_vertex({vec3( uu,  uu, uu)});   // 4
    pmesh->_push_vertex({vec3( uu,  uu, -uu)});  // 5
    pmesh->_push_vertex({vec3( -uu, uu, -uu)});  // 6
    pmesh->_push_vertex({vec3( -uu, uu, uu)});   // 7

    pmesh->push_line(0, 1);
    pmesh->push_line(0, 3);
    pmesh->push_line(0, 4);
    pmesh->push_line(6, 5);
    pmesh->push_line(6, 2);
    pmesh->push_line(6, 7);
    pmesh->push_line(4, 5);
    pmesh->push_line(4, 7);
    pmesh->push_line(2, 1);
    pmesh->push_line(2, 3);
    pmesh->push_line(7, 3);
    pmesh->push_line(5, 1);

    pmesh->set_centered(true);

    return pmesh;
}

// Create a unit sphere centered around (0,0,0)
MeshP* make_uv_sphere_3P(uint32_t nRings,
                         uint32_t nRingPoints,
                         bool lines)
{
    MeshP* pmesh = new MeshP;

    // Angle increments
    float deltaTheta = M_PI/nRings;
    float deltaPhi   = 2.0f * M_PI/nRingPoints;

    float theta = 0.0f;
    float phi   = 0.0f;

    for(uint32_t ring=0; ring<nRings; ++ring)
    {
        theta += deltaTheta;
        for(uint32_t point=0; point<nRingPoints; ++point)
        {
            phi += deltaPhi;
            pmesh->_push_vertex({vec3(sin(theta) * cos(phi),
                                      cos(theta),
                                      sin(theta) * sin(phi))});
        }
    }
    // South pole
    pmesh->_push_vertex({vec3(0,-1,0)});
    // North pole
    pmesh->_push_vertex({vec3(0,1,0)});

    uint32_t npoints = nRingPoints*nRings + 2;

    // Triangles
    // Top cap
    for(uint32_t pp=0; pp<nRingPoints; ++pp)
    {
        if(lines)
        {
            pmesh->push_line(npoints-1, (pp+1)%nRingPoints);
            pmesh->push_line((pp+1)%nRingPoints, pp);
            pmesh->push_line(pp, npoints-1);
        }
        else
        {
            pmesh->_push_triangle((pp+1)%nRingPoints, pp, npoints-1);
        }
    }
    for(uint32_t rr=0; rr<nRings-2; ++rr)
    {
        for(uint32_t pp=0; pp<nRingPoints; ++pp)
        {
            if(lines)
            {
                pmesh->push_line(pp%nRingPoints + rr * nRingPoints,
                                 (pp+1)%nRingPoints + rr * nRingPoints);
                pmesh->push_line((pp+1)%nRingPoints + rr * nRingPoints,
                                 (pp+1)%nRingPoints + (rr+1) * nRingPoints);
                pmesh->push_line((pp+1)%nRingPoints + (rr+1) * nRingPoints,
                                 pp%nRingPoints + rr * nRingPoints);
                pmesh->push_line(pp%nRingPoints + rr * nRingPoints,
                                 pp%nRingPoints + (rr+1) * nRingPoints);
                pmesh->push_line(pp%nRingPoints + (rr+1) * nRingPoints,
                                 (pp+1)%nRingPoints + (rr+1) * nRingPoints);
            }
            else
            {
                pmesh->_push_triangle((pp+1)%nRingPoints + (rr+1) * nRingPoints,
                                      pp%nRingPoints + rr * nRingPoints,
                                     (pp+1)%nRingPoints + rr * nRingPoints);
                pmesh->_push_triangle(pp%nRingPoints + (rr+1) * nRingPoints,
                                     pp%nRingPoints + rr * nRingPoints,
                                     (pp+1)%nRingPoints + (rr+1) * nRingPoints);
            }
        }
    }
    // Bottom cap
    uint32_t base = nRingPoints*(nRings-2);
    for(uint32_t pp=0; pp<nRingPoints; ++pp)
    {
        if(lines)
        {
            pmesh->push_line(base + pp,
                             base + (pp+1)%nRingPoints);
            pmesh->push_line(base + (pp+1)%nRingPoints,
                             npoints-2);
            pmesh->push_line(npoints-2,
                             base + pp);
        }
        else
        {
            pmesh->_push_triangle(base + (pp+1)%nRingPoints, npoints-2, base + pp);
        }
    }

    pmesh->set_centered(true);

    return pmesh;
}

std::shared_ptr<FaceMesh> make_uv_sphere(uint32_t nRings,
                                         uint32_t nRingPoints)
{
    std::shared_ptr<FaceMesh> pmesh(new FaceMesh);

    // Angle increments
    float deltaTheta = M_PI/nRings;
    float deltaPhi   = 2.0f * M_PI/nRingPoints;

    float theta = 0.0f;
    float phi   = 0.0f;

    for(uint32_t ring=0; ring<nRings; ++ring)
    {
        theta += deltaTheta;
        for(uint32_t point=0; point<nRingPoints; ++point)
        {
            phi += deltaPhi;
            pmesh->push_vertex({vec3(sin(theta) * cos(phi),
                                     cos(theta),
                                     sin(theta) * sin(phi)),
                                vec3(0),vec3(0),vec2(0)});
        }
    }
    // South pole
    pmesh->push_vertex({vec3(0.f,-1.f,0.f),vec3(0.f),vec3(0.f),vec2(0.f)});
    // North pole
    pmesh->push_vertex({vec3(0.f,1.f,0.f),vec3(0.f),vec3(0.f),vec2(0.f)});

    uint32_t npoints = nRingPoints*nRings + 2;

    // Triangles
    // Top cap
    for(uint32_t pp=0; pp<nRingPoints; ++pp)
    {

        pmesh->push_triangle((pp+1)%nRingPoints, pp, npoints-1);
    }
    for(uint32_t rr=0; rr<nRings-2; ++rr)
    {
        for(uint32_t pp=0; pp<nRingPoints; ++pp)
        {
            pmesh->push_triangle((pp+1)%nRingPoints + (rr+1) * nRingPoints,
                                  pp%nRingPoints + rr * nRingPoints,
                                 (pp+1)%nRingPoints + rr * nRingPoints);
            pmesh->push_triangle(pp%nRingPoints + (rr+1) * nRingPoints,
                                 pp%nRingPoints + rr * nRingPoints,
                                 (pp+1)%nRingPoints + (rr+1) * nRingPoints);
        }
    }
    // Bottom cap
    uint32_t base = nRingPoints*(nRings-2);
    for(uint32_t pp=0; pp<nRingPoints; ++pp)
    {
        pmesh->push_triangle(base + (pp+1)%nRingPoints, npoints-2, base + pp);
    }

    pmesh->set_centered(true);

    pmesh->build_normals_and_tangents();
    pmesh->compute_dimensions();
    return pmesh;
}

// Constants to avoid having to renormalize positions of icosahedron vertices each call
static const float PHI = (1.0f + sqrt(5.0f)) / 2.0f;
static const float ONE_N = 1.0f/(sqrt(2.0f+PHI)); // norm of any icosahedron vertex position
static const float PHI_N = PHI*ONE_N;

std::shared_ptr<TriangularMesh> make_icosahedron(bool finalize)
{
    std::shared_ptr<TriangularMesh> pmesh(new TriangularMesh);

    pmesh->push_vertex({vec3(-ONE_N,  PHI_N,  0.f), vec3(0.f), vec3(0.f), vec2(0.f)});
    pmesh->push_vertex({vec3( ONE_N,  PHI_N,  0.f), vec3(0.f), vec3(0.f), vec2(0.f)});
    pmesh->push_vertex({vec3(-ONE_N, -PHI_N,  0.f), vec3(0.f), vec3(0.f), vec2(0.f)});
    pmesh->push_vertex({vec3( ONE_N, -PHI_N,  0.f), vec3(0.f), vec3(0.f), vec2(0.f)});

    pmesh->push_vertex({vec3( 0.f, -ONE_N,  PHI_N), vec3(0.f), vec3(0.f), vec2(0.f)});
    pmesh->push_vertex({vec3( 0.f,  ONE_N,  PHI_N), vec3(0.f), vec3(0.f), vec2(0.f)});
    pmesh->push_vertex({vec3( 0.f, -ONE_N, -PHI_N), vec3(0.f), vec3(0.f), vec2(0.f)});
    pmesh->push_vertex({vec3( 0.f,  ONE_N, -PHI_N), vec3(0.f), vec3(0.f), vec2(0.f)});

    pmesh->push_vertex({vec3( PHI_N,  0.f, -ONE_N), vec3(0.f), vec3(0.f), vec2(0.f)});
    pmesh->push_vertex({vec3( PHI_N,  0.f,  ONE_N), vec3(0.f), vec3(0.f), vec2(0.f)});
    pmesh->push_vertex({vec3(-PHI_N,  0.f, -ONE_N), vec3(0.f), vec3(0.f), vec2(0.f)});
    pmesh->push_vertex({vec3(-PHI_N,  0.f,  ONE_N), vec3(0.f), vec3(0.f), vec2(0.f)});

    pmesh->push_triangle(0, 11, 5);
    pmesh->push_triangle(0, 5, 1);
    pmesh->push_triangle(0, 1, 7);
    pmesh->push_triangle(0, 7, 10);
    pmesh->push_triangle(0, 10, 11);

    pmesh->push_triangle(1, 5, 9);
    pmesh->push_triangle(5, 11, 4);
    pmesh->push_triangle(11, 10, 2);
    pmesh->push_triangle(10, 7, 6);
    pmesh->push_triangle(7, 1, 8);

    pmesh->push_triangle(3, 9, 4);
    pmesh->push_triangle(3, 4, 2);
    pmesh->push_triangle(3, 2, 6);
    pmesh->push_triangle(3, 6, 8);
    pmesh->push_triangle(3, 8, 9);

    pmesh->push_triangle(4, 9, 5);
    pmesh->push_triangle(2, 4, 11);
    pmesh->push_triangle(6, 2, 10);
    pmesh->push_triangle(8, 6, 7);
    pmesh->push_triangle(9, 8, 1);

    pmesh->set_centered(true);

    if(!finalize)
    {
        pmesh->build_normals_and_tangents();
        pmesh->compute_dimensions();
    }

    return pmesh;
}

MeshP* make_icosahedron_3P()
{
    MeshP* pmesh = new MeshP;

    pmesh->_push_vertex({vec3(-ONE_N,  PHI_N,  0.f)});
    pmesh->_push_vertex({vec3( ONE_N,  PHI_N,  0.f)});
    pmesh->_push_vertex({vec3(-ONE_N, -PHI_N,  0.f)});
    pmesh->_push_vertex({vec3( ONE_N, -PHI_N,  0.f)});

    pmesh->_push_vertex({vec3( 0.f, -ONE_N,  PHI_N)});
    pmesh->_push_vertex({vec3( 0.f,  ONE_N,  PHI_N)});
    pmesh->_push_vertex({vec3( 0.f, -ONE_N, -PHI_N)});
    pmesh->_push_vertex({vec3( 0.f,  ONE_N, -PHI_N)});

    pmesh->_push_vertex({vec3( PHI_N,  0.f, -ONE_N)});
    pmesh->_push_vertex({vec3( PHI_N,  0.f,  ONE_N)});
    pmesh->_push_vertex({vec3(-PHI_N,  0.f, -ONE_N)});
    pmesh->_push_vertex({vec3(-PHI_N,  0.f,  ONE_N)});

    pmesh->push_line(0, 1);
    pmesh->push_line(0, 5);
    pmesh->push_line(0, 7);
    pmesh->push_line(0, 10);
    pmesh->push_line(0, 11);
    pmesh->push_line(1, 5);
    pmesh->push_line(1, 7);
    pmesh->push_line(1, 8);
    pmesh->push_line(1, 9);
    pmesh->push_line(2, 3);
    pmesh->push_line(2, 4);
    pmesh->push_line(2, 6);
    pmesh->push_line(2, 10);
    pmesh->push_line(2, 11);
    pmesh->push_line(3, 4);
    pmesh->push_line(3, 6);
    pmesh->push_line(3, 8);
    pmesh->push_line(3, 9);
    pmesh->push_line(4, 5);
    pmesh->push_line(4, 9);
    pmesh->push_line(4, 11);
    pmesh->push_line(5, 9);
    pmesh->push_line(5, 11);
    pmesh->push_line(6, 7);
    pmesh->push_line(6, 8);
    pmesh->push_line(6, 10);
    pmesh->push_line(7, 10);
    pmesh->push_line(7, 8);
    pmesh->push_line(8, 9);
    pmesh->push_line(10, 11);

    pmesh->set_centered(true);

    return pmesh;
}

MeshP* make_segment_x_3P()
{
    MeshP* pmesh = new MeshP;

    pmesh->_push_vertex({vec3(0.f,0.f,0.f)});
    pmesh->_push_vertex({vec3(1.f,0.f,0.f)});

    pmesh->push_line(0, 1);

    return pmesh;
}

MeshP* make_segment_y_3P()
{
    MeshP* pmesh = new MeshP;

    pmesh->_push_vertex({vec3(0.f,0.f,0.f)});
    pmesh->_push_vertex({vec3(0.f,1.f,0.f)});

    pmesh->push_line(0, 1);

    return pmesh;
}

MeshP* make_segment_z_3P()
{
    MeshP* pmesh = new MeshP;

    pmesh->_push_vertex({vec3(0.f,0.f,0.f)});
    pmesh->_push_vertex({vec3(0.f,0.f,1.f)});

    pmesh->push_line(0, 1);

    return pmesh;
}

MeshP* make_cross3D_3P()
{
    MeshP* pmesh = new MeshP;

    pmesh->_push_vertex({vec3(-0.5f,0.f,0.f)});
    pmesh->_push_vertex({vec3(0.5f,0.f,0.f)});
    pmesh->_push_vertex({vec3(0.f,-0.5f,0.f)});
    pmesh->_push_vertex({vec3(0.f,0.5f,0.f)});
    pmesh->_push_vertex({vec3(0.f,0.f,-0.5f)});
    pmesh->_push_vertex({vec3(0.f,0.f,0.5f)});

    pmesh->push_line(0, 1);
    pmesh->push_line(2, 3);
    pmesh->push_line(4, 5);

    return pmesh;
}


static uint32_t get_mid_point(const i32vec2& edge,
                              std::shared_ptr<TriangularMesh> pmesh,
                              std::unordered_map<i32vec2, uint32_t>& lookup)
{
    auto it = lookup.find(edge);

    uint32_t midp;
    if(it != lookup.end())
        midp = it->second;
    else
    {
        // Normalize position to force it on the unit sphere
        vec3 pos(pmesh->mid_position(edge.x(),edge.y()).normalized());
        midp = pmesh->push_vertex({pos, vec3(0.f), vec3(0.f), pmesh->mid_uv(edge.x(),edge.y())});
        lookup.insert(std::make_pair(edge, midp));
    }

    return midp;
}

static inline i32vec2 ordered_edge(uint32_t p1, uint32_t p2)
{
    return (p1<p2)?i32vec2(p1,p2):i32vec2(p2,p1);
}

void subdivide_mesh(std::shared_ptr<TriangularMesh> pmesh)
{
    // Copy indices list
    std::vector<uint32_t> indices(pmesh->get_index_buffer());
    // Hashmap to register new vertices and avoid duplicating them
    std::unordered_map<i32vec2, uint32_t> lookup;
    // for each triangle in mesh
    for(size_t ii=0; ii+2<indices.size(); ii+=3)
    {
        // Triangle points indices
        uint32_t a = indices[ii+0];
        uint32_t b = indices[ii+1];
        uint32_t c = indices[ii+2];

        // Try to find mid points in lookup first
        // If can't find, create new vertex
        uint32_t m_ab = get_mid_point(ordered_edge(a,b), pmesh, lookup);
        uint32_t m_bc = get_mid_point(ordered_edge(b,c), pmesh, lookup);
        uint32_t m_ca = get_mid_point(ordered_edge(c,a), pmesh, lookup);

        // add new triangles
        pmesh->push_triangle(m_ca, a, m_ab);
        pmesh->push_triangle(m_ab, b, m_bc);
        pmesh->push_triangle(m_bc, c, m_ca);
        // reassign old triangle to make center triangle
        pmesh->set_triangle_by_index(ii, i32vec3(m_ab, m_bc, m_ca));
    }
}

std::shared_ptr<TriangularMesh> make_ico_sphere(uint32_t refine, bool finalize)
{
    // * Generate an icosahedron, but don't compute normals and tangents ftm
    std::shared_ptr<TriangularMesh> pmesh = make_icosahedron(false);

    // * Subdivide: replace each triangle by 4 triangles
    for(uint32_t kk=0; kk<refine; ++kk)
        subdivide_mesh(pmesh);

    if(finalize)
    {
        pmesh->build_normals_and_tangents();
        pmesh->compute_dimensions();
    }

    return pmesh;
}


std::shared_ptr<FaceMesh> make_terrain(const HeightMap& hm, float latScale, float texScale)
{
    std::shared_ptr<FaceMesh> pmesh(new FaceMesh);
    // For each quad
    for(uint32_t ii=0; ii+1<hm.get_width(); ++ii)
    {
        for(uint32_t jj=0; jj+1<hm.get_length(); ++jj)
        {
            // Heights for bottom right, top right, top left, bottom left corners of quad
            float hbl = hm.get_height(ii,  jj  );
            float hbr = hm.get_height(ii+1,jj  );
            float htr = hm.get_height(ii+1,jj+1);
            float htl = hm.get_height(ii,  jj+1);

            // Set vertices
            pmesh->push_vertex({latScale*vec3(ii,  hbl,jj),   vec3(0), vec3(0), texScale*vec2(0.0f+ii, 0.0f+jj)}); //-3
            pmesh->push_vertex({latScale*vec3(ii+1,hbr,jj),   vec3(0), vec3(0), texScale*vec2(1.0f+ii, 0.0f+jj)}); //-2
            pmesh->push_vertex({latScale*vec3(ii+1,htr,jj+1), vec3(0), vec3(0), texScale*vec2(1.0f+ii, 1.0f+jj)}); //-1
            uint32_t nvert =
            pmesh->push_vertex({latScale*vec3(ii, htl,jj+1),  vec3(0), vec3(0), texScale*vec2(0.0f+ii, 1.0f+jj)});

            // Set two triangles per quad
            // Change direction each row
            if((jj)%2)
            {
                pmesh->push_triangle(nvert-3, nvert-0, nvert-1);  // Top Left
                pmesh->push_triangle(nvert-3, nvert-1, nvert-2);  // Bottom Right
            }
            else
            {
                pmesh->push_triangle(nvert-3, nvert-0, nvert-2);  // Top Left
                pmesh->push_triangle(nvert-2, nvert-0, nvert-1);  // Bottom Right
            }
        }
    }
    pmesh->build_normals_and_tangents();
    pmesh->smooth_normals_and_tangents(Smooth::COMPRESS_QUADRATIC);
    pmesh->compute_dimensions();

    return pmesh;
}


std::shared_ptr<TriangularMesh> make_terrain_tri_mesh(const HeightMap& hm,
                                            float latScale,
                                            float texScale)
{
    std::shared_ptr<TriangularMesh> pmesh(new TriangularMesh);

    // Hex terrain triangle mesh is 1 tile longer to allow for seamless terrain
    // Actual length is heightmap length minus 1.
    const uint32_t width  = hm.get_width();
    const uint32_t length = hm.get_length()-1;

    // For each position in heightmap
    for(uint32_t ii=0; ii<width; ++ii)
    {
        for(uint32_t jj=0; jj<length; ++jj)
        {
            // Get height
            float h1 = hm.get_height(ii, jj);
            if(ii%2)
            {
                // Set vertex. Index will be ii*length+jj
                pmesh->push_vertex({latScale*vec3(ii, h1, jj), vec3(0), vec3(0),
                                    texScale*vec2(ii, jj)});
            }
            else
            {
                float h2 = hm.get_height(ii, jj+1);
                pmesh->push_vertex({latScale*vec3(ii, 0.5f*(h1+h2), jj+0.5f), vec3(0), vec3(0),
                                    texScale*vec2(ii, jj+0.5f)});
            }
        }
    }
    // For each oblique quad
    for(uint32_t ii=0; ii+1<width; ++ii)
    {
        for(uint32_t jj=0; jj+1<length; ++jj)
        {
            uint32_t ibl = ii*length + jj;
            uint32_t ibr = (ii+1)*length + jj;
            uint32_t itr = (ii+1)*length + jj+1;
            uint32_t itl = ii*length + jj+1;

            // Set two triangles per quad
            // Change direction each column
            if((ii)%2==0)
            {
                pmesh->push_triangle(ibl, itl, itr);  // Top Left
                pmesh->push_triangle(ibl, itr, ibr);  // Bottom Right
            }
            else
            {
                pmesh->push_triangle(ibl, itl, ibr);  // Top Left
                pmesh->push_triangle(ibr, itl, itr);  // Bottom Right
            }
        }
    }

    pmesh->build_normals_and_tangents();
    pmesh->compute_dimensions();

    return pmesh;
}

MeshP* make_quad_3P()
{
    MeshP* pmesh = new MeshP;
    pmesh->_push_vertex({vec3(-1.0f, -1.0f, 0.0f)});
    pmesh->_push_vertex({vec3(1.0f,  -1.0f, 0.0f)});
    pmesh->_push_vertex({vec3(-1.0f,  1.0f, 0.0f)});
    pmesh->_push_vertex({vec3(1.0f,   1.0f, 0.0f)});
    pmesh->_push_triangle(0,  1,  2);
    pmesh->_push_triangle(1,  3,  2);
    return pmesh;
}

MeshPU* make_quad_3P2U()
{
    MeshPU* pmesh = new MeshPU;
    pmesh->_push_vertex({vec3(-1.0f, -1.0f, 0.0f), vec2(0.f, 0.f)});
    pmesh->_push_vertex({vec3(1.0f,  -1.0f, 0.0f), vec2(1.f, 0.f)});
    pmesh->_push_vertex({vec3(-1.0f,  1.0f, 0.0f), vec2(0.f, 1.f)});
    pmesh->_push_vertex({vec3(1.0f,   1.0f, 0.0f), vec2(1.f, 1.f)});
    pmesh->_push_triangle(0,  1,  2);
    pmesh->_push_triangle(1,  3,  2);
    return pmesh;
}

// Procedural meshes



std::shared_ptr<FaceMesh> make_crystal(unsigned seed)
{

    //std::random_device rd;
    std::mt19937 generator(seed);

    //std::default_random_engine generator(seed);
    // Distribution for radial perturbations
    std::uniform_real_distribution<float> polygon_weights_distrib(-1.0f,1.0f);
    // Distribution for dimension modifiers
    std::uniform_real_distribution<float> polygon_dim_distrib(0.0f,1.0f);
    // Distribution for number of sides
    std::uniform_int_distribution<int> polygon_sides_distrib(3,7);

    int n_sides = polygon_sides_distrib(generator);

    std::shared_ptr<FaceMesh> pmesh(new FaceMesh);

    float radius = 0.7f;
    float height[3]{0.0f,
                    3.8f + 1.0f*polygon_dim_distrib(generator),
                    0.0f};
    height[2] = height[1] + 0.2f + 0.7f*polygon_dim_distrib(generator);

    float radius_modifier[3]{1.0f,
                             1.0f + 0.5f*polygon_dim_distrib(generator),
                             1.0f - (0.2f+0.3f*polygon_dim_distrib(generator))};
    float dist_spread = 0.2f;

    float dist_to_center = radius + dist_spread*polygon_weights_distrib(generator);
    float xx = 0.0f;
    float zz = 0.0f;
    float xx_last = dist_to_center;
    float zz_last = 0.0f;
    float xx_first = xx_last;
    float zz_first = zz_last;

    // Create 3 scaled polygons
    for(int ii=0; ii<n_sides; ++ii) // for each face
    {
        if(ii<n_sides-1)
        {
            dist_to_center = radius + dist_spread*polygon_weights_distrib(generator);
            xx = dist_to_center * cos((ii+1)*2.0f*M_PI/n_sides);
            zz = dist_to_center * sin((ii+1)*2.0f*M_PI/n_sides);
        }
        else
        {
            xx = xx_first;
            zz = zz_first;
        }

        // Left side
        pmesh->push_vertex({vec3(xx_last*radius_modifier[0], height[0], zz_last*radius_modifier[0]), vec3(0), vec3(0),
                              vec2(1,1)});
        pmesh->push_vertex({vec3(xx_last*radius_modifier[1], height[1], zz_last*radius_modifier[1]), vec3(0), vec3(0),
                              vec2(1,0)});
        pmesh->push_vertex({vec3(xx_last*radius_modifier[1], height[1], zz_last*radius_modifier[1]), vec3(0), vec3(0),
                              vec2(1,1)});
        pmesh->push_vertex({vec3(xx_last*radius_modifier[2], height[2], zz_last*radius_modifier[2]), vec3(0), vec3(0),
                              vec2(1,0)});
        pmesh->push_vertex({vec3(xx_last*radius_modifier[2], height[2], zz_last*radius_modifier[2]), vec3(0), vec3(0),
                              vec2(1,1)});
        // Right side
        pmesh->push_vertex({vec3(xx*radius_modifier[0], height[0], zz*radius_modifier[0]), vec3(0), vec3(0),
                              vec2(0,1)});
        pmesh->push_vertex({vec3(xx*radius_modifier[1], height[1], zz*radius_modifier[1]), vec3(0), vec3(0),
                              vec2(0,0)});
        pmesh->push_vertex({vec3(xx*radius_modifier[1], height[1], zz*radius_modifier[1]), vec3(0), vec3(0),
                              vec2(0,1)});
        pmesh->push_vertex({vec3(xx*radius_modifier[2], height[2], zz*radius_modifier[2]), vec3(0), vec3(0),
                              vec2(0,0)});
        pmesh->push_vertex({vec3(xx*radius_modifier[2], height[2], zz*radius_modifier[2]), vec3(0), vec3(0),
                              vec2(0,1)});

        xx_last = xx;
        zz_last = zz;
    }
    // top vertex
    pmesh->push_vertex({vec3(0.0f,height[2],0.0f), vec3(0), vec3(0), vec2(0.0f)});

    // triangles
    for(int F=0; F<n_sides; ++F) // for each face
    {
        unsigned int vv = 10*F;
        // bottom face
        pmesh->push_triangle(vv, vv+6, vv+5); // inferior
        pmesh->push_triangle(vv, vv+1, vv+6); // superior
        // mid face
        pmesh->push_triangle(vv+2, vv+8, vv+7); // inferior
        pmesh->push_triangle(vv+2, vv+3, vv+8); // superior
        // top triangle
        pmesh->push_triangle(vv+4, 10*n_sides, vv+9);
    }
    pmesh->build_normals_and_tangents();
    pmesh->compute_dimensions();

    return pmesh;
}

std::shared_ptr<FaceMesh> make_tentacle(const math::CSplineCatmullV3& spline,
                                    uint32_t nRings,
                                    uint32_t nRingPoints,
                                    float radius_0,
                                    float radius_exponent)
{
    std::shared_ptr<FaceMesh> pmesh(new FaceMesh);
    skin_spline(pmesh, spline, nRings, nRingPoints, radius_0, radius_exponent);
    pmesh->build_normals_and_tangents();
    pmesh->compute_dimensions();
    pmesh->smooth_normals_and_tangents();
    return pmesh;
}

void skin_spline(std::shared_ptr<FaceMesh> pmesh,
                 const math::CSplineCatmullV3& spline,
                 uint32_t nRings,
                 uint32_t nRingPoints,
                 float radius_0,
                 float radius_exponent)
{
    const float step = 1.0f/(nRings);
    vec3 xx(1,0,0);
    vec3 B, C, t_x, t_z;
    float radius_b, radius_t;
    for(uint32_t rr=0; rr<nRings-1; ++rr)
    {
        radius_b = radius_0 * 1.0f/pow(rr+1, radius_exponent);
        radius_t = radius_0 * 1.0f/pow(rr+2, radius_exponent);

        // Compute 3 levels of circle centers (along spline)
        float t0 = rr*step;
        float t1 = (rr+1)*step;
        float t2 = (rr+2)*step;
        vec3 A(spline.interpolate(t0));
        B = spline.interpolate(t1);
        C = spline.interpolate(t2);

        // A "ring" is a cylinder slice
        // Bottom orientation
        vec3 b_y(B-A);
        b_y.normalize();
        // Top orientation
        vec3 t_y(C-B);
        t_y.normalize();
        // Compute circles local basis
        vec3 b_z(xx.cross(b_y));
        b_z.normalize();
        vec3 b_x(b_y.cross(b_z));
        b_x.normalize();
        t_z = xx.cross(t_y);
        t_z.normalize();
        t_x = t_y.cross(t_z);
        t_x.normalize();

        // Add each ring face
        for(uint32_t ff=0; ff<nRingPoints; ++ff)
        {
            float theta_l = 2*M_PI * ff/(1.0f*nRingPoints);
            float theta_r = 2*M_PI * (ff+1)/(1.0f*nRingPoints);

            vec3 bl_pos(A + radius_b*cos(theta_l)*b_x + radius_b*sin(theta_l)*b_z);
            vec3 tl_pos(B + radius_t*cos(theta_l)*t_x + radius_t*sin(theta_l)*t_z);
            vec3 br_pos(A + radius_b*cos(theta_r)*b_x + radius_b*sin(theta_r)*b_z);
            vec3 tr_pos(B + radius_t*cos(theta_r)*t_x + radius_t*sin(theta_r)*t_z);

            uint32_t BL = pmesh->push_vertex({bl_pos, vec3(0.f), vec3(0.f), vec2(0.f, 0.f)});
            uint32_t TL = pmesh->push_vertex({tl_pos, vec3(0.f), vec3(0.f), vec2(0.f, 1.f)});
            uint32_t BR = pmesh->push_vertex({br_pos, vec3(0.f), vec3(0.f), vec2(1.f, 0.f)});
            uint32_t TR = pmesh->push_vertex({tr_pos, vec3(0.f), vec3(0.f), vec2(1.f, 1.f)});

            pmesh->push_triangle(BL, TR, BR);
            pmesh->push_triangle(BL, TL, TR);
        }
    }

    // Add end cone
    for(uint32_t ff=0; ff<nRingPoints; ++ff)
    {
        float theta_l = 2*M_PI * ff/(1.0f*nRingPoints);
        float theta_r = 2*M_PI * (ff+1)/(1.0f*nRingPoints);

        vec3 bl_pos(B + radius_t*cos(theta_l)*t_x + radius_t*sin(theta_l)*t_z);
        vec3 br_pos(B + radius_t*cos(theta_r)*t_x + radius_t*sin(theta_r)*t_z);

        uint32_t BL = pmesh->push_vertex({bl_pos, vec3(0.f), vec3(0.f), vec2(0.f,0.f)});
        uint32_t BR = pmesh->push_vertex({br_pos, vec3(0.f), vec3(0.f), vec2(1.f,0.f)});
        uint32_t T  = pmesh->push_vertex({C,      vec3(0.f), vec3(0.f), vec2(0.5f,1.f)});

        pmesh->push_triangle(BL, T, BR);
    }
}

} // namespace factory
} // namespace wcore
