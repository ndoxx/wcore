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

std::shared_ptr<AnimMesh> WeshLoader::load(std::istream& stream)
{
    // Read header
    WeshHeaderWrapper header;
    if(!read_header(stream, header))
        return nullptr;

    size_t vsize, isize;

    // Read vertex data size
    stream.read(reinterpret_cast<char*>(&vsize), sizeof(vsize));
    // Read vertex data
    std::vector<VertexAnim> vertices(vsize);
    stream.read(reinterpret_cast<char*>(&vertices[0]), vsize*sizeof(VertexAnim));
    // Read index data size
    stream.read(reinterpret_cast<char*>(&isize), sizeof(isize));
    // Read index data
    std::vector<uint32_t> indices(isize);
    stream.read(reinterpret_cast<char*>(&indices[0]), isize*sizeof(uint32_t));

    return std::make_shared<AnimMesh>(std::move(vertices), std::move(indices));
}

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

void WeshLoader::write(std::ostream& stream,
                       const std::vector<VertexAnim>& vertices,
                       const std::vector<uint32_t>& indices)
{
    // Write header
    write_header(stream);

    size_t vsize = vertices.size();
    size_t isize = indices.size();

    // Write vertex data size
    stream.write(reinterpret_cast<const char*>(&vsize), sizeof(vsize));
    // Write vertex data
    stream.write(reinterpret_cast<const char*>(&vertices[0]), vertices.size()*sizeof(VertexAnim));
    // Write index data size
    stream.write(reinterpret_cast<const char*>(&isize), sizeof(isize));
    // Write index data
    stream.write(reinterpret_cast<const char*>(&indices[0]), indices.size()*sizeof(uint32_t));
}

bool WeshLoader::write_header(std::ostream& stream)
{
    WeshHeaderWrapper header;
    header.h.magic         = WESH_MAGIC;
    header.h.version_major = WESH_VERSION_MAJOR;
    header.h.version_minor = WESH_VERSION_MINOR;

    // Set padding bytes to 0
    //memset(&header + sizeof(WeshHeader), 0x00, WESH_HEADER_SIZE-sizeof(WeshHeader));

    stream.write(reinterpret_cast<const char*>(&header), sizeof(header));
    return true;
}

void WeshLoader::write(std::ostream& stream, const AnimMesh& mesh)
{
    write(stream,
          mesh.get_vertex_buffer(),
          mesh.get_index_buffer());
}

} // namespace wcore
