#include <png.h>
#include <istream>
#include <sstream>
#include <cstring>
#include <cstdio>
#include <vector>

#include "png_loader.h"
#include "logger.h"
#include "pixel_buffer.h"

namespace wcore
{

static constexpr const int PNGSIGSIZE = 8;

static bool is_valid_png(std::istream& stream)
{
    if(!stream.good()) return false;

    // Compare file signature
    png_byte pngsig[PNGSIGSIZE];
    int is_png = 0;
    stream.read((char*)pngsig, PNGSIGSIZE);

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

/*static bool is_valid_png(const std::vector<char>& source)
{
    int is_png = png_sig_cmp((png_bytep)&source[0], 0, PNGSIGSIZE);
    return (is_png == 0);
}

static void stream_read_data(png_structp p_png, png_bytep data, png_size_t length)
{
    // Here we get our IO pointer back from the read struct.
    // This is the parameter we passed to the png_set_read_fn() function.
    png_voidp source = png_get_io_ptr(p_png);
    memcpy(data, &(reinterpret_cast<unsigned char*>(source))[PNGSIGSIZE], length);
}*/

PixelBuffer* PngLoader::load_png(std::istream& stream)
{
    DLOGN("[PngLoader] Loading png image from stream.", "ios");

    if(!stream)
    {
        DLOGE("[PngLoader] Stream error.", "ios");
        return nullptr;
    }

    // Validate file as a png by checking signature
    if(!is_valid_png(stream))
    {
        DLOGE("[PngLoader] Invalid PNG file.", "ios");
        return nullptr;
    }

    // Get a handle on png file
    png_structp p_png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!p_png)
    {
        DLOGE("[PngLoader] Couldn't initialize png read struct.", "ios");
        return nullptr;
    }

    // Get info struct
    png_infop p_info = png_create_info_struct(p_png);
    if(!p_info)
    {
        DLOGE("[PngLoader] Couldn't initialize png info struct.", "ios");
        png_destroy_read_struct(&p_png, (png_infopp)0, (png_infopp)0);
        return nullptr;
    }

    PixelBuffer* px_buf = nullptr;

    // DIRTY Error handling, libpng JUMPS here on error.
    if(setjmp(png_jmpbuf(p_png)))
    {
        //An error occured, so clean up what we have allocated so far...
        png_destroy_read_struct(&p_png, &p_info,(png_infopp)0);
        if (px_buf != nullptr) delete px_buf;
        DLOGE("[PngLoader] An error occured while reading file.", "ios");
        return nullptr;
    }

    // Set data read function to our stream reader
    png_set_read_fn(p_png,(png_voidp)&stream, stream_read_data);
    //png_set_read_fn(p_png,(png_voidp)&stream[0], stream_read_data);
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
    switch(colorType)
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
    if(png_get_valid(p_png, p_info, PNG_INFO_tRNS))
    {
        png_set_tRNS_to_alpha(p_png);
        channels+=1;
    }

    if(bitDepth == 16)
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

PixelBuffer* PngLoader::load_png(const fs::path& file_path)
{
    DLOGN("[PngLoader] Loading png image:", "ios");
    DLOGI("<p>" + file_path.string() + "</p>", "ios");

    // Try to open file
    std::ifstream stream;
    stream.open(file_path, std::ios::binary | std::ios::in);

    return load_png(stream);
}

bool PngLoader::write_png(const fs::path& file_path, unsigned char* pixels, int w, int h)
{
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);//8
    if(!png)
        return false;

    png_infop info = png_create_info_struct(png);
    if(!info)
    {
        png_destroy_write_struct(&png, &info);
        return false;
    }

    FILE* fp = fopen(file_path.string().c_str(), "wb");
    if(!fp)
    {
        png_destroy_write_struct(&png, &info);
        return false;
    }
    png_init_io(png, fp);
    png_set_IHDR(png, info, w, h, 8 /* depth */, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_colorp palette = (png_colorp)png_malloc(png, PNG_MAX_PALETTE_LENGTH * sizeof(png_color));
    if(!palette)
    {
        fclose(fp);
        png_destroy_write_struct(&png, &info);
        return false;
    }
    png_set_PLTE(png, info, palette, PNG_MAX_PALETTE_LENGTH);
    png_write_info(png, info);
    png_set_packing(png);

    png_bytepp rows = (png_bytepp)png_malloc(png, h * sizeof(png_bytep));
    for(int i = 0; i < h; ++i)
        rows[i] = (png_bytep)(pixels + (h - i) * w * 4);

    png_write_image(png, rows);
    png_write_end(png, info);
    png_free(png, palette);
    png_destroy_write_struct(&png, &info);

    fclose(fp);
    delete[] rows;
    return true;
}

}
