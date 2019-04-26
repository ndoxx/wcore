#include "geometry_common.h"
#include "mesh_factory.h"
#include "logger.h"

namespace wcore
{

using namespace math;

GeometryCommon::GeometryCommon():
buffer_unit_line_(GL_LINES)
{
    add_mesh_3P("quad"_h, factory::make_quad_3P());
    add_mesh_3P("sphere"_h, factory::make_uv_sphere_3P(4, 7));

    add_mesh_line("cube_line"_h, factory::make_cube_3P());
    add_mesh_line("sphere_line"_h, factory::make_uv_sphere_3P(4, 7, true));
    add_mesh_line("segment_x"_h, factory::make_segment_x_3P());
    add_mesh_line("segment_y"_h, factory::make_segment_y_3P());
    add_mesh_line("segment_z"_h, factory::make_segment_z_3P());
    add_mesh_line("cross"_h, factory::make_cross3D_3P());


    // Screen quad for 2D renderers
    float xpos = 0.0f;
    float ypos = 0.0f;
    float w = 1.0f;
    float h = 1.0f;
    Mesh<Vertex2P2U>* scr_quad_mesh = new Mesh<Vertex2P2U>();
    scr_quad_mesh->_emplace_vertex(vec2(xpos,   ypos  ), vec2(0, 0));
    scr_quad_mesh->_emplace_vertex(vec2(xpos+w, ypos  ), vec2(1, 0));
    scr_quad_mesh->_emplace_vertex(vec2(xpos+w, ypos+h), vec2(1, 1));
    scr_quad_mesh->_emplace_vertex(vec2(xpos,   ypos+h), vec2(0, 1));
    scr_quad_mesh->_push_triangle(0,  1,  2);
    scr_quad_mesh->_push_triangle(0,  2,  3);
    add_mesh_2P2U("screen_quad"_h, scr_quad_mesh);

    Mesh<Vertex2P2U>* char_quad_mesh = new Mesh<Vertex2P2U>();
    char_quad_mesh->_emplace_vertex(vec2(xpos,   ypos  ), vec2(0, 1));
    char_quad_mesh->_emplace_vertex(vec2(xpos+w, ypos  ), vec2(1, 1));
    char_quad_mesh->_emplace_vertex(vec2(xpos+w, ypos+h), vec2(1, 0));
    char_quad_mesh->_emplace_vertex(vec2(xpos,   ypos+h), vec2(0, 0));
    char_quad_mesh->_push_triangle(0,  1,  2);
    char_quad_mesh->_push_triangle(0,  2,  3);
    add_mesh_2P2U("char_quad"_h, scr_quad_mesh);

    buffer_unit_3P_.upload();
    buffer_unit_2P2U_.upload();
    buffer_unit_line_.upload();
}

GeometryCommon::~GeometryCommon()
{

}

void GeometryCommon::add_mesh_3P(hash_t hname, Mesh<Vertex3P>* pmesh)
{
    buffer_unit_3P_.submit(*pmesh);

    MeshInfo mesh_info;
    mesh_info.offset = pmesh->get_buffer_offset();
    mesh_info.n_elem = pmesh->get_n_elements();
    mesh_info.buffer = MeshInfo::BufferIndex::BUFFER_3P;
    meshes_.insert(std::pair(hname, mesh_info));

    delete pmesh;
}

void GeometryCommon::add_mesh_2P2U(hash_t hname, Mesh<Vertex2P2U>* pmesh)
{
    buffer_unit_2P2U_.submit(*pmesh);

    MeshInfo mesh_info;
    mesh_info.offset = pmesh->get_buffer_offset();
    mesh_info.n_elem = pmesh->get_n_elements();
    mesh_info.buffer = MeshInfo::BufferIndex::BUFFER_2P2U;
    meshes_.insert(std::pair(hname, mesh_info));

    delete pmesh;
}

void GeometryCommon::add_mesh_line(hash_t hname, Mesh<Vertex3P>* pmesh)
{
    buffer_unit_line_.submit(*pmesh);

    MeshInfo mesh_info;
    mesh_info.offset = pmesh->get_buffer_offset();
    mesh_info.n_elem = pmesh->get_n_elements();
    mesh_info.buffer = MeshInfo::BufferIndex::BUFFER_LINE;
    meshes_.insert(std::pair(hname, mesh_info));

    delete pmesh;
}


void GeometryCommon::draw(hash_t hname)
{
    auto it = meshes_.find(hname);

    if(it != meshes_.end())
    {
        const MeshInfo& info = it->second;
        switch(info.buffer)
        {
            case MeshInfo::BufferIndex::BUFFER_3P:
                buffer_unit_3P_.draw(info.n_elem, info.offset);
                break;
            case MeshInfo::BufferIndex::BUFFER_2P2U:
                buffer_unit_2P2U_.draw(info.n_elem, info.offset);
                break;
            case MeshInfo::BufferIndex::BUFFER_LINE:
                buffer_unit_line_.draw(info.n_elem, info.offset);
                break;
        }
    }
    else
    {
        DLOGE("Unknown common geometry name: " + std::to_string(hname) + " -> " + HRESOLVE(hname), "core");
    }
}


}
