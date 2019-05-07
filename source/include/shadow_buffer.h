#ifndef SHADOW_BUFFER_H
#define SHADOW_BUFFER_H

#include "buffer_module.h"
#include "singleton.hpp"
#include "math3d.h"

namespace wcore
{

class ShadowBuffer : public BufferModule, public SingletonNDI<ShadowBuffer>
{
public:
    friend ShadowBuffer& SingletonNDI<ShadowBuffer>::Instance();
    friend void SingletonNDI<ShadowBuffer>::Kill();
    template <class T, typename ...Args>
    friend void SingletonNDI<T>::Init(Args&&... args);
private:
    ShadowBuffer(unsigned int width,
                 unsigned int height);
    virtual ~ShadowBuffer();
};

#define SHADOWBUFFER ShadowBuffer::Instance()

}

#endif // SHADOW_BUFFER_H
