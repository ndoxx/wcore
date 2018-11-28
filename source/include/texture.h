#ifndef TEXTURE_H
#define TEXTURE_H

#include <memory>
#include <cassert>
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

enum class TextureUnit: uint16_t;
struct TextureDescriptor;

class Texture
{
private:
    class TextureInternal;
    friend class Material;

    typedef std::shared_ptr<TextureInternal> pInternal;
    typedef std::shared_ptr<Texture> pTexture;
    typedef std::weak_ptr<Texture> wpTexture;

#ifdef __PRESERVE_STRS__
    typedef std::unordered_map<hash_t, pInternal> RMap;
    typedef std::unordered_map<hash_t, pTexture> TMap;
    typedef std::unordered_map<hash_t, std::vector<std::string>> AMap;
#else
    typedef std::map<hash_t, pInternal> RMap;
    typedef std::map<hash_t, pTexture> TMap;
    typedef std::map<hash_t, std::vector<std::string>> AMap;
#endif

    pInternal internal_;
    hash_t    resourceID_;
    uint16_t  units_;
    std::vector<hash_t> uniform_sampler_names_;
    std::map<TextureUnit, uint32_t> unit_indices_;

    static RMap RESOURCE_MAP_;   // TextureInternal cache
    static TMap NAMED_TEXTURES_; // Holds pointers to named textures
    static std::map<TextureUnit, hash_t> SAMPLER_NAMES_;
    static PngLoader png_loader_;

public:
    static const std::string TEX_IMAGE_PATH;

    // Load texture from TextureDescriptor structure
    // Can be obtained from MaterialFactory
    Texture(const TextureDescriptor& descriptor);

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

    Texture(const Texture& texture);

    Texture(Texture&& texture);

    ~Texture();

    // Register texture in the named texture map (DON'T ABUSE, just for special
    // textures like those with a render attachment)
    static void register_named_texture(hash_t name, pTexture ptex);
    // Get a weak pointer to named texture
    static wpTexture get_named_texture(hash_t name);

    void bind(GLuint unit = 0) const;
    void bind(GLuint unit, uint32_t index) const;
    void bind_all() const;
    void unbind() const;
    void generate_mipmaps(uint32_t unit,
                          uint32_t base_level = 0,
                          uint32_t max_level = 3) const;

    inline uint32_t get_width()  const;
    inline uint32_t get_height() const;
    inline uint32_t get_num_textures() const;
    inline GLenum get_texture_target() const;
    inline hash_t get_sampler_name(uint32_t index) const { return uniform_sampler_names_.at(index); }
    inline bool is_depth(uint32_t ii) const;
    inline GLuint get_texture_id(uint32_t index) const;
    inline GLuint operator[](uint32_t index) const;

    // Check if texture has a given unit (like albedo, normal map...)
    inline bool has_unit(TextureUnit unit) const { return (units_&(uint16_t)unit); }

    inline uint32_t get_unit_index(TextureUnit unit) const { return unit_indices_.at(unit); }
    static inline hash_t unit_to_sampler_name(TextureUnit unit) { return SAMPLER_NAMES_.at(unit); }


    bool operator==(const Texture& texture) const;
    bool operator!=(const Texture& texture) const;

#ifdef __DEBUG__
    // For all cached textures, print their current binding state (id, active texture)
    // Active texture is -1 if unbound
    static void debug_print_rmap_bindings();
#endif
};

class Texture::TextureInternal
{
public:
    TextureInternal(const TextureDescriptor& descriptor);

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
    inline GLuint get_texture_id(uint32_t index) const { return textureID_[index]; }

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
inline uint32_t Texture::get_num_textures() const { return internal_->get_num_textures(); }
inline GLenum Texture::get_texture_target() const { return internal_->get_texture_target(); }
inline bool Texture::is_depth(uint32_t ii) const
{
    return internal_->is_depth(ii);
}
inline GLuint Texture::get_texture_id(uint32_t index) const { return internal_->get_texture_id(index); }
inline GLuint Texture::operator[](uint32_t index) const { return internal_->get_texture_id(index); }

}

#endif // TEXTURE_H
