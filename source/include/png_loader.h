#ifndef PNG_LOADER_H
#define PNG_LOADER_H

namespace wcore
{

class PixelBuffer;
class PngLoader
{
public:
    PngLoader() = default;
    ~PngLoader() = default;

    PixelBuffer* load_png(const char* filename);

};

}

#endif // PNG_LOADER_H
