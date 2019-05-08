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
#include <GL/glew.h>

#include "wtypes.h"
#include "png_loader.h"

namespace wcore
{

#ifdef __TEXTURE_OLD__ // CURRENT TEXTURE REFACTORING

enum class TextureUnit: uint16_t;
struct TextureDescriptor;

class Shader;
class Texture
{
private:
    class TextureInternal;
    friend class Material;

    typedef std::shared_ptr<TextureInternal> pInternal;
    typedef std::shared_ptr<Texture> pTexture;
    typedef std::map<hash_t, pInternal> RMap;

    pInternal internal_;
    hash_t    resourceID_;
    uint16_t  units_;
    uint8_t   sampler_group_;
    std::vector<hash_t> uniform_sampler_names_;
    std::map<TextureUnit, uint32_t> unit_indices_;

    static const std::string TEX_IMAGE_PATH;
    static RMap RESOURCE_MAP_;   // TextureInternal cache
    static PngLoader png_loader_;

public:
    // Load texture from TextureDescriptor structure
    // Can be obtained from MaterialFactory
    Texture(const TextureDescriptor& descriptor);

    // Create single texture2D from stream with all default options
    // These cannot be cached
    Texture(std::istream& stream);

    // Create an empty texture, ideal for creating a render target for an FBO
    Texture(const std::vector<hash_t>& sampler_names,
            const std::vector<uint32_t>& filters,
            const std::vector<uint32_t>& internalFormats,
            const std::vector<uint32_t>& formats,
            uint32_t width   = 0,
            uint32_t height  = 0,
            bool clamp       = false,
            bool lazy_mipmap = true);

    ~Texture();

    void bind(GLuint unit = 0) const;
    void bind(GLuint unit, uint32_t index) const;
    void bind_all() const;
    void unbind() const;
    void bind_sampler(const Shader& shader, TextureUnit unit) const;
    void generate_mipmaps(uint32_t unit,
                          uint32_t base_level = 0,
                          uint32_t max_level = 3) const;


    inline uint32_t get_width()  const;
    inline uint32_t get_height() const;
    inline uint32_t get_num_units() const;
    inline GLenum get_texture_target() const;
    inline bool is_depth(uint32_t unit) const;
    inline GLuint operator[](uint32_t unit) const;

    inline hash_t get_sampler_name(uint32_t unit) const  { return uniform_sampler_names_.at(unit); }
    // Check if texture has a given unit (like albedo, normal map...)
    inline bool has_unit(TextureUnit unit) const         { return (units_&(uint16_t)unit); }

    bool operator==(const Texture& texture) const;
    bool operator!=(const Texture& texture) const;
};

class Texture::TextureInternal
{
public:
    friend class Texture;

    TextureInternal(const TextureDescriptor& descriptor);
    TextureInternal(std::istream& stream);

    TextureInternal(GLenum textureTarget,
                    uint32_t numTextures,
                    uint32_t width,
                    uint32_t height,
                    unsigned char** data,
                    GLenum* filters,
                    GLenum* internalFormat,
                    GLenum* format,
                    bool clamp,
                    bool lazy_mipmap = false);

    void bind(GLuint textureNum) const;

    virtual ~TextureInternal();

private:
    TextureInternal(TextureInternal& other) {}
    void operator=(TextureInternal& other) {}

    GLuint* textureID_;
    GLenum  textureTarget_;
    uint32_t numTextures_;
    uint32_t width_;
    uint32_t height_;
    uint32_t ID_;
    bool* is_depth_;

    static uint32_t Ninst;
};

inline uint32_t Texture::get_width()  const            { return internal_->width_; }
inline uint32_t Texture::get_height() const            { return internal_->height_; }
inline uint32_t Texture::get_num_units() const         { return internal_->numTextures_; }
inline GLenum Texture::get_texture_target() const      { return internal_->textureTarget_; }
inline bool Texture::is_depth(uint32_t unit) const     { return internal_->is_depth_[unit]; }
inline GLuint Texture::operator[](uint32_t unit) const { return internal_->textureID_[unit]; }

#else // __TEXTURE_OLD__

enum class TextureUnit: uint16_t;
struct TextureDescriptor;
class Shader;
class Texture
{
public:
    // Load texture from TextureDescriptor structure
    // Can be obtained from MaterialFactory
    Texture(const TextureDescriptor& descriptor);

    // Create single texture2D from stream with all default options
    // These cannot be cached
    Texture(std::istream& stream);

    // Create an empty texture, ideal for creating a render target for an FBO
    Texture(const std::vector<hash_t>& sampler_names,
            const std::vector<uint32_t>& filters,
            const std::vector<uint32_t>& internalFormats,
            const std::vector<uint32_t>& formats,
            uint32_t width   = 0,
            uint32_t height  = 0,
            bool clamp       = false,
            bool lazy_mipmap = true);

    ~Texture();

    // Send sampler uniforms to shader
    void bind_sampler(const Shader& shader, TextureUnit unit) const;
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

    // Get the number of texture units in this texture
    inline uint32_t get_num_units() const                { return n_units; }
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
    uint32_t n_units;       // Number of texture units in this texture
    uint32_t width_;        // Width of all texture units
    uint32_t height_;       // Height of all texture units
    uint32_t* texture_ids_; // Hold texture IDs generated by OpenGL
    uint16_t unit_flags_;   // Flag special units (albedo, normal, depth...) held in this texture
    uint8_t sampler_group_; // Sampler group index for splat-mapping

    std::vector<hash_t> uniform_sampler_names_;    // Sampler names used to bind each texture unit to a shader
    std::map<TextureUnit, uint32_t> unit_indices_; // Associate special texture units to sampler indices

#ifdef __DEBUG__
    std::vector<bool> is_depth_; // Retain information on whether texture units contain depth info or not
#endif
};


#endif // __TEXTURE_OLD__

}

#endif // TEXTURE_H
