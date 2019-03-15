#ifndef WCORE_H
#define WCORE_H

#include <memory>
#include <cstdint>

#include "wapi.h"
#include "wtypes.h"
#include "wecs.h"
#include "wcontext.h"

#include "include/math3d.h"

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

        // Register a resource archive
        bool UseResourceArchive(const char* filename, hash_t key);
        // Register and launch systems
        // Can pass a context here or leave it null. If let null, a
        // GLFW context will be created automatically
        void Init(int argc,
                  char const *argv[],
                  void(*parse_arguments)(int, char const **)=nullptr,
                  AbstractContext* context=nullptr);
        // Load first level, load and send chunk geometry near to camera start position
        void LoadStart();
        // Load first level global info, no chunk is loaded
        void LoadLevel();
        uint32_t LoadChunk(uint32_t xx, uint32_t zz, bool send_geometry=true);
        void SendChunk(uint32_t xx, uint32_t zz);


        uint32_t LoadModel(hash_t name, uint32_t chunk_index);
        void SetModelPosition(uint32_t model_index, const math::vec3& position);
        void SetModelOrientation(uint32_t model_index, const math::vec3& orientation);
        void SetModelScale(uint32_t model_index, float scale);

        uint32_t LoadPointLight(uint32_t chunk_index);
        void SetLightPosition(uint32_t light_index, const math::vec3& value);
        void SetLightColor(uint32_t light_index, const math::vec3& value);
        void SetLightRadius(uint32_t light_index, float value);
        void SetLightBrightness(uint32_t light_index, float value);

        // Automated game loop with frame control
        int Run();

        // Functions to handle game loop from outside
        void Update(float dt);
        void RenderFrame();
        void FinishFrame();
        bool WindowRequired();

        // To set the default framebuffer to render to in case it is non-zero
        void SetDefaultFrameBuffer(unsigned int index);

    private:
        struct EngineImpl;
        std::unique_ptr<EngineImpl> eimpl_; // opaque pointer
    };
}

#endif // WCORE_H
