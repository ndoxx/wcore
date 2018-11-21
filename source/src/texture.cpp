#include <cassert>
#include <stdexcept>
#include <filesystem>
#include <vector>
namespace fs = std::filesystem;

#include "texture.h"
#include "png_loader.h"
#include "pixel_buffer.h"
#include "logger.h"
#include "algorithms.h"
#include "material_common.h"

uint32_t Texture::TextureInternal::Ninst = 0;
Texture::RMap Texture::RESOURCE_MAP_;
Texture::TMap Texture::NAMED_TEXTURES_;
const std::string Texture::TEX_IMAGE_PATH("../res/textures/");

std::map<TextureUnit, hash_t> Texture::SAMPLER_NAMES_ =
{
    {TextureUnit::ALBEDO,    H_("mt.albedoTex")},
    {TextureUnit::AO,        H_("mt.AOTex")},
    {TextureUnit::DEPTH,     H_("mt.depthTex")},
    {TextureUnit::METALLIC,  H_("mt.metallicTex")},
    {TextureUnit::NORMAL,    H_("mt.normalTex")},
    {TextureUnit::ROUGHNESS, H_("mt.roughnessTex")}
};

static std::map<GLenum, GLenum> DATA_TYPES =
{
    {GL_SRGB_ALPHA, GL_FLOAT},
    {GL_RGB16F, GL_FLOAT},
    {GL_RGBA16F, GL_FLOAT},
    {GL_RG16_SNORM, GL_UNSIGNED_BYTE},
    {GL_RGB16_SNORM, GL_UNSIGNED_BYTE},
    {GL_DEPTH32F_STENCIL8, GL_FLOAT_32_UNSIGNED_INT_24_8_REV},
    {GL_DEPTH24_STENCIL8, GL_UNSIGNED_INT_24_8},
};

static GLenum internal_format_to_data_type(GLenum iformat)
{
    auto it = DATA_TYPES.find(iformat);
    if(it!=DATA_TYPES.end())
        return it->second;
    return GL_UNSIGNED_BYTE;
}

static bool handle_filter(GLenum filter, GLenum target)
{
    bool has_mipmap = (filter == GL_NEAREST_MIPMAP_NEAREST ||
                       filter == GL_NEAREST_MIPMAP_LINEAR ||
                       filter == GL_LINEAR_MIPMAP_NEAREST ||
                       filter == GL_LINEAR_MIPMAP_LINEAR);

    // Set filter
    if(filter != GL_NONE)
    {
        glTexParameterf(target, GL_TEXTURE_MIN_FILTER, filter);
        if(!has_mipmap)
            glTexParameterf(target, GL_TEXTURE_MAG_FILTER, filter);
        else
            glTexParameterf(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    return has_mipmap;
}

// TMP
static void handle_addressUV(bool clamp, GLenum target)
{
    if(clamp)
    {
        glTexParameterf(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
}

static void handle_mipmap(bool bound_tex_has_mipmap, GLenum target)
{
    if(bound_tex_has_mipmap)
    {
        glGenerateMipmap(target);
        GLfloat maxAnisotropy;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
        glTexParameterf(target,
                        GL_TEXTURE_MAX_ANISOTROPY_EXT,
                        math::clamp(0.0f, 8.0f, maxAnisotropy));
    }
    else
    {
        glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, 0);
    }
}

#ifdef __DEBUG_TEXTURE__
void Texture::debug_print_rmap_bindings()
{
    DLOG("[Texture] Displaying binding states: ");
    for(auto&& [hash, pinternal]: RESOURCE_MAP_)
    {
        auto && states = pinternal->get_binding_states();
        std::cout << hash << " :" << std::endl;
        for(auto&& [texID, activeTexID]: states)
            std::cout << "  -> id: " << texID << " active: " << activeTexID << std::endl;
    }
}
#endif

void Texture::register_named_texture(hash_t name, pTexture ptex)
{
    #if __DEBUG_TEXTURE_VERBOSE__
    {
        std::stringstream ss;
        ss << "[Texture] Registering new named texture: "
           << "<n>" << name << "</n>";
        DLOGN(ss.str());
        DLOGI("width:  <v>" + std::to_string(ptex->get_width()) + "</v>");
        DLOGI("height: <v>" + std::to_string(ptex->get_height()) + "</v>");
        DLOGI("units:  <v>" + std::to_string(ptex->get_num_textures()) + "</v>");

    }
    #endif
    auto it = NAMED_TEXTURES_.find(name);
    if(it == NAMED_TEXTURES_.end())
    {
        NAMED_TEXTURES_[name] = ptex;
    }
    else
    {
        #if __DEBUG_TEXTURE_VERBOSE__
            std::stringstream ss;
            ss << "[Texture] Ignored duplicate named texture registration for: <n>" << name << "</n>";
            DLOGW(ss.str());
        #endif
    }
}

Texture::wpTexture Texture::get_named_texture(hash_t name)
{
    auto it = NAMED_TEXTURES_.find(name);
    if(it == NAMED_TEXTURES_.end())
    {
        std::stringstream ss;
        ss << "[Texture] Couldn't find named texture: <n>" << name << "</n>";
        DLOGF(ss.str());
        throw std::runtime_error("Couldn't find named texture.");
    }
    else
        return wpTexture(it->second);
}

Texture::TextureInternal::TextureInternal(const TextureDescriptor& descriptor):
textureTarget_(GL_TEXTURE_2D),
numTextures_(descriptor.locations.size()),
ID_(++Ninst)
{
    unsigned char** data    = new unsigned char*[numTextures_];
    PixelBuffer** px_bufs   = new PixelBuffer*[numTextures_];
    GLenum* filters         = new GLenum[numTextures_];
    GLenum* internalFormats = new GLenum[numTextures_];
    GLenum* formats         = new GLenum[numTextures_];

    uint32_t ii=0;
    for(auto&& [key, sampler_name]: SAMPLER_NAMES_)
    {
        if(!descriptor.has_unit(key))
            continue;

        filters[ii] = descriptor.parameters.filter;
        if(sampler_name == H_("mt.diffuseTex"))
        {
            // Load Albedo / Diffuse textures as sRGB to avoid
            // double gamma-correction.
            internalFormats[ii] = GL_SRGB_ALPHA;
        }
        else
        {
            internalFormats[ii] = descriptor.parameters.internal_format;
        }
        formats[ii] = descriptor.parameters.format;

        try
        {
            px_bufs[ii] = PngLoader::Instance().load_png((TEX_IMAGE_PATH + descriptor.locations.at(key)).c_str());
            data[ii] = px_bufs[ii]->get_data_pointer();
            #if __DEBUG_TEXTURE_VERBOSE__
                DLOGN("[PixelBuffer] <z>[" + std::to_string(ii) + "]</z>");
                std::cout << *px_bufs[ii] << std::endl;
            #endif
        }
        catch(const std::exception& e)
        {
            DLOGF("[Texture] Unable to load Texture.");
            for (uint32_t jj=0; jj<=ii; ++jj)
                if(px_bufs[jj])
                    delete px_bufs[jj];
            delete [] formats;
            delete [] internalFormats;
            delete [] filters;
            delete [] data;
            delete [] px_bufs;
            throw;
        }
        ++ii;
    }

    // Get texture size
#ifdef __PROFILING_SET_2x2_TEXTURE__
    width_  = 2;
    height_ = 2;
#else
    width_  = px_bufs[0]->get_width();
    height_ = px_bufs[0]->get_height();
#endif

    // * GL calls
    // Generate the right amount of textures
    textureID_ = new GLuint[numTextures_];
    is_depth_  = new bool[numTextures_];
    glGenTextures(numTextures_, textureID_);
    // For each texture in there
    for(uint32_t ii = 0; ii < numTextures_; ++ii)
    {
        bind(ii);
        // Generate filter and check whether this unit has mipmaps
        bool has_mipmap = handle_filter(filters[ii], textureTarget_);
        // Set clamp/wrap parameters
        handle_addressUV(descriptor.parameters.clamp, textureTarget_);
        // Check whether this unit has depth information
        is_depth_[ii] = (formats[ii] == GL_DEPTH_COMPONENT ||
                         formats[ii] == GL_DEPTH_STENCIL);
        // Get data type relative to internal format
        GLenum dataType = internal_format_to_data_type(internalFormats[ii]);

        // Specify OpenGL texture
        glTexImage2D(textureTarget_,
                     0,
                     internalFormats[ii],
                     width_,
                     height_,
                     0,
                     formats[ii],
                     dataType,
                     (data)?data[ii]:nullptr);

        // Handle mipmap if specified
        handle_mipmap(has_mipmap, textureTarget_);
    }

    // Free allocations
    for (uint32_t jj=0; jj<numTextures_; ++jj)
        if(px_bufs[jj])
            delete px_bufs[jj];
    delete [] formats;
    delete [] internalFormats;
    delete [] filters;
    delete [] data;
    delete [] px_bufs;
}


Texture::TextureInternal::TextureInternal(GLenum textureTarget,
                                          uint32_t numTextures,
                                          uint32_t width,
                                          uint32_t height,
                                          unsigned char** data,
                                          GLenum* filters,
                                          GLenum* internalFormats,
                                          GLenum* formats,
                                          bool clamp,
                                          bool lazy_mipmap):
textureTarget_(textureTarget),
numTextures_(numTextures),
#ifdef __PROFILING_SET_2x2_TEXTURE__
    width_(2),
    height_(2),
#else
    width_(width),
    height_(height),
#endif
ID_(++Ninst)
{
    textureID_ = new GLuint[numTextures_];
    is_depth_  = new bool[numTextures_];

    // Generate the right amount of texture units
    glGenTextures(numTextures_, textureID_);
    // For each unit
    for(uint32_t ii = 0; ii < numTextures_; ++ii)
    {
        // Bind unit
        bind(ii);
        // Generate filter and check whether this unit has mipmaps
        bool has_mipmap = handle_filter(filters[ii], textureTarget_);
        // Set clamp/wrap parameters
        handle_addressUV(clamp, textureTarget_);
        // Check whether this unit has depth information
        is_depth_[ii] = (formats[ii] == GL_DEPTH_COMPONENT ||
                         formats[ii] == GL_DEPTH_STENCIL);
        // Get data type relative to internal format
        GLenum dataType = internal_format_to_data_type(internalFormats[ii]);

        // Specify OpenGL texture
        glTexImage2D(textureTarget_,
                     0,
                     internalFormats[ii],
                     width_,
                     height_,
                     0,
                     formats[ii],
                     dataType,
                     (data)?data[ii]:nullptr);

        // Handle mipmap if specified
        handle_mipmap(has_mipmap && !lazy_mipmap, textureTarget_);
    }
}


Texture::TextureInternal::~TextureInternal()
{
    if(*textureID_) glDeleteTextures(numTextures_, textureID_);
    if(textureID_) delete[] textureID_;
    if(is_depth_) delete[] is_depth_;
}

void Texture::TextureInternal::bind(GLuint textureNum) const
{
    glBindTexture(textureTarget_, textureID_[textureNum]);
}

Texture::Texture(const std::vector<hash_t>& sampler_names,
                 uint32_t width,
                 uint32_t height,
                 GLenum textureTarget,
                 GLenum filter,
                 GLenum internalFormat,
                 GLenum format,
                 bool clamp,
                 bool lazy_mipmap):
resourceID_(H_("")),
units_(0),
uniform_sampler_names_(sampler_names)
{
    GLenum* filters         = new GLenum[sampler_names.size()];
    GLenum* internalFormats = new GLenum[sampler_names.size()];
    GLenum* formats         = new GLenum[sampler_names.size()];
    for(uint32_t ii=0; ii<sampler_names.size(); ++ii)
    {
        filters[ii] = filter;
        internalFormats[ii] = internalFormat;
        formats[ii] = format;
    }

    internal_ = std::make_shared<TextureInternal>(textureTarget,
                                                  sampler_names.size(),
                                                  width,
                                                  height,
                                                  nullptr,
                                                  filters,
                                                  internalFormats,
                                                  formats,
                                                  clamp,
                                                  lazy_mipmap);
    delete [] filters;
    delete [] internalFormats;
    delete [] formats;
}

Texture::Texture(const std::vector<hash_t>& sampler_names,
                 const std::vector<GLenum>& filters,
                 const std::vector<GLenum>& internalFormats,
                 const std::vector<GLenum>& formats,
                 uint32_t width,
                 uint32_t height,
                 GLenum textureTarget,
                 bool clamp,
                 bool lazy_mipmap):
internal_(new TextureInternal(textureTarget,
                              sampler_names.size(),
                              width,
                              height,
                              nullptr,
                              (GLenum*)&filters[0],
                              (GLenum*)&internalFormats[0],
                              (GLenum*)&formats[0],
                              clamp,
                              lazy_mipmap)),
resourceID_(H_("")),
units_(0),
uniform_sampler_names_(sampler_names){}

Texture::Texture(const Texture& texture) :
internal_(texture.internal_),
resourceID_(texture.resourceID_),
units_(texture.units_),
uniform_sampler_names_(texture.uniform_sampler_names_),
unit_indices_(texture.unit_indices_){}

Texture::Texture(Texture&& texture):
internal_(std::move(texture.internal_)),
resourceID_(std::move(texture.resourceID_)),
units_(std::move(texture.units_)),
uniform_sampler_names_(std::move(texture.uniform_sampler_names_)),
unit_indices_(std::move(texture.unit_indices_)){}

#include <sstream>

Texture::Texture(const TextureDescriptor& descriptor):
resourceID_(descriptor.resource_id),
units_(descriptor.units)
{
    #if __DEBUG_TEXTURE_VERBOSE__
    {
        std::stringstream ss;
        ss << "[Texture] New texture from asset: <n>" << resourceID_ << "</n>";
        DLOGN(ss.str());
    }
    #endif


    // Try to find a cached version first
    auto it = RESOURCE_MAP_.find(resourceID_);
    bool cache_exists = (it != RESOURCE_MAP_.end());

    // Register a sampler name for each unit
    for(auto&& [key, sampler_name]: SAMPLER_NAMES_)
    {
        if(descriptor.has_unit(key))
        {
            unit_indices_[key] = uniform_sampler_names_.size();
            uniform_sampler_names_.push_back(sampler_name);

            #if __DEBUG_TEXTURE_VERBOSE__
                if(!cache_exists)
                {
                    std::stringstream ss;
                    ss << "<v>" << sampler_name << "</v> <- <p>"
                       << descriptor.locations.at(key) << "</p>";
                    DLOGI(ss.str());
                }
            #endif
        }
    }

    // If cached version exists, use it.
    if(cache_exists)
    {
        internal_ = it->second;
        #if __DEBUG_TEXTURE_VERBOSE__
            DLOGI("<i>Using cache.</i>");
        #endif
    }
    // Else, create new texture internal using parameters
    else
    {
        internal_ = std::make_shared<TextureInternal>(descriptor);

        // Cache resource for later
        RESOURCE_MAP_.insert(std::make_pair(resourceID_, internal_));
    }
}

Texture::~Texture()
{
    // If only the object which caused destruction of Texture
    // and the resource map hold a reference to texture,
    // no need to cache texture anymore
    if(internal_.use_count() == 2)
    {
        if(resourceID_ != H_(""))
        {
            RESOURCE_MAP_.erase(resourceID_);
            #ifdef __DEBUG_TEXTURE_VERBOSE__
            std::stringstream ss;
            ss << "[Texture] Destroying cached texture: <n>" << resourceID_ << "</n>";
            DLOGN(ss.str());
            #endif
        }
    }
}

void Texture::bind(GLuint unit) const
{
    assert(unit >= 0 && unit <= 31);
    glActiveTexture(GL_TEXTURE0 + unit);
    internal_->bind(unit);

#ifdef __DEBUG_TEXTURE__
    internal_->set_binding_state(unit, unit);
#endif
}

void Texture::bind(GLuint unit, uint32_t index) const
{
    assert(unit >= 0 && unit <= 31);
    assert(index >= 0 && index < internal_->get_num_textures());
    glActiveTexture(GL_TEXTURE0 + unit);
    internal_->bind(index);

#ifdef __DEBUG_TEXTURE__
    internal_->set_binding_state(index, unit);
#endif
}

void Texture::bind_all() const
{
    for(GLuint ii=0; ii<internal_->get_num_textures(); ++ii)
        bind(ii);
}

void Texture::unbind() const
{
    for(GLuint ii=0; ii<internal_->get_num_textures(); ++ii)
    {
        glActiveTexture(GL_TEXTURE0 + ii);
        glBindTexture(GL_TEXTURE_2D, 0);
#ifdef __DEBUG_TEXTURE__
        internal_->set_binding_state(ii, -1);
#endif
    }
}

void Texture::generate_mipmaps(uint32_t unit,
                               uint32_t base_level,
                               uint32_t max_level) const
{
    internal_->bind(unit);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, base_level);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, max_level);
    glGenerateMipmap(GL_TEXTURE_2D);
    GLfloat maxAnisotropy;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
    glTexParameterf(GL_TEXTURE_2D,
                    GL_TEXTURE_MAX_ANISOTROPY_EXT,
                    math::clamp(0.0f, 8.0f, maxAnisotropy));
}

bool Texture::operator==(const Texture& texture) const
{
    return internal_->ID() == texture.internal_->ID();
}

bool Texture::operator!=(const Texture& texture) const
{
    return !operator==(texture);
}
