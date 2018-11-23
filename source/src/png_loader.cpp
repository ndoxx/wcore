#include <png.h>
#include <istream>
#include <stdexcept>
#include <sstream>

#include "png_loader.h"
#include "logger.h"
#include "pixel_buffer.h"

namespace wcore
{

static constexpr const int PNGSIGSIZE = 8;

static bool is_valid_png(std::istream& source) {

    // Compare file signature
    png_byte pngsig[PNGSIGSIZE];
    int is_png = 0;
    source.read((char*)pngsig, PNGSIGSIZE);

    if (!source.good()) return false;

    is_png = png_sig_cmp(pngsig, 0, PNGSIGSIZE);
    return (is_png == 0);
}

static void stream_read_data(png_structp p_png, png_bytep data, png_size_t length)
{
    //Here we get our IO pointer back from the read struct.
    //This is the parameter we passed to the png_set_read_fn() function.
    //Our std::istream pointer.
    png_voidp a = png_get_io_ptr(p_png);
    //Cast the pointer to std::istream* and read 'length' bytes into 'data'
    ((std::istream*)a)->read((char*)data, length);
}

PngLoader::PngLoader()
{

}

PngLoader::~PngLoader()
{

}

PixelBuffer* PngLoader::load_png(const char* filename)
{
    // Try to open file, throw if not found.
    std::ifstream source;
    source.open(filename, std::ios::binary | std::ios::in);
    if(!source)
    {
        std::stringstream ss;
        ss << "[PngLoader] Couldn't reach file: " << filename;
        DLOGE(ss.str(), "io", Severity::CRIT);
        throw std::runtime_error("Couldn't reach file.");
    }

    // Validate file as a png by checking signature
    if(!is_valid_png(source))
    {
        std::stringstream ss;
        ss << "[PngLoader] File: " << filename << " is not a valid PNG file.";
        DLOGE(ss.str(), "parsing", Severity::CRIT);
        throw std::runtime_error("Not a valid PNG file.");
    }

    // Get a handle on png file
    png_structp p_png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!p_png)
    {
        std::stringstream ss;
        ss << "[PngLoader] Couldn't initialize png read struct for: " << filename;
        DLOGE(ss.str(), "parsing", Severity::CRIT);
        throw std::runtime_error("Read struct init failed.");
    }

    // Get info struct
    png_infop p_info = png_create_info_struct(p_png);
    if (!p_info)
    {
        std::stringstream ss;
        ss << "[PngLoader] Couldn't initialize png info struct for: " << filename;
        png_destroy_read_struct(&p_png, (png_infopp)0, (png_infopp)0);
        DLOGE(ss.str(), "parsing", Severity::CRIT);
        throw std::runtime_error("Info struct init failed.");
    }

    PixelBuffer* px_buf = nullptr;

    // DIRTY Error handling, libpng JUMPS here on error.
    if (setjmp(png_jmpbuf(p_png))) {
        //An error occured, so clean up what we have allocated so far...
        png_destroy_read_struct(&p_png, &p_info,(png_infopp)0);
        if (px_buf != nullptr) delete px_buf;
        std::stringstream ss;
        ss << "[PngLoader] An error occured while reading: " << filename;
        DLOGE(ss.str(), "parsing", Severity::CRIT);
        throw std::runtime_error("Png read error.");
    }

    // Set data read function to our stream reader
    png_set_read_fn(p_png,(png_voidp)&source, stream_read_data);
    // Tell libpng we already read the first 8 bytes.
    png_set_sig_bytes(p_png, PNGSIGSIZE);
    // Read header
    png_read_info(p_png, p_info);

    png_uint_32 imgWidth  = png_get_image_width(p_png, p_info);
    png_uint_32 imgHeight = png_get_image_height(p_png, p_info);
    png_uint_32 bitDepth  = png_get_bit_depth(p_png, p_info);  // Bits per CHANNEL!
    png_uint_32 channels  = png_get_channels(p_png, p_info);   // Number of channels
    png_uint_32 colorType = png_get_color_type(p_png, p_info); // Color type. (RGB, RGBA, Luminance, luminance alpha... palette... etc)

    // We may want to convert formats to have proper rgb color
    switch (colorType)
    {
        // Load palette as RGB
        case PNG_COLOR_TYPE_PALETTE:
            png_set_palette_to_rgb(p_png);
            channels = 3;   // Update number of channels (was 1)
            break;
        // Load grayscale as RGB
        case PNG_COLOR_TYPE_GRAY:
            if (bitDepth < 8)
            png_set_expand_gray_1_2_4_to_8(p_png);
            bitDepth = 8;   // Update bit depth info
            break;
    }

    // Convert to full alpha if transparency is set
    if (png_get_valid(p_png, p_info, PNG_INFO_tRNS))
    {
        png_set_tRNS_to_alpha(p_png);
        channels+=1;
    }

    if (bitDepth == 16)
    {
        png_set_strip_16(p_png);
        bitDepth = 8;
    }

    // Update info structs
    png_read_update_info(p_png, p_info);

    // Initialize PixelBuffer object and read data to its internal buffer
    // using a lambda.
    px_buf = new PixelBuffer(imgWidth, imgHeight, bitDepth, channels,
    [&](unsigned char** pp_rows)
    {
        // At last, read image to pixel buffer
        png_read_image(p_png, pp_rows);
    });

    // Clean up
    png_destroy_read_struct(&p_png, &p_info,(png_infopp)0);

    return px_buf;
}

}
