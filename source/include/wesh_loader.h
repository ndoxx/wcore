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

namespace fs = std::filesystem;

namespace wcore
{

template <typename VertexT> class Mesh;
struct Vertex3P3N3T2U;
struct VertexAnim;
using AnimMesh = Mesh<VertexAnim>;
using SurfaceMesh = Mesh<Vertex3P3N3T2U>;

//#pragma pack(push,1)
struct WeshHeader
{
    uint32_t magic;
    uint8_t version_major;
    uint8_t version_minor;
    uint16_t is_animated;
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
    // Animated mesh
    bool load(std::istream& stream,
              std::vector<VertexAnim>& vertices,
              std::vector<uint32_t>& indices);
    void write(std::ostream& stream,
               const std::vector<VertexAnim>& vertices,
               const std::vector<uint32_t>& indices);
    void write(std::ostream& stream, const AnimMesh& mesh);

    // Static mesh
    bool load(std::istream& stream,
              std::vector<Vertex3P3N3T2U>& vertices,
              std::vector<uint32_t>& indices);
    void write(std::ostream& stream,
               const std::vector<Vertex3P3N3T2U>& vertices,
               const std::vector<uint32_t>& indices);
    void write(std::ostream& stream, const SurfaceMesh& mesh);

private:
    bool read_header(std::istream& stream, WeshHeaderWrapper& header);
    bool write_header(std::ostream& stream,
                      uint32_t n_vertices,
                      uint32_t n_indices,
                      bool is_animated);
};

} // namespace wcore

#endif
