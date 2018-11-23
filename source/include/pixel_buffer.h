#ifndef PIXEL_BUFFER_H
#define PIXEL_BUFFER_H

#include <ostream>
#include <cstdint>
#include <functional>

namespace wcore
{

class PixelBuffer
{
private:
    unsigned char*  p_data_;
    unsigned char** pp_rows_;
    uint32_t        width_;
    uint32_t        height_;
    uint32_t        bit_depth_;
    uint32_t        channels_;
    const uint32_t  stride_;
    const uint32_t  size_;
    float           aspect_ratio_;

public:
    PixelBuffer() = delete;
    PixelBuffer(uint32_t imgWidth, uint32_t imgHeight,
                uint32_t bitDepth, uint32_t channels);
    PixelBuffer(uint32_t imgWidth, uint32_t imgHeight,
                uint32_t bitDepth, uint32_t channels,
                std::function<void(unsigned char**)> init_func);
    ~PixelBuffer();

    inline unsigned char* get_data_pointer() { return p_data_; }
    inline uint32_t       get_width()        { return width_; }
    inline uint32_t       get_height()       { return height_; }
    inline uint32_t       get_bitdepth()     { return bit_depth_; }
    inline uint32_t       get_channels()     { return channels_; }
    inline uint32_t       get_stride()       { return stride_; }
    inline uint32_t       get_size_bytes()   { return size_; }
    inline float          get_aspect_ratio() { return aspect_ratio_; }

    inline unsigned char& operator[](uint32_t index) { return p_data_[index]; }

#ifdef __DEBUG__
    void debug_display();

    friend std::ostream& operator<<(std::ostream& stream, const PixelBuffer& px_buf)
    {
        stream << "\t|WxH= " << px_buf.width_ << "x" << px_buf.height_
               << " -> aspect ratio= " << px_buf.aspect_ratio_ << std::endl;
        stream << "\t|" << px_buf.channels_ << " color channels, bitdepth= " << px_buf.bit_depth_ << std::endl;
        stream << "\t|stride= " << px_buf.stride_ << " total size= " << px_buf.size_/1024.0f << "kB" << std::endl;
        stream << "\t|first 10 bytes: [";
        for(unsigned short ii=0; ii<10; ++ii)
        {
            stream << std::hex << (int)px_buf.p_data_[ii];
            stream << " ";
        }
        stream << "...]" << std::dec;
        return stream;
    }
#endif
};

}

#endif // PIXEL_BUFFER_H
