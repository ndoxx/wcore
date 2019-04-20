#ifndef G_BUFFER_H
#define G_BUFFER_H

#include "buffer_module.h"
#include "singleton.hpp"
#include "math3d.h"

namespace wcore
{

class GBuffer : public BufferModule, public SingletonNDI<GBuffer>
{
public:
    friend GBuffer& SingletonNDI<GBuffer>::Instance();
    friend void SingletonNDI<GBuffer>::Kill();
    template <class T, typename ...Args>
    friend void SingletonNDI<T>::Init(Args&&... args);
private:
    GBuffer(unsigned int screenWidth,
            unsigned int screenHeight);
   virtual ~GBuffer();
};

class BackFaceDepthBuffer : public BufferModule, public SingletonNDI<BackFaceDepthBuffer>
{
public:
    friend BackFaceDepthBuffer& SingletonNDI<BackFaceDepthBuffer>::Instance();
    friend void SingletonNDI<BackFaceDepthBuffer>::Kill();
    template <class T, typename ...Args>
    friend void SingletonNDI<T>::Init(Args&&... args);
private:
    BackFaceDepthBuffer(unsigned int screenWidth,
                        unsigned int screenHeight);
   virtual ~BackFaceDepthBuffer();
};

}

#endif // G_BUFFER_H
