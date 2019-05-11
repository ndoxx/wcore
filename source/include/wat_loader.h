#ifndef WAT_LOADER_H
#define WAT_LOADER_H

/*
    WatLoader class can read and write .wat files. These binary files
    contain material information. They can encapsulate texture data as well
    as uniform data. Color channels for texture data is interleaved.

    Texture maps are layed out in three blocks:
    [   Block0   ]  [     Block1   ]  [        Block2       ]
    [[R][G][B][A]]  [[R][G][B]  [A]]  [[R]  [G] [B]      [A]]
      Albedo           Normal  Depth  Metal AO  Rough  UNUSED
*/

#include <filesystem>
#include <fstream>
#include <memory>
#include <cassert>

#include "math3d.h"
#include "material_common.h"

namespace fs = std::filesystem;

namespace wcore
{

//#pragma pack(push,1)
struct WatHeader
{
    uint32_t magic;
    uint16_t version_major;
    uint16_t version_minor;
    uint64_t unique_id;
    uint16_t min_filter;
    uint16_t mag_filter;
    uint16_t address_U;
    uint16_t address_V;
    uint16_t width;
    uint16_t height;
    uint16_t unit_flags;
    uint8_t  has_block0;
    uint8_t  has_block1;
    uint8_t  has_block2;
    uint8_t  has_transparency;
    float    parallax_height_scale;
};
//#pragma pack(pop)

#define WAT_HEADER_SIZE 128
typedef union
{
    struct WatHeader h;
    uint8_t padding[WAT_HEADER_SIZE];
} WatHeaderWrapper;

class Texture;
class WatLoader
{
public:
    void read(std::istream& stream, MaterialDescriptor& descriptor, bool read_texture_data);
    void write(std::ostream& stream, const MaterialDescriptor& descriptor);

private:
    void read_header(std::istream& stream, WatHeaderWrapper& header);
    void write_header(std::ostream& stream, WatHeaderWrapper& header);
    bool header_sanity_check(const WatHeader& header);
};

} // namespace wcore

#endif
