#include <cctype>
#include <cstring>
#include <cstdio>
#include <sstream>
#include <fstream>
#include <map>
#include <vector>
#include <string>

#include <algorithm>
#include <cctype>
#include <locale>

#include "obj_loader.h"
#include "surface_mesh.h"
#include "vertex_format.h"
#include "logger.h"
#include "string_utils.h"

namespace wcore
{

using namespace math;

#define TEXCOORD 0x01
#define NORMAL   0x02
struct Triangle
{
public:
    math::i32vec3 indices;
    std::array<math::vec3,3> vertices;
    std::array<math::vec3,3> uvs;
    std::array<math::vec3,3> normals;
    int material;
    int attributes;
};

std::shared_ptr<SurfaceMesh> ObjLoader::load(std::istream& stream,
                                             bool process_uv,
                                             bool process_normals,
                                             int smooth_func)
{
#ifdef __DEBUG__
    DLOGN("[ObjLoader] Loading obj file from stream.", "model", Severity::LOW);
    //DLOGI("<p>" + std::string(objfile) + "</p>", "model", Severity::LOW);
#endif

    // Setup temporary variables
    std::string line, mtllib, usemtl;
    int material = -1;
    std::unordered_map<std::string, int> material_map;
    std::vector<vec3> positions;
    std::vector<vec3> normals;
    std::vector<vec3> uvs;
    std::vector<Triangle> triangles;
    std::vector<std::string> materials;

    // For each line
    bool first_line = true;
    bool is_blender_export = false;
    while(std::getline(stream, line))
    {
#ifdef __DEBUG__
        if(first_line)
        {
            // Read first line and detect Blender exports
            if(!line.substr(0,9).compare("# Blender"))
            {
                DLOGI("<h>Blender</h> export detected.", "model", Severity::LOW);
                is_blender_export = true;
            }
            first_line = false;
        }
#endif

        if(!line.substr(0,6).compare("mtllib"))
        {
            //mtllib = trim_whitespaces(&line[7]);
            mtllib = line.substr(7);
            trim_whitespaces(mtllib);
        }
        //if(strncmp(line, "usemtl", 6) == 0)
        if(!line.substr(0,6).compare("usemtl"))
        {
            //std::string usemtl(trim_whitespaces(&line[7]));
            usemtl = line.substr(7);
            trim_whitespaces(usemtl);
            if(material_map.find(usemtl) == material_map.end())
            {
                material_map[usemtl] = materials.size();
                materials.push_back(usemtl);
            }
            material = material_map[usemtl];
        }

        // Vertex position
        vec3 pos;
        vec3 uv;
        vec3 normal;
        if(line[0] == 'v' && line[1] == ' ')
        {
            if(sscanf(line.c_str(),"v %f %f %f", &pos[0], &pos[1], &pos[2])==3)
                positions.push_back(pos);
        }
        // Texture coordinate
        else if(line[0] == 'v' && line[1] == 't' && line[2] == ' ')
        {
            if(sscanf(line.c_str(),"vt %f %f", &uv[0], &uv[1])==2)
            {
                uv[2] = 0.0f;
                uvs.push_back(uv);
            }
            else if(sscanf(line.c_str(),"vt %f %f %f", &uv[0], &uv[1], &uv[2])==3)
            {
                uvs.push_back(uv);
            }
        }
        // Normal
        else if(line[0] == 'v' && line[1] == 'n' && line[2] == ' ')
        {
            if(sscanf(line.c_str(),"vn %f %f %f", &normal[0], &normal[1], &normal[2])==3)
            {
                normal.normalize();
                normals.push_back(normal);
            }
        }

        // Faces
        int integers[9];
        if(line[0] == 'f')
        {
            Triangle tri;
            bool tri_ok = false;
            bool has_uv = false;
            bool has_nm = false;

            // f v1 v2 v3
            if(sscanf(line.c_str(),"f %d %d %d", &integers[0], &integers[1], &integers[2])==3)
            {
                tri_ok = true;
            }
            // f v1// v2// v3//
            else if(sscanf(line.c_str(),"f %d// %d// %d//", &integers[0], &integers[1], &integers[2])==3)
            {
                tri_ok = true;
            }
            // f v1//vn1 v2//vn2 v3//vn3
            else if(sscanf(line.c_str(),"f %d//%d %d//%d %d//%d",
                &integers[0], &integers[3],
                &integers[1], &integers[4],
                &integers[2], &integers[5])==6)
            {
                tri_ok = true;
                has_nm = true;
            }
            // f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3
            else if(sscanf(line.c_str(),"f %d/%d/%d %d/%d/%d %d/%d/%d",
                &integers[0], &integers[6], &integers[3],
                &integers[1], &integers[7], &integers[4],
                &integers[2], &integers[8], &integers[5])==9)
            {
                tri_ok = true;
                has_uv = true;
                has_nm = true;
            }
            else
            {
                DLOGE("Unrecognized sequence during face extraction: ", "model", Severity::CRIT);
                DLOGI(line, "model", Severity::CRIT);
                return nullptr;
            }

            if(tri_ok)
            {
                tri.indices = math::i32vec3(integers[0]-1, integers[1]-1, integers[2]-1);
                tri.vertices[0] = positions[integers[0]-1];
                tri.vertices[1] = positions[integers[1]-1];
                tri.vertices[2] = positions[integers[2]-1];

                tri.attributes = 0;

                if(process_uv && has_uv)
                {
                    tri.uvs[0] = uvs[integers[6]-1];
                    tri.uvs[1] = uvs[integers[7]-1];
                    tri.uvs[2] = uvs[integers[8]-1];

                    tri.attributes |= TEXCOORD;
                }

                if(process_normals && has_nm)
                {
                    tri.normals[0] = normals[integers[3]-1];
                    tri.normals[1] = normals[integers[4]-1];
                    tri.normals[2] = normals[integers[5]-1];

                    tri.attributes |= NORMAL;
                }

                tri.material = material;
                triangles.push_back(tri);
            }
        }
    }

    DLOGI("#triangles: <v>" + std::to_string(triangles.size()) + "</v>", "model", Severity::LOW);
    DLOGI("#vertices:  <v>" + std::to_string(positions.size()) + "</v>", "model", Severity::LOW);
    DLOGI("#normals:   <v>" + std::to_string(normals.size())   + "</v>", "model", Severity::LOW);
    DLOGI("#UVs:       <v>" + std::to_string(uvs.size())       + "</v>", "model", Severity::LOW);

    // TODO Refactor this shyte
    if(!process_normals)
    {
        std::shared_ptr<TriangularMesh> pmesh(new TriangularMesh);
        for(uint32_t ii=0; ii<positions.size(); ++ii)
        {
            pmesh->emplace_vertex(positions[ii],
                                  vec3(0),
                                  vec3(0),
                                  vec2(0));
        }
        for(uint32_t ii=0; ii<triangles.size(); ++ii)
        {
            for(uint32_t jj=0; jj<3; ++jj)
                (*pmesh)[triangles[ii].indices[jj]].uv_ = process_uv ? triangles[ii].uvs[jj].xy() : vec2(0);

            pmesh->push_triangle(triangles[ii].indices);
        }
        pmesh->build_normals_and_tangents();
        pmesh->compute_dimensions();

        return static_cast<std::shared_ptr<SurfaceMesh>>(pmesh);
    }
    else
    {
        std::shared_ptr<FaceMesh> pmesh(new FaceMesh);
        for(uint32_t ii=0; ii<triangles.size(); ++ii)
        {
            for(uint32_t jj=0; jj<3; ++jj)
            {
                pmesh->emplace_vertex(triangles[ii].vertices[jj],
                                      triangles[ii].normals[jj],
                                      vec3(0),
                                      process_uv ? triangles[ii].uvs[jj].xy() : vec2(0));
            }
            pmesh->push_triangle(3*ii, 3*ii+1, 3*ii+2);
        }
        pmesh->smooth_normals_and_tangents((Smooth)smooth_func);
        pmesh->build_tangents();
        pmesh->compute_dimensions();

        return static_cast<std::shared_ptr<SurfaceMesh>>(pmesh);
    }
}

std::shared_ptr<SurfaceMesh> ObjLoader::load(const char* objfile,
                                             bool process_uv,
                                             bool process_normals,
                                             int smooth_func)
{
    // Open file, sanity check
    std::ifstream stream(objfile);
    if(!stream.is_open())
    {
        DLOGE(std::string("Unable to open file: ") + objfile, "model", Severity::CRIT);
        return nullptr;
    }

    return load(stream, process_uv, process_normals, smooth_func);
}

std::shared_ptr<SurfaceMesh> ObjLoader::load(const fs::path& path,
                                             bool process_uv,
                                             bool process_normals,
                                             int smooth_func)
{
    if(fs::exists(path))
        return load(path.string().c_str(), process_uv, process_normals, smooth_func);
    return nullptr;
}


}
