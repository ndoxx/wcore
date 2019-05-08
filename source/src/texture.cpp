#include <cassert>
#include <vector>
#include <sstream>


#include "texture.h"
#include "pixel_buffer.h"
#include "shader.h"
#include "logger.h"
#include "algorithms.h"
#include "material_common.h"
#include "config.h"
#include "file_system.h"
#include "error.h"

namespace wcore
{

#ifdef __TEXTURE_OLD__

uint32_t Texture::TextureInternal::Ninst = 0;
Texture::RMap Texture::RESOURCE_MAP_;

static std::vector<std::map<TextureUnit, hash_t>> SAMPLER_NAMES_ =
{
    // Sampler names for sampler group 1
    {
        {TextureUnit::BLOCK0, "mt.sg1.block0Tex"_h},
        {TextureUnit::BLOCK1, "mt.sg1.block1Tex"_h},
        {TextureUnit::BLOCK2, "mt.sg1.block2Tex"_h}
    },

    // Sampler names for sampler group 2
    {
        {TextureUnit::BLOCK0, "mt.sg2.block0Tex"_h},
        {TextureUnit::BLOCK1, "mt.sg2.block1Tex"_h},
        {TextureUnit::BLOCK2, "mt.sg2.block2Tex"_h}
    }
};

static uint32_t SAMPLER_GROUP_SIZE = 3;

PngLoader Texture::png_loader_;

static std::map<GLenum, GLenum> DATA_TYPES =
{
    {GL_SRGB_ALPHA, GL_FLOAT},
    {GL_RGB16F, GL_FLOAT},
    {GL_RGBA16F, GL_FLOAT},
    {GL_RG16_SNORM, GL_UNSIGNED_BYTE},
    {GL_RGB16_SNORM, GL_UNSIGNED_BYTE},
    {GL_DEPTH32F_STENCIL8, GL_FLOAT_32_UNSIGNED_INT_24_8_REV},
    {GL_DEPTH24_STENCIL8, GL_UNSIGNED_INT_24_8},
    {GL_RGB8, GL_BYTE},
    {GL_R8, GL_BYTE},
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
    for(auto&& [key, sampler_name]: SAMPLER_NAMES_[descriptor.sampler_group-1])
    {
        if(!descriptor.has_unit(key))
            continue;

        filters[ii] = descriptor.parameters.filter;
        if(sampler_name == "mt.sg1.block0Tex"_h || sampler_name == "mt.sg2.block0Tex"_h)
        {
            // Load Albedo / Diffuse textures as sRGB to avoid
            // double gamma-correction.
            internalFormats[ii] = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
            //internalFormats[ii] = GL_SRGB_ALPHA;
        }
        else if(sampler_name == "mt.sg1.block1Tex"_h || sampler_name == "mt.sg2.block1Tex"_h)
        {
            // Do not use DXT1 compression on normal-depth texure (block1) because
            // it will screw up the normals
            internalFormats[ii] = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;//GL_RGBA;
        }
        else
        {
            internalFormats[ii] = descriptor.parameters.internal_format;
        }
        formats[ii] = descriptor.parameters.format;

        auto stream = FILESYSTEM.get_file_as_stream(descriptor.locations.at(key).c_str(), "root.folders.texture"_h, "pack0"_h);
        px_bufs[ii] = png_loader_.load_png(*stream);

        if(px_bufs[ii])
        {
            data[ii] = px_bufs[ii]->get_data_pointer();
            #if __DEBUG__
                DLOGN("[PixelBuffer] <z>[" + std::to_string(ii) + "]</z>", "texture");
                if(dbg::LOG.get_channel_verbosity("texture"_h) == 3u)
                    px_bufs[ii]->debug_display();
                    //std::cout << *px_bufs[ii] << std::endl;
            #endif
        }
        else
        {
            DLOGF("[Texture] Unable to load Texture.", "texture");
            for (uint32_t jj=0; jj<=ii; ++jj)
                if(px_bufs[jj])
                    delete px_bufs[jj];
            delete [] formats;
            delete [] internalFormats;
            delete [] filters;
            delete [] data;
            delete [] px_bufs;
            fatal();
        }
        ++ii;
    }

    generate_texture_units(GL_TEXTURE_2D,
                           numTextures_,
                           px_bufs[0]->get_width(),
                           px_bufs[0]->get_height(),
                           data,
                           filters,
                           internalFormats,
                           formats,
                           descriptor.parameters.clamp,
                           false);


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

Texture::TextureInternal::TextureInternal(std::istream& stream):
ID_(++Ninst)
{
    if(!stream.good())
    {
        DLOGE("[Texture] Failed to create texture from stream: stream is bad.", "texture");
        return;
    }

    PixelBuffer* px_buf = png_loader_.load_png(stream);

#if __DEBUG__
    DLOGN("[PixelBuffer]", "texture");
    if(dbg::LOG.get_channel_verbosity("texture"_h) == 3u)
        px_buf->debug_display();
#endif

    GLenum filter = GL_LINEAR;
    GLenum internal_format = GL_RGB;
    GLenum format = GL_RGB;
    unsigned char* data = px_buf->get_data_pointer();
    generate_texture_units(GL_TEXTURE_2D,
                           1,
                           px_buf->get_width(),
                           px_buf->get_height(),
                           &data,
                           &filter,
                           &internal_format,
                           &format,
                           true,
                           false);

    delete px_buf;
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
ID_(++Ninst)
{
    generate_texture_units(textureTarget,
                           numTextures,
                           width,
                           height,
                           data,
                           filters,
                           internalFormats,
                           formats,
                           clamp,
                           lazy_mipmap);
}

void Texture::TextureInternal::generate_texture_units(GLenum textureTarget,
                                                      uint32_t numTextures,
                                                      uint32_t width,
                                                      uint32_t height,
                                                      unsigned char** data,
                                                      GLenum* filters,
                                                      GLenum* internalFormats,
                                                      GLenum* formats,
                                                      bool clamp,
                                                      bool lazy_mipmap)
{
    textureTarget_ = textureTarget;
    numTextures_ = numTextures;
#ifdef __PROFILING_SET_2x2_TEXTURE__
    width_  = 2;
    height_ = 2;
#else
    width_  = width;
    height_ = height;
#endif

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
        //handle_mipmap(has_mipmap && !lazy_mipmap, textureTarget_);
        if(has_mipmap && !lazy_mipmap)
            generate_mipmaps(ii);
        else
        {
            glTexParameteri(textureTarget_, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(textureTarget_, GL_TEXTURE_MAX_LEVEL, 0);
        }
    }
}


Texture::TextureInternal::~TextureInternal()
{
    if(textureID_)
    {
        glDeleteTextures(numTextures_, textureID_);
        delete[] textureID_;
    }
    if(is_depth_) delete[] is_depth_;
}

void Texture::TextureInternal::bind(GLuint textureNum) const
{
    glBindTexture(textureTarget_, textureID_[textureNum]);
}

void Texture::TextureInternal::generate_mipmaps(uint32_t unit,
                                                uint32_t base_level,
                                                uint32_t max_level) const
{
    bind(unit);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, base_level);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, max_level);
    glGenerateMipmap(GL_TEXTURE_2D);
    GLfloat maxAnisotropy;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
    glTexParameterf(GL_TEXTURE_2D,
                    GL_TEXTURE_MAX_ANISOTROPY_EXT,
                    math::clamp(0.0f, 8.0f, maxAnisotropy));
}

Texture::Texture(const std::vector<hash_t>& sampler_names,
                 const std::vector<uint32_t>& filters,
                 const std::vector<uint32_t>& internalFormats,
                 const std::vector<uint32_t>& formats,
                 uint32_t width,
                 uint32_t height,
                 bool clamp,
                 bool lazy_mipmap):
internal_(new TextureInternal(GL_TEXTURE_2D,
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
sampler_group_(1),
uniform_sampler_names_(sampler_names){}

Texture::Texture(const TextureDescriptor& descriptor):
resourceID_(descriptor.resource_id),
units_(descriptor.units)
{
    #if __DEBUG__
    {
        std::stringstream ss;
        ss << "[Texture] New texture from asset: <n>" << HRESOLVE(resourceID_) << "</n>";
        DLOGN(ss.str(), "texture");
    }
    #endif


    // Try to find a cached version first
    auto it = RESOURCE_MAP_.find(resourceID_);
    bool cache_exists = (it != RESOURCE_MAP_.end());

    // Register a sampler name for each unit
    sampler_group_ = descriptor.sampler_group;
    for(auto&& [key, sampler_name]: SAMPLER_NAMES_[descriptor.sampler_group-1])
    {
        if(descriptor.has_unit(key))
        {
            unit_indices_[key] = uniform_sampler_names_.size() + (descriptor.sampler_group-1)*SAMPLER_GROUP_SIZE;
            uniform_sampler_names_.push_back(sampler_name);

            #if __DEBUG__
                if(!cache_exists)
                {
                    std::stringstream ss;
                    ss << "<v>" << sampler_name << "</v> <- <p>"
                       << descriptor.locations.at(key) << "</p>";
                    DLOGI(ss.str(), "texture");
                }
            #endif
        }
    }

    // If cached version exists, use it.
    if(cache_exists)
    {
        internal_ = it->second;
        #if __DEBUG__
            DLOGI("<i>Using cache.</i>", "texture");
        #endif
    }
    // Else, create new texture internal using parameters
    else
    {
        internal_ = std::make_shared<TextureInternal>(descriptor);

        // Cache resource for later if texture is meant to be cached
        if(resourceID_ != ""_h)
            RESOURCE_MAP_.insert(std::make_pair(resourceID_, internal_));
    }
}

Texture::Texture(std::istream& stream):
resourceID_(""_h),
units_(1)
{
    internal_ = std::make_shared<TextureInternal>(stream);
}


Texture::~Texture()
{
    // If only the object which caused destruction of Texture
    // and the resource map hold a reference to texture,
    // no need to cache texture anymore
    if(internal_.use_count() == 2)
    {
        if(resourceID_ != ""_h)
        {
            RESOURCE_MAP_.erase(resourceID_);
            #ifdef __DEBUG__
            std::stringstream ss;
            ss << "[Texture] Destroying cached texture: <n>" << HRESOLVE(resourceID_) << "</n>";
            DLOGN(ss.str(), "texture");
            #endif
        }
    }
}

void Texture::bind(GLuint unit) const
{
    assert(unit >= 0 && unit <= 31);
    glActiveTexture(GL_TEXTURE0 + unit + (sampler_group_-1)*SAMPLER_GROUP_SIZE);
    internal_->bind(unit);
}

void Texture::bind(GLuint unit, uint32_t index) const
{
    assert(unit >= 0 && unit <= 31);
    assert(index >= 0 && index < internal_->numTextures_);
    glActiveTexture(GL_TEXTURE0 + unit);
    internal_->bind(index);
}

void Texture::bind_all() const
{
    for(GLuint ii=0; ii<internal_->numTextures_; ++ii)
        bind(ii);
}

void Texture::unbind() const
{
    for(GLuint ii=0; ii<internal_->numTextures_; ++ii)
    {
        glActiveTexture(GL_TEXTURE0 + ii);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void Texture::bind_sampler(const Shader& shader, TextureUnit unit) const
{
    //shader.send_uniform<int>(unit_to_sampler_name(unit), get_unit_index(unit));
    shader.send_uniform<int>(SAMPLER_NAMES_[sampler_group_-1].at(unit), unit_indices_.at(unit));
}

void Texture::generate_mipmaps(uint32_t unit,
                               uint32_t base_level,
                               uint32_t max_level) const
{
    internal_->generate_mipmaps(unit, base_level, max_level);
}

bool Texture::operator==(const Texture& texture) const
{
    return internal_->ID_ == texture.internal_->ID_;
}

bool Texture::operator!=(const Texture& texture) const
{
    return !operator==(texture);
}








#else // __TEXTURE_OLD__

static std::vector<std::map<TextureUnit, hash_t>> SAMPLER_NAMES =
{
    // Sampler names for sampler group 1
    {
        {TextureUnit::BLOCK0, "mt.sg1.block0Tex"_h},
        {TextureUnit::BLOCK1, "mt.sg1.block1Tex"_h},
        {TextureUnit::BLOCK2, "mt.sg1.block2Tex"_h}
    },

    // Sampler names for sampler group 2
    {
        {TextureUnit::BLOCK0, "mt.sg2.block0Tex"_h},
        {TextureUnit::BLOCK1, "mt.sg2.block1Tex"_h},
        {TextureUnit::BLOCK2, "mt.sg2.block2Tex"_h}
    }
};

static const uint32_t SAMPLER_GROUP_SIZE = 3;

static std::map<GLenum, GLenum> DATA_TYPES =
{
    {GL_SRGB_ALPHA,        GL_FLOAT},
    {GL_RGB16F,            GL_FLOAT},
    {GL_RGBA16F,           GL_FLOAT},
    {GL_RG16_SNORM,        GL_UNSIGNED_BYTE},
    {GL_RGB16_SNORM,       GL_UNSIGNED_BYTE},
    {GL_DEPTH32F_STENCIL8, GL_FLOAT_32_UNSIGNED_INT_24_8_REV},
    {GL_DEPTH24_STENCIL8,  GL_UNSIGNED_INT_24_8},
    {GL_RGB8,              GL_BYTE},
    {GL_R8,                GL_BYTE},
};

static PngLoader PNG_LOADER;

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

static void handle_addressUV(bool clamp, GLenum target)
{
    if(clamp)
    {
        glTexParameterf(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
}

static GLenum internal_format_to_data_type(GLenum iformat)
{
    auto it = DATA_TYPES.find(iformat);
    if(it!=DATA_TYPES.end())
        return it->second;
    return GL_UNSIGNED_BYTE;
}

Texture::Texture(const TextureDescriptor& descriptor):
n_units_(descriptor.locations.size()),
unit_flags_(descriptor.units)
{
#ifdef __DEBUG__
    {
        std::stringstream ss;
        ss << "[Texture] New texture from asset: <n>" << HRESOLVE(descriptor.resource_id) << "</n>";
        DLOGN(ss.str(), "texture");
    }
#endif

    // Load data
    unsigned char** data    = new unsigned char*[n_units_];
    PixelBuffer** px_bufs   = new PixelBuffer*[n_units_];
    GLenum* filters         = new GLenum[n_units_];
    GLenum* internalFormats = new GLenum[n_units_];
    GLenum* formats         = new GLenum[n_units_];

    sampler_group_ = descriptor.sampler_group;
    uint32_t ii=0;
    for(auto&& [key, sampler_name]: SAMPLER_NAMES[descriptor.sampler_group-1])
    {
        if(descriptor.has_unit(key))
        {
            // Register a sampler name for each unit
            unit_indices_[key] = uniform_sampler_names_.size() + (descriptor.sampler_group-1)*SAMPLER_GROUP_SIZE;
            uniform_sampler_names_.push_back(sampler_name);

            filters[ii] = descriptor.parameters.filter;

            // Load Albedo / Diffuse textures as sRGB to avoid double gamma-correction.
            if(sampler_name == "mt.sg1.block0Tex"_h || sampler_name == "mt.sg2.block0Tex"_h)
                internalFormats[ii] = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
            // Do not use DXT1 compression on normal-depth texure (block1) because it will screw up the normals
            else if(sampler_name == "mt.sg1.block1Tex"_h || sampler_name == "mt.sg2.block1Tex"_h)
                internalFormats[ii] = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;//GL_RGBA;
            else
                internalFormats[ii] = descriptor.parameters.internal_format;
            formats[ii] = descriptor.parameters.format;

            // Load from PNG
            auto stream = FILESYSTEM.get_file_as_stream(descriptor.locations.at(key).c_str(), "root.folders.texture"_h, "pack0"_h);
            px_bufs[ii] = PNG_LOADER.load_png(*stream);

            if(px_bufs[ii])
            {
                data[ii] = px_bufs[ii]->get_data_pointer();
#ifdef __DEBUG__
                DLOGN("[PixelBuffer] <z>[" + std::to_string(ii) + "]</z>", "texture");
                if(dbg::LOG.get_channel_verbosity("texture"_h) == 3u)
                    px_bufs[ii]->debug_display();
#endif
            }
            else
            {
                DLOGF("[Texture] Unable to load Texture.", "texture");
                fatal();
            }
            ++ii;
        }
    }

    generate_texture_units(n_units_,
                           px_bufs[0]->get_width(),
                           px_bufs[0]->get_height(),
                           data,
                           filters,
                           internalFormats,
                           formats,
                           descriptor.parameters.clamp,
                           false);


    // Free allocations
    for (uint32_t jj=0; jj<n_units_; ++jj)
        if(px_bufs[jj])
            delete px_bufs[jj];
    delete [] formats;
    delete [] internalFormats;
    delete [] filters;
    delete [] data;
    delete [] px_bufs;
}

Texture::Texture(std::istream& stream):
unit_flags_(0),
sampler_group_(1)
{
    // Sanity check on stream
    if(!stream.good())
    {
        DLOGE("[Texture] Failed to create texture from stream: stream is bad.", "texture");
        return;
    }

    // ASSUME stream to png file
    PixelBuffer* px_buf = PNG_LOADER.load_png(stream);

#if __DEBUG__
    DLOGN("[PixelBuffer]", "texture");
    if(dbg::LOG.get_channel_verbosity("texture"_h) == 3u)
        px_buf->debug_display();
#endif

    GLenum filter = GL_LINEAR;
    GLenum internal_format = GL_RGB;
    GLenum format = GL_RGB;
    unsigned char* data = px_buf->get_data_pointer();
    generate_texture_units(1,
                           px_buf->get_width(),
                           px_buf->get_height(),
                           &data,
                           &filter,
                           &internal_format,
                           &format,
                           true,
                           false);

    delete px_buf;
}

Texture::Texture(const std::vector<hash_t>& sampler_names,
        const std::vector<uint32_t>& filters,
        const std::vector<uint32_t>& internalFormats,
        const std::vector<uint32_t>& formats,
        uint32_t width,
        uint32_t height,
        bool clamp,
        bool lazy_mipmap):
unit_flags_(0),
sampler_group_(1),
uniform_sampler_names_(sampler_names)
{
    generate_texture_units(sampler_names.size(),
                           width,
                           height,
                           nullptr,
                           (GLenum*)&filters[0],
                           (GLenum*)&internalFormats[0],
                           (GLenum*)&formats[0],
                           clamp,
                           lazy_mipmap);
}

Texture::~Texture()
{
    if(texture_ids_)
    {
        glDeleteTextures(n_units_, texture_ids_);
        delete[] texture_ids_;
    }
}

void Texture::bind_sampler(const Shader& shader, TextureUnit unit) const
{
    shader.send_uniform<int>(SAMPLER_NAMES[sampler_group_-1].at(unit), unit_indices_.at(unit));
}

void Texture::bind_all() const
{
    for(uint32_t ii=0; ii<n_units_; ++ii)
        bind(ii,ii);
}

void Texture::bind(uint32_t unit, uint32_t index) const
{
    glActiveTexture(GL_TEXTURE0 + unit + (sampler_group_-1)*SAMPLER_GROUP_SIZE);
    glBindTexture(GL_TEXTURE_2D, texture_ids_[index]);
}

void Texture::unbind() const
{
    for(uint32_t ii=0; ii<n_units_; ++ii)
    {
        glActiveTexture(GL_TEXTURE0 + ii);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void Texture::generate_mipmaps(uint32_t index,
                               uint32_t base_level,
                               uint32_t max_level) const
{
    glBindTexture(GL_TEXTURE_2D, texture_ids_[index]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, base_level);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, max_level);
    glGenerateMipmap(GL_TEXTURE_2D);
    GLfloat maxAnisotropy;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
    glTexParameterf(GL_TEXTURE_2D,
                    GL_TEXTURE_MAX_ANISOTROPY_EXT,
                    math::clamp(0.0f, 8.0f, maxAnisotropy));
}

void Texture::generate_texture_units(uint32_t n_units,
                                     uint32_t width,
                                     uint32_t height,
                                     unsigned char** data,
                                     unsigned int* filters,
                                     unsigned int* internalFormats,
                                     unsigned int* formats,
                                     bool clamp,
                                     bool lazy_mipmap)
{
    n_units_ = n_units;
#ifdef __PROFILING_SET_2x2_TEXTURE__
    width_  = 2;
    height_ = 2;
#else
    width_  = width;
    height_ = height;
#endif

    texture_ids_ = new GLuint[n_units];

    // Generate the right amount of texture units
    glGenTextures(n_units, texture_ids_);
    // For each unit
    for(uint32_t ii = 0; ii < n_units; ++ii)
    {
        // Bind unit
        bind(ii, ii);
        // Generate filter and check whether this unit has mipmaps
        bool has_mipmap = handle_filter(filters[ii], GL_TEXTURE_2D);
        // Set clamp/wrap parameters
        handle_addressUV(clamp, GL_TEXTURE_2D);
        // Check whether this unit has depth information
        is_depth_.push_back((formats[ii] == GL_DEPTH_COMPONENT ||
                             formats[ii] == GL_DEPTH_STENCIL));
        // Get data type relative to internal format
        GLenum dataType = internal_format_to_data_type(internalFormats[ii]);

        // Specify OpenGL texture
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     internalFormats[ii],
                     width_,
                     height_,
                     0,
                     formats[ii],
                     dataType,
                     (data)?data[ii]:nullptr);

        // Handle mipmap if specified
        if(has_mipmap && !lazy_mipmap)
            generate_mipmaps(ii);
        else
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        }
    }
}

#endif // __TEXTURE_OLD__

}
