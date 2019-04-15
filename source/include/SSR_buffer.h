#ifndef SSR_BUFFER_H
#define SSR_BUFFER_H

#include "buffer_module.h"
#include "singleton.hpp"
#include "math3d.h"

namespace wcore
{

class SSRBuffer : public BufferModule, public SingletonNDI<SSRBuffer>
{
public:
    friend SSRBuffer& SingletonNDI<SSRBuffer>::Instance();
    friend void SingletonNDI<SSRBuffer>::Kill();
    template <class T, typename ...Args>
    friend void SingletonNDI<T>::Init(Args&&... args);
private:
    SSRBuffer(unsigned int screenWidth,
              unsigned int screenHeight);
   virtual ~SSRBuffer();
};

}

#endif
