#include <cstring>

#include "wesh_loader.h"
#include "mesh.hpp"
#include "vertex_format.h"
#include "logger.h"

#define WESH_MAGIC 0x48534557 // ASCII(WESH)
#define WESH_VERSION_MAJOR 1
#define WESH_VERSION_MINOR 0

namespace wcore
{

// ----------------------------- ANIMATED MESHES ------------------------------

bool WeshLoader::load(std::istream& stream,
                      std::vector<VertexAnim>& vertices,
                      std::vector<uint32_t>& indices)
{
    // Check that input vectors are empty
    assert(vertices.size()==0);
    assert(indices.size()==0);

    // Read header
    WeshHeaderWrapper header;
    if(!read_header(stream, header))
        return false;

    if(!header.h.is_animated)
    {
        DLOGW("[Wesh] Mesh is static, skipping.", "parsing");
        return false;
    }

    size_t vsize = header.h.n_vertices;
    size_t isize = header.h.n_indices;
    vertices.resize(vsize);
    indices.resize(isize);

    // Read vertex data
    stream.read(reinterpret_cast<char*>(&vertices[0]), vsize*sizeof(VertexAnim));
    // Read index data
    stream.read(reinterpret_cast<char*>(&indices[0]), isize*sizeof(uint32_t));

    return true;
}

void WeshLoader::write(std::ostream& stream,
                       const std::vector<VertexAnim>& vertices,
                       const std::vector<uint32_t>& indices)
{
    // Write header
    size_t vsize = vertices.size();
    size_t isize = indices.size();
    write_header(stream, vsize, isize, true);

    // Write vertex data
    stream.write(reinterpret_cast<const char*>(&vertices[0]), vsize*sizeof(VertexAnim));
    // Write index data
    stream.write(reinterpret_cast<const char*>(&indices[0]), isize*sizeof(uint32_t));
}

void WeshLoader::write(std::ostream& stream, const AnimMesh& mesh)
{
    write(stream,
          mesh.get_vertex_buffer(),
          mesh.get_index_buffer());
}


// ------------------------------ STATIC MESHES -------------------------------

bool WeshLoader::load(std::istream& stream,
                      std::vector<Vertex3P3N3T2U>& vertices,
                      std::vector<uint32_t>& indices)
{
    // Check that input vectors are empty
    assert(vertices.size()==0);
    assert(indices.size()==0);

    // Read header
    WeshHeaderWrapper header;
    if(!read_header(stream, header))
        return false;

    if(header.h.is_animated)
    {
        DLOGW("[Wesh] Mesh is animated, skipping.", "parsing");
        return false;
    }

    size_t vsize = header.h.n_vertices;
    size_t isize = header.h.n_indices;
    vertices.resize(vsize);
    indices.resize(isize);

    // Read vertex data
    stream.read(reinterpret_cast<char*>(&vertices[0]), vsize*sizeof(Vertex3P3N3T2U));
    // Read index data
    stream.read(reinterpret_cast<char*>(&indices[0]), isize*sizeof(uint32_t));

    return true;
}

void WeshLoader::write(std::ostream& stream,
                       const std::vector<Vertex3P3N3T2U>& vertices,
                       const std::vector<uint32_t>& indices)
{
    // Write header
    size_t vsize = vertices.size();
    size_t isize = indices.size();
    write_header(stream, vsize, isize, false);

    // Write vertex data
    stream.write(reinterpret_cast<const char*>(&vertices[0]), vsize*sizeof(Vertex3P3N3T2U));
    // Write index data
    stream.write(reinterpret_cast<const char*>(&indices[0]), isize*sizeof(uint32_t));
}

void WeshLoader::write(std::ostream& stream, const SurfaceMesh& mesh)
{
    write(stream,
          mesh.get_vertex_buffer(),
          mesh.get_index_buffer());
}


// ------------------------------ HEADER STUFF --------------------------------

bool WeshLoader::read_header(std::istream& stream, WeshHeaderWrapper& header)
{
    stream.read(reinterpret_cast<char*>(&header), sizeof(header));

    // Check magic bytes
    if(header.h.magic != WESH_MAGIC)
    {
        DLOGE("[Wesh] Not a valid Wesh file.", "parsing");
        return false;
    }

    // Check version for compatibility
    bool version_ok = header.h.version_major == WESH_VERSION_MAJOR
                   && header.h.version_minor == WESH_VERSION_MINOR;
    if(!version_ok)
    {
        DLOGW("[Wesh] Version mismatch. Data may not be fetched correctly.", "parsing");
        DLOGI("Required version: " + std::to_string(WESH_VERSION_MAJOR) + "."
                                   + std::to_string(WESH_VERSION_MINOR), "parsing");
        DLOGI("Got: " + std::to_string(header.h.version_major) + "."
                      + std::to_string(header.h.version_minor), "parsing");
    }
    return true;
}

bool WeshLoader::write_header(std::ostream& stream,
                              uint32_t n_vertices,
                              uint32_t n_indices,
                              bool is_animated)
{
    WeshHeaderWrapper header;
    header.h.magic         = WESH_MAGIC;
    header.h.version_major = WESH_VERSION_MAJOR;
    header.h.version_minor = WESH_VERSION_MINOR;
    header.h.is_animated   = is_animated;
    header.h.n_vertices    = n_vertices;
    header.h.n_indices     = n_indices;

    // Set padding bytes to 0
    //memset(&header + sizeof(WeshHeader), 0x00, WESH_HEADER_SIZE-sizeof(WeshHeader));

    stream.write(reinterpret_cast<const char*>(&header), sizeof(header));
    return true;
}

} // namespace wcore
