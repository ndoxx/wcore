#ifndef PNG_LOADER_H
#define PNG_LOADER_H

#include <filesystem>
#include <istream>

namespace wcore
{

namespace fs = std::filesystem;

class PixelBuffer;
class PngLoader
{
public:
    PngLoader() = default;
    ~PngLoader() = default;

    PixelBuffer* load_png(std::istream& stream);

    [[deprecated("use streams instead")]]
    PixelBuffer* load_png(const fs::path& file_path);

    bool write_png(const fs::path& file_path, unsigned char* pixels, int w, int h);
};

}

#endif // PNG_LOADER_H
