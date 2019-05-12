#include <bitset>

#include "wat_loader.h"
#include "pixel_buffer.h"
#include "logger.h"

#define WAT_MAGIC 0x4C544157 // ASCII(WATL)
#define WAT_VERSION_MAJOR 1
#define WAT_VERSION_MINOR 0

namespace wcore
{

static inline bool has_unit(uint16_t flags, TextureUnit unit)
{
    return (flags&(uint16_t)unit);
}

void WatLoader::read(std::istream& stream, MaterialDescriptor& descriptor, bool read_texture_data)
{
    // * Read header
    WatHeaderWrapper header;
    read_header(stream, header);
    if(!header_sanity_check(header.h))
        return;

    bool has_block0 = (bool)header.h.has_block0;
    bool has_block1 = (bool)header.h.has_block1;
    bool has_block2 = (bool)header.h.has_block2;
    bool is_textured = has_block0 || has_block1 || has_block2;

    descriptor.texture_descriptor.owns_data   = read_texture_data;
    descriptor.texture_descriptor.is_wat      = true;
    descriptor.texture_descriptor.unit_flags  = header.h.unit_flags;
    descriptor.texture_descriptor.width       = header.h.width;
    descriptor.texture_descriptor.height      = header.h.height;
    descriptor.texture_descriptor.resource_id = header.h.unique_id;
    descriptor.parallax_height_scale          = header.h.parallax_height_scale;
    descriptor.has_transparency               = bool(header.h.has_transparency);
    descriptor.is_textured                    = is_textured;

    // * Read uniform data
    stream.read(reinterpret_cast<char*>(&descriptor.albedo), 3*sizeof(float));
    stream.read(reinterpret_cast<char*>(&descriptor.metallic), sizeof(float));
    stream.read(reinterpret_cast<char*>(&descriptor.roughness), sizeof(float));
    stream.read(reinterpret_cast<char*>(&descriptor.transparency), sizeof(float));

    // * Read texture data
    if(read_texture_data)
    {
        size_t block_size = size_t(header.h.width) * size_t(header.h.height) * 4;

        if(has_block0)
        {
            descriptor.texture_descriptor.block0_data = new unsigned char[block_size];
            stream.read(reinterpret_cast<char*>(descriptor.texture_descriptor.block0_data), block_size);
        }
        if(has_block1)
        {
            descriptor.texture_descriptor.block1_data = new unsigned char[block_size];
            stream.read(reinterpret_cast<char*>(descriptor.texture_descriptor.block1_data), block_size);
        }
        if(has_block2)
        {
            descriptor.texture_descriptor.block2_data = new unsigned char[block_size];
            stream.read(reinterpret_cast<char*>(descriptor.texture_descriptor.block2_data), block_size);
        }
    }
    else
    {
        bool has_normal = has_unit(header.h.unit_flags, TextureUnit::NORMAL);
        bool has_depth  = has_unit(header.h.unit_flags, TextureUnit::DEPTH);

        DLOGI("Unique id:  <v>" + std::to_string(header.h.unique_id) + "</v>",               "material");
        DLOGI("Min filter: <v>" + std::to_string(header.h.min_filter) + "</v>",              "material");
        DLOGI("Mag filter: <v>" + std::to_string(header.h.mag_filter) + "</v>",              "material");
        DLOGI("Address U:  <v>" + std::to_string(header.h.address_U) + "</v>",               "material");
        DLOGI("Address V:  <v>" + std::to_string(header.h.address_V) + "</v>",               "material");
        DLOGI("Width:      <v>" + std::to_string(header.h.width) + "</v>",                   "material");
        DLOGI("Height:     <v>" + std::to_string(header.h.height) + "</v>",                  "material");
        DLOGI("Units:      <v>" + std::bitset<16>(header.h.unit_flags).to_string() + "</v>", "material");
        DLOGI("Has Block0: <v>" + std::to_string(bool(header.h.has_block0)) + "</v>",        "material");
        DLOGI("Has Block1: <v>" + std::to_string(bool(header.h.has_block1)) + "</v>",        "material");
        DLOGI("Has Block2: <v>" + std::to_string(bool(header.h.has_block2)) + "</v>",        "material");
        DLOGI("Has alpha:  <v>" + std::to_string(bool(header.h.has_transparency)) + "</v>",  "material");
        DLOGI("Normal map: " + (has_normal?std::string("<g>yes</g>"):"<b>no</b>"),           "material");
        DLOGI("Depth map:  " + (has_depth?std::string("<g>yes</g>"):"<b>no</b>"),            "material");
        DLOGI("Parallax:   <v>" + std::to_string(header.h.parallax_height_scale) + "</v>",   "material");
    }
}

void WatLoader::write(std::ostream& stream, const MaterialDescriptor& descriptor)
{
    // * Write header
    bool has_block0 = descriptor.texture_descriptor.has_unit(TextureUnit::ALBEDO);
    bool has_block1 = descriptor.texture_descriptor.has_unit(TextureUnit::NORMAL)
                   || descriptor.texture_descriptor.has_unit(TextureUnit::DEPTH);
    bool has_block2 = descriptor.texture_descriptor.has_unit(TextureUnit::METALLIC)
                   || descriptor.texture_descriptor.has_unit(TextureUnit::AO)
                   || descriptor.texture_descriptor.has_unit(TextureUnit::ROUGHNESS);

    WatHeaderWrapper header;
    header.h.unique_id             = descriptor.texture_descriptor.resource_id;
    header.h.min_filter            = 0;
    header.h.mag_filter            = 0;
    header.h.address_U             = 0;
    header.h.address_V             = 0;
    header.h.width                 = descriptor.texture_descriptor.width;
    header.h.height                = descriptor.texture_descriptor.height;
    header.h.unit_flags            = descriptor.texture_descriptor.unit_flags;
    header.h.has_block0            = (uint8_t) has_block0;
    header.h.has_block1            = (uint8_t) has_block1;
    header.h.has_block2            = (uint8_t) has_block2;
    header.h.has_transparency      = (uint8_t) descriptor.has_transparency;
    header.h.parallax_height_scale = descriptor.parallax_height_scale;

    write_header(stream, header);

    // * Write uniform data
    stream.write(reinterpret_cast<const char*>(&descriptor.albedo), 3*sizeof(float));
    stream.write(reinterpret_cast<const char*>(&descriptor.metallic), sizeof(float));
    stream.write(reinterpret_cast<const char*>(&descriptor.roughness), sizeof(float));
    stream.write(reinterpret_cast<const char*>(&descriptor.transparency), sizeof(float));

    // * Write texture data
    size_t block_size = size_t(descriptor.texture_descriptor.width) * size_t(descriptor.texture_descriptor.height) * 4;

    if(has_block0)
        stream.write(reinterpret_cast<const char*>(descriptor.texture_descriptor.block0_data), block_size);
    if(has_block1)
        stream.write(reinterpret_cast<const char*>(descriptor.texture_descriptor.block1_data), block_size);
    if(has_block2)
        stream.write(reinterpret_cast<const char*>(descriptor.texture_descriptor.block2_data), block_size);
}

void WatLoader::read_header(std::istream& stream, WatHeaderWrapper& header)
{
    stream.read(reinterpret_cast<char*>(&header), sizeof(header));
}

void WatLoader::write_header(std::ostream& stream, WatHeaderWrapper& header)
{
    header.h.magic         = WAT_MAGIC;
    header.h.version_major = WAT_VERSION_MAJOR;
    header.h.version_minor = WAT_VERSION_MINOR;

    stream.write(reinterpret_cast<const char*>(&header), sizeof(header));
}

bool WatLoader::header_sanity_check(const WatHeader& header)
{
    // Check magic bytes
    if(header.magic != WAT_MAGIC)
    {
        DLOGE("[Wat] Not a valid Wat file.", "parsing");
        DLOGI("Magic bytes should be " + std::to_string(WAT_MAGIC) + " but got " + std::to_string(header.magic), "parsing");
        return false;
    }

    // Check version for compatibility
    bool version_ok = header.version_major == WAT_VERSION_MAJOR
                   && header.version_minor == WAT_VERSION_MINOR;
    if(!version_ok)
    {
        DLOGW("[Wat] Version mismatch.", "parsing");
        DLOGI("Required version: " + std::to_string(WAT_VERSION_MAJOR) + "."
                                   + std::to_string(WAT_VERSION_MINOR), "parsing");
        DLOGI("Got: " + std::to_string(header.version_major) + "."
                      + std::to_string(header.version_minor), "parsing");
        return false;
    }

    return true;
}





} // namespace wcore
