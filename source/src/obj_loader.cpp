#include <cctype>
#include <cstring>
#include <cstdio>
#include <sstream>
#include <map>
#include <vector>
#include <string>

#include "obj_loader.h"
#include "surface_mesh.h"
#include "vertex_format.h"
#include "logger.h"

using namespace math;

ObjLoader::ObjLoader()
{

}

ObjLoader::~ObjLoader()
{

}


static char* trim_whitespaces(char* str)
{
    // Trim leading spaces
    while(isspace((unsigned char)*str)) ++str;

    // If string is all spaces
    if(*str == 0)
    return str;

    // Trim trailing spaces
    char* end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) --end;

    // Write new null terminator
    *(end+1) = 0;

    return str;
}

#define MAXLINE 512
SurfaceMesh* ObjLoader::operator()(const char* objfile, bool process_uv)
{
#ifdef __DEBUG_MODEL__
    DLOGN("[ObjLoader] Loading obj file: ");
    DLOGI(objfile);
#endif

    // Open file, sanity check
    FILE* fn;
    if(objfile==NULL) return nullptr;
    if((char)objfile[0]==0) return nullptr;
    if((fn = fopen(objfile, "rb")) == NULL)
    {
        DLOGE(std::string("File ") + objfile + std::string(" not found."));
        return nullptr;
    }

    // Setup temporary variables
    char line[MAXLINE];
    char* mtllib;
    memset(line, 0, MAXLINE);
    int vertex_cnt = 0;
    int material = -1;
    std::unordered_map<std::string, int> material_map;
    std::vector<vec3> positions;
    std::vector<vec3> normals;
    std::vector<vec3> uvs;
    std::vector<Triangle> triangles;
    std::vector<std::string> materials;
    std::vector<std::vector<int>> uvMap;

    // For each line
    while(fgets(line, MAXLINE, fn) != NULL)
    {
        if(strncmp(line, "mtllib", 6) == 0)
        {
            mtllib = trim_whitespaces(&line[7]);
        }
        if(strncmp(line, "usemtl", 6) == 0)
        {
            std::string usemtl(trim_whitespaces(&line[7]));
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
            if(sscanf(line,"v %f %f %f", &pos[0], &pos[1], &pos[2])==3)
                positions.push_back(pos);
        }
        // Texture coordinate
        else if(line[0] == 'v' && line[1] == 't' && line[2] == ' ')
        {
            if(sscanf(line,"vt %f %f", &uv[0], &uv[1])==2)
            {
                uv[2] = 0.0f;
                uvs.push_back(uv);
            }
            else if(sscanf(line,"vt %f %f %f", &uv[0], &uv[1], &uv[2])==3)
            {
                uvs.push_back(uv);
            }
        }
        // Normal
        else if(line[0] == 'v' && line[1] == 'n' && line[2] == ' ')
        {
            if(sscanf(line,"vn %f %f %f", &normal[0], &normal[1], &normal[2])==3)
            {
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

            if(sscanf(line,"f %d %d %d", &integers[0], &integers[1], &integers[2])==3)
            {
                tri_ok = true;
            }
            else if(sscanf(line,"f %d// %d// %d//", &integers[0], &integers[1], &integers[2])==3)
            {
                tri_ok = true;
            }
            else if(sscanf(line,"f %d//%d %d//%d %d//%d",
                &integers[0], &integers[3],
                &integers[1], &integers[4],
                &integers[2], &integers[5])==6)
            {
                tri_ok = true;
            }
            else if(sscanf(line,"f %d/%d/%d %d/%d/%d %d/%d/%d",
                &integers[0], &integers[6], &integers[3],
                &integers[1], &integers[7], &integers[4],
                &integers[2], &integers[8], &integers[5])==9)
            {
                tri_ok = true;
                has_uv = true;
            }
            else
            {
                DLOGE("Unrecognized sequence during face extraction: ");
                DLOGI(line);
                fclose(fn);
                return nullptr;
            }

            if(tri_ok)
            {
                tri.indices[0] = integers[0]-1-vertex_cnt;
                tri.indices[1] = integers[1]-1-vertex_cnt;
                tri.indices[2] = integers[2]-1-vertex_cnt;
                tri.attributes = 0;

                if(process_uv && has_uv)
                {
                    std::vector<int> indices;
                    indices.push_back(integers[6]-1-vertex_cnt);
                    indices.push_back(integers[7]-1-vertex_cnt);
                    indices.push_back(integers[8]-1-vertex_cnt);
                    uvMap.push_back(indices);
                    tri.attributes |= TEXCOORD;
                }

                tri.material = material;
                triangles.push_back(tri);
            }
        }
    }

    if (process_uv && uvs.size())
        for(uint32_t ii=0; ii<triangles.size(); ++ii)
            for(uint32_t jj=0; jj<3; ++jj)
                triangles[ii].uvs[jj] = uvs[uvMap[ii][jj]];

#ifdef __DEBUG_MODEL__
    std::stringstream ss;
    ss << "#vertices: " << positions.size()
       << ", #normals: " << normals.size()
       << ", #triangles: " << triangles.size()
       << ", #UVs: " << uvs.size();
    DLOGI(ss.str());
#endif

    fclose(fn);

    // TMP
    TriangularMesh* pmesh = new TriangularMesh;
    for(uint32_t ii=0; ii<positions.size(); ++ii)
    {
        pmesh->emplace_vertex(positions[ii], vec3(0), vec3(0), vec2(0));
    }
    for(uint32_t ii=0; ii<triangles.size(); ++ii)
    {
        for(uint32_t jj=0; jj<3; ++jj)
        {
            (*pmesh)[triangles[ii].indices[jj]].set_uv(triangles[ii].uvs[jj].xy());
        }
        pmesh->push_triangle(triangles[ii].indices);
    }
    pmesh->build_normals_and_tangents();
    pmesh->compute_dimensions();

    return (SurfaceMesh*)pmesh;
}
