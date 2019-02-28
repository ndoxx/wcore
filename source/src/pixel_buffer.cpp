#include "pixel_buffer.h"
#include "logger.h"

namespace wcore
{

PixelBuffer::PixelBuffer(uint32_t imgWidth,
                         uint32_t imgHeight,
                         uint32_t bitDepth,
                         uint32_t channels):
width_(imgWidth),
height_(imgHeight),
bit_depth_(bitDepth),
channels_(channels),
stride_(imgWidth * bitDepth * channels / 8),
size_(imgHeight * stride_),
aspect_ratio_((1.0f*imgWidth)/imgHeight)
{
    // Allocate memory
    pp_rows_ = new unsigned char*[height_];
    p_data_  = new unsigned char[size_];

    // Set row pointers to the correct offsets in data buffer (one for each row)
    for (uint32_t ii=0; ii < imgHeight; ++ii) // For each row
    {
        uint32_t offset = ii * stride_;
        pp_rows_[ii]    = (unsigned char*)p_data_ + offset;
    }
}

PixelBuffer::PixelBuffer(uint32_t imgWidth,
                         uint32_t imgHeight,
                         uint32_t bitDepth,
                         uint32_t channels,
                         std::function<void(unsigned char**)> init_func):
PixelBuffer(imgWidth, imgHeight, bitDepth, channels)
{
    // External code will use pp_rows_ to initialize the buffer
    init_func(pp_rows_);
    delete [] pp_rows_;
    pp_rows_ = nullptr;
}

PixelBuffer::~PixelBuffer()
{
    if(pp_rows_!=nullptr) delete [] pp_rows_;
    delete [] p_data_;
}

void PixelBuffer::debug_display()
{
    std::stringstream stream;
    stream << "WxH= " << width_ << "x" << height_
           << " -> aspect ratio= " << aspect_ratio_;
    DLOGI(stream.str(), "texture", Severity::DET);
    stream.str("");

    stream << channels_ << " color channels, bitdepth= " << bit_depth_;
    DLOGI(stream.str(), "texture", Severity::DET);
    stream.str("");

    stream << "stride= " << stride_ << ", total size= " << size_/1024.0f << "kB";
    DLOGI(stream.str(), "texture", Severity::DET);
    stream.str("");

    stream << "first 10 bytes: [";
    for(unsigned short ii=0; ii<10; ++ii)
    {
        stream << std::hex << (int)p_data_[ii];
        stream << " ";
    }
    stream << "...]" << std::dec;
    DLOGI(stream.str(), "texture", Severity::DET);
    stream.str("");
}


}
