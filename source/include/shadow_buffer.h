#ifndef SHADOW_BUFFER_H
#define SHADOW_BUFFER_H

#include "buffer_module.h"
#include "math3d.h"

namespace wcore
{

class ShadowBuffer : public BufferModule
{
public:
    ShadowBuffer(unsigned int width,
                 unsigned int height);
    virtual ~ShadowBuffer();
};

}

#endif // SHADOW_BUFFER_H
