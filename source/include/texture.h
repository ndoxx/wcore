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
    typedef std::weak_ptr<Texture> wpTexture;

    typedef std::map<hash_t, pInternal> RMap;
    typedef std::map<hash_t, pTexture> TMap;
    typedef std::map<hash_t, std::vector<std::string>> AMap;

    pInternal internal_;
    hash_t    resourceID_;
    uint16_t  units_;
    uint8_t   sampler_group_;
    std::vector<hash_t> uniform_sampler_names_;
    std::map<TextureUnit, uint32_t> unit_indices_;

    static RMap RESOURCE_MAP_;   // TextureInternal cache
    static PngLoader png_loader_;

public:
    static const std::string TEX_IMAGE_PATH;
    static std::map<TextureUnit, hash_t> SAMPLER_NAMES_;
    static std::map<TextureUnit, hash_t> SAMPLER_NAMES_2_; // Sampler names for alt-materials used in splat-mapping

    // Load texture from TextureDescriptor structure
    // Can be obtained from MaterialFactory
    Texture(const TextureDescriptor& descriptor);

    // Create single texture2D from stream with all default options
    // These cannot be cached
    Texture(std::istream& stream);

    // Create an empty texture, ideal for creating a render target for an FBO
    // Init all units with same filter and format parameters
    Texture(const std::vector<hash_t>& sampler_names,
            uint32_t width         = 0,
            uint32_t height        = 0,
            GLenum textureTarget   = GL_TEXTURE_2D,
            GLenum filter          = GL_LINEAR,
            GLenum internalFormat  = GL_RGBA,
            GLenum format          = GL_RGBA,
            bool clamp             = false,
            bool lazy_mipmap       = true);

    Texture(const std::vector<hash_t>& sampler_names,
            const std::vector<GLenum>& filters,
            const std::vector<GLenum>& internalFormats,
            const std::vector<GLenum>& formats,
            uint32_t width         = 0,
            uint32_t height        = 0,
            GLenum textureTarget   = GL_TEXTURE_2D,
            bool clamp             = false,
            bool lazy_mipmap       = true);

    ~Texture();

    void bind(GLuint unit = 0) const;
    void bind(GLuint unit, uint32_t index) const;
    void bind_all() const;
    void unbind() const;
    void generate_mipmaps(uint32_t unit,
                          uint32_t base_level = 0,
                          uint32_t max_level = 3) const;

    inline uint32_t get_width()  const;
    inline uint32_t get_height() const;
    inline uint32_t get_num_units() const;
    inline GLenum get_texture_target() const;
    inline hash_t get_sampler_name(uint32_t index) const { return uniform_sampler_names_.at(index); }
    inline bool is_depth(uint32_t ii) const;
    inline GLuint operator[](uint32_t index) const;

    void bind_sampler(const Shader& shader, TextureUnit unit) const;

    // Check if texture has a given unit (like albedo, normal map...)
    inline bool has_unit(TextureUnit unit) const { return (units_&(uint16_t)unit); }

    static const std::map<TextureUnit, hash_t>& select_sampler_group(uint8_t group);

    bool operator==(const Texture& texture) const;
    bool operator!=(const Texture& texture) const;

#ifdef __DEBUG__
    // For all cached textures, print their current binding state (id, active texture)
    // Active texture is -1 if unbound
    static void debug_print_rmap_bindings();
#endif

private:
    inline uint32_t get_unit_index(TextureUnit unit) const { return unit_indices_.at(unit); }
    inline hash_t unit_to_sampler_name(TextureUnit unit) const
    {
        return select_sampler_group(sampler_group_).at(unit);
    }
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

    inline bool is_depth(uint32_t index)
    {
        assert(index<numTextures_
               && "[TextureInternal.is_depth()] index out of bounds.");
        return is_depth_[index];
    }
    inline uint32_t get_width()  const       { return width_; }
    inline uint32_t get_height() const       { return height_; }
    inline uint32_t ID() const      { return ID_; }
    inline uint32_t get_num_textures() const { return numTextures_; }
    inline GLenum get_texture_target() const { return textureTarget_; }

    virtual ~TextureInternal();

#ifdef __DEBUG__
    inline void set_binding_state(GLuint texID, int activeTexIndex)
    {
        binding_states_[texID] = activeTexIndex;
    }
    inline const auto& get_binding_states() const
    {
        return binding_states_;
    }
#endif

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

#ifdef __DEBUG__
    // Map of texture binding states for debug purposes
    std::map<GLuint, int> binding_states_;
#endif
};

inline uint32_t Texture::get_width()  const { return internal_->get_width(); }
inline uint32_t Texture::get_height() const { return internal_->get_height(); }
inline uint32_t Texture::get_num_units() const { return internal_->get_num_textures(); }
inline GLenum Texture::get_texture_target() const { return internal_->get_texture_target(); }
inline bool Texture::is_depth(uint32_t ii) const
{
    return internal_->is_depth(ii);
}
inline GLuint Texture::operator[](uint32_t index) const { return internal_->textureID_[index]; }




#else // __TEXTURE_OLD__

enum class TextureUnit: uint16_t;
class Shader;
class Texture
{
public:
    // Create single texture2D from stream with all default options
    // These cannot be cached
    Texture(std::istream& stream);

    // Send sampler uniforms to shader
    void bind_sampler(const Shader& shader, TextureUnit unit) const;

    // Get the number of texture units in this texture
    inline uint32_t get_num_units() const               { return n_units; }
    // Get texture units width
    inline uint32_t get_width() const                   { return width_; }
    // Get texture units height
    inline uint32_t get_height() const                  { return height_; }
    // Get the sampler name associated to a given texture unit
    inline hash_t get_sampler_name(uint32_t unit) const { return uniform_sampler_names_.at(unit); }
    // Get OpenGL texture index for given unit
    inline uint32_t operator[](uint32_t unit) const     { return units_[unit]; }


private:
    uint32_t n_units; // Number of texture units in this texture
    uint32_t width_;  // Width of all texture units
    uint32_t height_; // Height of all texture units
    std::vector<hash_t> uniform_sampler_names_; // Sampler names used to bind each texture unit to a shader

    uint32_t* units_;
};


#endif // __TEXTURE_OLD__

}

#endif // TEXTURE_H
