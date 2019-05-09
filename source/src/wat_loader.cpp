#include "wat_loader.h"
#include "pixel_buffer.h"
#include "logger.h"

#define WAT_MAGIC 0x4C544157 // ASCII(WATL)
#define WAT_VERSION_MAJOR 1
#define WAT_VERSION_MINOR 0

namespace wcore
{

Material* WatLoader::read(std::istream& stream)
{
    return nullptr;
}

static inline bool has_unit(uint16_t flags, TextureUnit unit)
{
    return (flags&(uint16_t)unit);
}

void WatLoader::write(std::ostream& stream, const MaterialInfo& mat_info)
{
    // * Write header



    /*bool has_block0 = mat_info.has_albedo;
    bool has_block1 = mat_info.has_normal || mat_info.has_depth;
    bool has_block2 = mat_info.has_metal || mat_info.has_AO || mat_info.has_rough;*/
    bool has_block0 = has_unit(mat_info.unit_flags, TextureUnit::ALBEDO);
    bool has_block1 = has_unit(mat_info.unit_flags, TextureUnit::NORMAL)
                   || has_unit(mat_info.unit_flags, TextureUnit::DEPTH);
    bool has_block2 = has_unit(mat_info.unit_flags, TextureUnit::METALLIC)
                   || has_unit(mat_info.unit_flags, TextureUnit::AO)
                   || has_unit(mat_info.unit_flags, TextureUnit::ROUGHNESS);

    WatHeaderWrapper header;
    header.h.unique_id             = mat_info.unique_id;
    header.h.min_filter            = 0;
    header.h.mag_filter            = 0;
    header.h.address_U             = 0;
    header.h.address_V             = 0;
    header.h.width                 = mat_info.width;
    header.h.height                = mat_info.height;
    header.h.unit_flags            = mat_info.unit_flags;
    header.h.has_block0            = (uint8_t) has_block0;
    header.h.has_block1            = (uint8_t) has_block1;
    header.h.has_block2            = (uint8_t) has_block2;
    header.h.has_transparency      = (uint8_t) mat_info.has_transparency;
    header.h.parallax_height_scale = mat_info.u_parallax_scale;

    write_header(stream, header);

    // * Write uniform data
    stream.write(reinterpret_cast<const char*>(&mat_info.u_albedo), 4*sizeof(float));
    stream.write(reinterpret_cast<const char*>(&mat_info.u_metal), sizeof(float));
    stream.write(reinterpret_cast<const char*>(&mat_info.u_rough), sizeof(float));
    stream.write(reinterpret_cast<const char*>(&mat_info.u_alpha), sizeof(float));

    // * Write texture data
    if(has_block0)
        stream.write(reinterpret_cast<const char*>(mat_info.block0_data), mat_info.width*mat_info.height*4);
    if(has_block1)
        stream.write(reinterpret_cast<const char*>(mat_info.block1_data), mat_info.width*mat_info.height*4);
    if(has_block2)
        stream.write(reinterpret_cast<const char*>(mat_info.block2_data), mat_info.width*mat_info.height*4);
}

void WatLoader::read_descriptor(std::istream& stream, MaterialDescriptor& descriptor)
{
    descriptor.is_wat = true;

    WatHeaderWrapper header;
    read_header(stream, header);
    if(!header_sanity_check(header.h))
        return;

    descriptor.has_transparency = header.h.has_transparency;

    bool has_block0 = (bool)header.h.has_block0;
    bool has_block1 = (bool)header.h.has_block1;
    bool has_block2 = (bool)header.h.has_block2;
    bool is_textured = has_block0 || has_block1 || has_block2;
    descriptor.is_textured = is_textured;
    descriptor.texture_descriptor.units = header.h.unit_flags;

    // * Read uniform data
    math::i32vec4 u_albedo;
    stream.read(reinterpret_cast<char*>(&u_albedo), 4*sizeof(float));
    stream.read(reinterpret_cast<char*>(&descriptor.metallic), sizeof(float));
    stream.read(reinterpret_cast<char*>(&descriptor.roughness), sizeof(float));
    stream.read(reinterpret_cast<char*>(&descriptor.transparency), sizeof(float));

    descriptor.albedo = math::vec3(u_albedo.x() / 255.f,
                                   u_albedo.y() / 255.f,
                                   u_albedo.z() / 255.f);
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
