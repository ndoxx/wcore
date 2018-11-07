#ifndef SSAO_BUFFER_H
#define SSAO_BUFFER_H

#include "buffer_module.h"
#include "singleton.hpp"
#include "math3d.h"

class SSAOBuffer : public BufferModule, public SingletonNDI<SSAOBuffer>
{
public:
    friend SSAOBuffer& SingletonNDI<SSAOBuffer>::Instance();
    friend void SingletonNDI<SSAOBuffer>::Kill();
    template <class T, typename ...Args>
    friend void SingletonNDI<T>::Init(Args&&... args);
private:
    SSAOBuffer(unsigned int screenWidth,
               unsigned int screenHeight);
   virtual ~SSAOBuffer();
};

#endif // SSAO_BUFFER_H
