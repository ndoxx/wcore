#ifndef WCORE_H
#define WCORE_H

#include <memory>
#include <cstdint>

#include "wapi.h"
#include "wtypes.h"
#include "wecs.h"

using wcore::H_;

namespace wcore
{
    // Globals access
    extern "C" void WAPI GlobalsSet(hash_t name, const void* data);

    class WAPI Engine
    {
    public:
        Engine();
        ~Engine();

        void Init(int argc, char const *argv[], void(*parse_arguments)(int, char const **)=nullptr);
        // Load first level, load and send chunk geometry near to camera start position
        void LoadStart();
        // Load first level global info, no chunk is loaded
        void LoadLevel();
        uint32_t LoadChunk(uint32_t xx, uint32_t zz, bool send_geometry=true);
        void LoadModel(hash_t name, uint32_t chunk_index);
        void SendChunk(uint32_t xx, uint32_t zz);
        int Run();

    private:
        struct EngineImpl;
        std::unique_ptr<EngineImpl> eimpl_; // opaque pointer
    };
}

#endif // WCORE_H
