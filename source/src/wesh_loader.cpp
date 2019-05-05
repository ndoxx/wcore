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

void WeshLoader::read_header(std::istream& stream, WeshHeaderWrapper& header)
{
    stream.read(reinterpret_cast<char*>(&header), sizeof(header));
}

void WeshLoader::write_header(std::ostream& stream,
                              uint32_t n_vertices,
                              uint32_t n_indices,
                              uint32_t vertex_size)
{
    WeshHeaderWrapper header;
    header.h.magic         = WESH_MAGIC;
    header.h.version_major = WESH_VERSION_MAJOR;
    header.h.version_minor = WESH_VERSION_MINOR;
    header.h.vertex_size   = vertex_size;
    header.h.n_vertices    = n_vertices;
    header.h.n_indices     = n_indices;

    // Set padding bytes to 0
    //memset(&header + sizeof(WeshHeader), 0x00, WESH_HEADER_SIZE-sizeof(WeshHeader));

    stream.write(reinterpret_cast<const char*>(&header), sizeof(header));
}

bool WeshLoader::header_sanity_check(const WeshHeader& header, size_t vertex_size)
{
    // Check magic bytes
    if(header.magic != WESH_MAGIC)
    {
        DLOGE("[Wesh] Not a valid Wesh file.", "parsing");
        DLOGI("Magic bytes should be " + std::to_string(WESH_MAGIC) + " but got " + std::to_string(header.magic), "parsing");
        return false;
    }

    // Check vertex size
    if(header.vertex_size != vertex_size)
    {
        DLOGE("[Wesh] Vertex size mismatch.", "parsing");
        return false;
    }

    // Check version for compatibility
    bool version_ok = header.version_major == WESH_VERSION_MAJOR
                   && header.version_minor == WESH_VERSION_MINOR;
    if(!version_ok)
    {
        DLOGW("[Wesh] Version mismatch. Data may not be fetched correctly.", "parsing");
        DLOGI("Required version: " + std::to_string(WESH_VERSION_MAJOR) + "."
                                   + std::to_string(WESH_VERSION_MINOR), "parsing");
        DLOGI("Got: " + std::to_string(header.version_major) + "."
                      + std::to_string(header.version_minor), "parsing");
    }

    return true;
}


} // namespace wcore
