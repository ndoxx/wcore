#ifndef TEXTURE_H
#define TEXTURE_H

#include <memory>
#include <cassert>
#include <istream>
#include <unordered_map>
#include <functional>
#include <vector>
#include <map>
#include <string>

#include "wtypes.h"

namespace wcore
{

enum TextureFilter: uint8_t
{
    MAG_NEAREST = 0,
    MAG_LINEAR  = 1,

    MIN_NEAREST = 2,
    MIN_LINEAR  = 4,
    MIN_NEAREST_MIPMAP_NEAREST = 8,
    MIN_LINEAR_MIPMAP_NEAREST  = 16,
    MIN_NEAREST_MIPMAP_LINEAR  = 32,
    MIN_LINEAR_MIPMAP_LINEAR   = 64
};

enum class TextureWrap: uint8_t
{
    REPEAT,
    MIRRORED_REPEAT,
    CLAMP_TO_EDGE
};

// Internal formats
enum class TextureIF: uint8_t
{
    R8,
    RGB8,
    RGBA8,
    RGB16F,
    RGBA16F,
    SRGB_ALPHA,
    RG16_SNORM,
    RGB16_SNORM,
    RGBA16_SNORM,
    COMPRESSED_RGB_S3TC_DXT1,
    COMPRESSED_RGBA_S3TC_DXT1,
    COMPRESSED_RGBA_S3TC_DXT3,
    COMPRESSED_RGBA_S3TC_DXT5,
    COMPRESSED_SRGB_S3TC_DXT1,
    COMPRESSED_SRGB_ALPHA_S3TC_DXT1,
    COMPRESSED_SRGB_ALPHA_S3TC_DXT3,
    COMPRESSED_SRGB_ALPHA_S3TC_DXT5,
    DEPTH_COMPONENT16,
    DEPTH_COMPONENT24,
    DEPTH_COMPONENT32F,
    DEPTH24_STENCIL8,
    DEPTH32F_STENCIL8,
};

// Formats
enum class TextureF: uint8_t
{
    RED,
    RGB,
    RGBA,
    DEPTH_COMPONENT,
    DEPTH_STENCIL,
};

struct TextureParameters
{
    TextureFilter filter;
    uint32_t internal_format;
    uint32_t format;
    TextureWrap wrap;
    bool lazy_mipmap;

    TextureParameters();
};

enum class TextureUnit: uint16_t
{
    ALBEDO    = 1,
    AO        = 2,
    DEPTH     = 4,
    METALLIC  = 8,
    NORMAL    = 16,
    ROUGHNESS = 32
};

enum class TextureBlock: uint16_t
{
    BLOCK0 = uint16_t(TextureUnit::ALBEDO),
    BLOCK1 = uint16_t(TextureUnit::NORMAL)
           | uint16_t(TextureUnit::DEPTH),
    BLOCK2 = uint16_t(TextureUnit::METALLIC)
           | uint16_t(TextureUnit::AO)
           | uint16_t(TextureUnit::ROUGHNESS)
};

struct TextureDescriptor
{
    typedef std::map<TextureBlock, std::string> TexMap;

    // Image texture file names by map type
    TexMap locations;
    // Flags for each unit
    uint16_t unit_flags;
    // Sampler group number
    uint8_t sampler_group;
    // OpenGL texture parameters
    TextureParameters parameters;
    // Unique id
    hash_t resource_id;

    // Wat format
    bool is_wat;
    bool owns_data;
    std::string wat_location;
    uint32_t width;
    uint32_t height;
    unsigned char* block0_data; // Pointers to pixel data for block 0
    unsigned char* block1_data; // Pointers to pixel data for block 1
    unsigned char* block2_data; // Pointers to pixel data for block 2

    TextureDescriptor();
    ~TextureDescriptor();

    inline bool has_block(TextureBlock block) const { return (unit_flags&(uint16_t)block); }
    inline bool has_unit(TextureUnit unit) const    { return (unit_flags&(uint16_t)unit); }
    inline void add_unit(TextureUnit unit)          { unit_flags |= (uint16_t)unit; }
    void release_data();
};

class TextureUnitInfo
{
public:
    friend class Texture;

    TextureUnitInfo(hash_t sampler_name,
                    TextureFilter filter,
                    uint32_t internal_format,
                    uint32_t format,
                    unsigned char* data = nullptr);

protected:
#ifdef __DEBUG__
    TextureUnitInfo(hash_t sampler_name,
                    uint32_t texture_id,
                    bool is_depth);
#else
    TextureUnitInfo(hash_t sampler_name,
                    uint32_t texture_id);
#endif

protected:
    hash_t sampler_name_;
    TextureFilter filter_;
    uint32_t internal_format_;
    uint32_t format_;
    unsigned char* data_;

    bool is_shared_;
    uint32_t texture_id_;
#ifdef __DEBUG__
    bool is_depth_;
#endif
};

struct MaterialInfo;
class Shader;
class Texture
{
public:
    // Load texture from TextureDescriptor structure
    // Inside a MaterialDescriptor obtained from MaterialFactory
    Texture(const TextureDescriptor& descriptor);

    // Create single texture2D from stream with all default options
    // Used only for splatmap loading atm.
    Texture(std::istream& stream);

    // Create an empty texture, ideal for creating a render target for an FBO
    Texture(std::initializer_list<TextureUnitInfo> units,
            uint32_t width   = 0,
            uint32_t height  = 0,
            TextureWrap wrap_param = TextureWrap::REPEAT,
            bool lazy_mipmap = true);

    ~Texture();

    // Send sampler uniforms to shader
    void bind_sampler(const Shader& shader, TextureBlock unit) const;
    // Bind each texture unit in order
    void bind_all() const;
    // Bind texture at a given index to sampler specified by unit
    void bind(uint32_t unit=0, uint32_t index=0) const;
    // Unbind all textures
    void unbind() const;

    // Generate mipmaps for texture at given index, specifying minimum and maximum levels
    void generate_mipmaps(uint32_t index,
                          uint32_t base_level = 0,
                          uint32_t max_level = 3) const;

    // Get structure that allows texture unit sharing between multiple Textures
    TextureUnitInfo share_unit(uint32_t index);

    // Get the number of texture units in this texture
    inline uint32_t get_num_units() const                { return n_units_; }
    // Get texture units width
    inline uint32_t get_width() const                    { return width_; }
    // Get texture units height
    inline uint32_t get_height() const                   { return height_; }
    // Get the sampler name associated to a given texture unit
    inline hash_t get_sampler_name(uint32_t index) const { return uniform_sampler_names_.at(index); }
    // Check if texture has a given special unit (like albedo, normal map...)
    inline bool has_unit(TextureUnit unit) const         { return (unit_flags_&(uint16_t)unit); }
    // Get OpenGL texture index for given unit
    inline uint32_t operator[](uint32_t index) const     { return texture_ids_[index]; }

#ifdef __DEBUG__
    // Check if texture at given index is a depth map
    inline bool is_depth(uint32_t index) const           { return is_depth_[index]; }
#endif

private:
    // Generic helper function to generate a texture unit inside this texture
    void generate_texture_unit(const TextureUnitInfo& unit_info,
                               TextureWrap wrap_param,
                               bool lazy_mipmap = false);

private:
    uint32_t n_units_;      // Number of texture units in this texture
    uint32_t width_;        // Width of all texture units
    uint32_t height_;       // Height of all texture units
    uint16_t unit_flags_;   // Flag special units (albedo, normal, depth...) held in this texture
    uint8_t sampler_group_; // Sampler group index for splat-mapping

    std::vector<uint32_t> texture_ids_; // Hold texture IDs generated by OpenGL
    std::vector<hash_t> uniform_sampler_names_;         // Sampler names used to bind each texture block to a shader
    std::map<TextureBlock, uint32_t> block_to_sampler_; // Associate texture blocks to sampler indices

#ifdef __DEBUG__
    std::vector<bool> is_depth_; // Retain information on whether texture units contain depth info or not
#endif
};

}

#endif // TEXTURE_H
