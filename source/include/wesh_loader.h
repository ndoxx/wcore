#ifndef WESH_LOADER_H
#define WESH_LOADER_H

/*
    WeshLoader class can read and write .wesh files. These binary files
    contain mesh information, data is stored in an interleaved fashion.
    TMP: For the moment, this format only supports animated meshes with underlying
    VertexAnim vertex format.

    As of version 1.0 data is layed out like this:
    [HEADER]              -> 128 bytes, padded
    [array of VertexAnim] -> vertex buffer content
    [array of uint32_t]   -> index buffer content

    Header contains among other things the number of vertices and indices.
*/

#include <filesystem>
#include <fstream>
#include <memory>
#include <cassert>

namespace fs = std::filesystem;

namespace wcore
{

template <typename VertexT> class Mesh;

//#pragma pack(push,1)
struct WeshHeader
{
    uint32_t magic;
    uint16_t version_major;
    uint16_t version_minor;
    uint32_t vertex_size;
    uint32_t n_vertices;
    uint32_t n_indices;
};
//#pragma pack(pop)

#define WESH_HEADER_SIZE 128
typedef union
{
    struct WeshHeader h;
    uint8_t padding[WESH_HEADER_SIZE];
} WeshHeaderWrapper;

class WeshLoader
{
public:
    template<typename VertexT>
    bool read(std::istream& stream,
              std::vector<VertexT>& vertices,
              std::vector<uint32_t>& indices);

    template<typename VertexT>
    void write(std::ostream& stream,
               const std::vector<VertexT>& vertices,
               const std::vector<uint32_t>& indices);

    template<typename VertexT>
    void write(std::ostream& stream, const Mesh<VertexT>& mesh);

private:
    void read_header(std::istream& stream, WeshHeaderWrapper& header);
    void write_header(std::ostream& stream,
                      uint32_t n_vertices,
                      uint32_t n_indices,
                      uint32_t vertex_size);
    bool header_sanity_check(const WeshHeader& header, size_t vertex_size);
};


template<typename VertexT>
bool WeshLoader::read(std::istream& stream,
                      std::vector<VertexT>& vertices,
                      std::vector<uint32_t>& indices)
{
    // Check that input vectors are empty
    assert(vertices.size()==0);
    assert(indices.size()==0);

    // Read header
    WeshHeaderWrapper header;
    read_header(stream, header);

    if(!header_sanity_check(header.h, sizeof(VertexT)))
        return false;

    size_t vsize = header.h.n_vertices;
    size_t isize = header.h.n_indices;
    vertices.resize(vsize);
    indices.resize(isize);

    // Read vertex data
    stream.read(reinterpret_cast<char*>(&vertices[0]), vsize*sizeof(VertexT));
    // Read index data
    stream.read(reinterpret_cast<char*>(&indices[0]), isize*sizeof(uint32_t));

    return true;
}

template<typename VertexT>
void WeshLoader::write(std::ostream& stream,
                       const std::vector<VertexT>& vertices,
                       const std::vector<uint32_t>& indices)
{
    // Write header
    size_t vsize = vertices.size();
    size_t isize = indices.size();
    write_header(stream, vsize, isize, sizeof(VertexT));

    // Write vertex data
    stream.write(reinterpret_cast<const char*>(&vertices[0]), vsize*sizeof(VertexT));
    // Write index data
    stream.write(reinterpret_cast<const char*>(&indices[0]), isize*sizeof(uint32_t));
}

template<typename VertexT>
void WeshLoader::write(std::ostream& stream, const Mesh<VertexT>& mesh)
{
    write(stream,
          mesh.get_vertex_buffer(),
          mesh.get_index_buffer());
}


} // namespace wcore

#endif
