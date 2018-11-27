#ifndef PNG_LOADER_H
#define PNG_LOADER_H

#include <filesystem>

namespace wcore
{

namespace fs = std::filesystem;

class PixelBuffer;
class PngLoader
{
public:
    PngLoader() = default;
    ~PngLoader() = default;

    PixelBuffer* load_png(const fs::path& file_path);

};

}

#endif // PNG_LOADER_H
