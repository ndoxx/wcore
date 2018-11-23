#ifndef PNG_LOADER_H
#define PNG_LOADER_H

#include "singleton.hpp"

namespace wcore
{

class PixelBuffer;
class PngLoader : public Singleton<PngLoader>
{
public:
    friend PngLoader& Singleton<PngLoader>::Instance();
    friend void Singleton<PngLoader>::Kill();
private:
    PngLoader (const PngLoader&)=delete;
    PngLoader();
   ~PngLoader();

public:
    PixelBuffer* load_png(const char* filename);

};

}

#endif // PNG_LOADER_H
