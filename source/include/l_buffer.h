#ifndef SCREEN_RENDERER_H
#define SCREEN_RENDERER_H

#include "singleton.hpp"
#include "buffer_module.h"

namespace wcore
{

class LBuffer : public BufferModule, public SingletonNDI<LBuffer>
{
public:
    friend LBuffer& SingletonNDI<LBuffer>::Instance();
    friend void SingletonNDI<LBuffer>::Kill();
    template <class T, typename ...Args>
    friend void SingletonNDI<T>::Init(Args&&... args);
private:
    LBuffer(unsigned int screenWidth,
            unsigned int screenHeight);
   virtual ~LBuffer();
};

#define LBUFFER LBuffer::Instance()

}

#endif // SCREEN_RENDERER_H
