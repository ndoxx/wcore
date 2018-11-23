#include "pixel_buffer.h"

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
        // Reverse row order to make it compatible with OpenGL
        uint32_t offset = (imgHeight - ii - 1) * stride_;
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

}
