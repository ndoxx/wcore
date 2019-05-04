#ifndef WESH_LOADER_H
#define WESH_LOADER_H

/*
    WeshLoader class can read and write .wesh files. These binary files
    contain mesh information, data is stored in an interleaved fashion.
    TMP: For the moment, this format only supports animated meshes with underlying
    VertexAnim vertex format.

    As of version 1.0 data is layed out like this:
    [HEADER]              -> 128 bytes, padded
    [size_t vsize]        -> number of vertices
    [array of VertexAnim] -> vertex buffer content of size vsize
    [size_t isize]        -> number of indices
    [array of uint32_t]   -> index buffer content of size isize
*/

#include <filesystem>
#include <fstream>
#include <memory>

namespace fs = std::filesystem;

namespace wcore
{

template <typename VertexT> class Mesh;
struct VertexAnim;
using AnimMesh = Mesh<VertexAnim>;

//#pragma pack(push,1)
struct WeshHeader
{
    uint32_t magic;
    uint8_t version_major;
    uint8_t version_minor;
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
    std::shared_ptr<AnimMesh> load(std::istream& stream);
    void write(std::ostream& stream, const AnimMesh& mesh);
    void write(std::ostream& stream,
               const std::vector<VertexAnim>& vertices,
               const std::vector<uint32_t>& indices);

private:
    bool read_header(std::istream& stream, WeshHeaderWrapper& header);
    bool write_header(std::ostream& stream);
};

} // namespace wcore

#endif
