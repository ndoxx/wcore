#ifndef WCORE_H
#define WCORE_H

#include <memory>
#include <functional>
#include <cstdint>

#include "wapi.h"
#include "wtypes.h"
#include "wecs.h"
#include "wcontext.h"

#include "include/math3d.h"

using wcore::H_;

namespace wcore
{

extern "C" void WAPI SetGlobal(hash_t name, const void* data);

class Model;
class WAPI Engine
{
private:
    struct EngineImpl;
    class SceneControl;
    class PipelineControl;

public:
    Engine();
    ~Engine();

    // * Global state access
    void SetFrameSize(uint32_t width, uint32_t height, bool fullscreen=false);
    void SetWindowSize(uint32_t width, uint32_t height);

    // * Base API
    // Register a resource archive
    bool UseResourceArchive(const char* filename, hash_t key);
    // Register and launch systems
    // Can pass a context here or leave it null. If let null, a
    // GLFW context will be created automatically
    void Init(int argc,
              char const *argv[],
              void(*parse_arguments)(int, char const **)=nullptr,
              AbstractContext* context=nullptr);

    // Automated game loop with frame control
    int Run();
    // * Functions to handle game loop from outside
    // Update all game systems
    void Update(float dt);
    // Draw frame to default framebuffer
    void RenderFrame();
    // Swap buffers and poll events. May not be needed if a non GLFW context is used
    void FinishFrame();
    // Return true if the application needs not be closed.
    bool WindowRequired();

public:
    SceneControl* scene;
    PipelineControl* pipeline;

private:
    std::shared_ptr<EngineImpl> eimpl_; // opaque pointer
};

class WAPI Engine::SceneControl
{
public:
    SceneControl(std::shared_ptr<EngineImpl> impl);

    // Set a level name to load with LoadStart
    void SetStartLevel(const char* level_name);
    // Load first level, load and send chunk geometry near to camera start position
    void LoadStart(const char* level_name=nullptr);
    // Load first level global info, no chunk is loaded
    void LoadLevel(const char* level_name=nullptr);
    // Load chunk at given chunk coordinates, send geometry to graphics driver if needed
    uint32_t LoadChunk(uint32_t xx, uint32_t zz, bool send_geometry=true);
    // Send chunk geometry to graphics driver
    void SendChunk(uint32_t xx, uint32_t zz);
    // Visit referenced model in scene by hash name
    bool VisitModelRef(hash_t href, std::function<void(Model& model)> visit);

    // * The following functions will likely be deprecated soon
    void LoadModel(hash_t name, uint32_t chunk_index, hash_t href=0);
    /*void SetModelPosition(uint32_t model_index, const math::vec3& position);
    void SetModelOrientation(uint32_t model_index, const math::vec3& orientation);
    void SetModelScale(uint32_t model_index, float scale);*/

    uint32_t LoadPointLight(uint32_t chunk_index);
    void SetLightPosition(uint32_t light_index, const math::vec3& value);
    void SetLightColor(uint32_t light_index, const math::vec3& value);
    void SetLightRadius(uint32_t light_index, float value);
    void SetLightBrightness(uint32_t light_index, float value);

private:
    std::shared_ptr<EngineImpl> eimpl_; // opaque pointer
};

class WAPI Engine::PipelineControl
{
public:
    PipelineControl(std::shared_ptr<EngineImpl> impl);

    // Set the default framebuffer to render to, in case it is non-zero
    void SetDefaultFrameBuffer(unsigned int index);
    // Enable/Disable shadow mapping
    void SetShadowMappingEnabled(bool value);
    // Enable/Disable bloom effect
    void SetBloomEnabled(bool value);
    // Enable/Disable fog
    void SetFogEnabled(bool value);
    // Enable/Disable FXAA
    void SetFXAAEnabled(bool value);

    // * Lighting control
    // Set the amount of light necessary to trigger the bloom effect locally
    void SetLightingBrightThreshold(float value);
    // Set the transition softness around the bright threshold
    void SetLightingBrightKnee(float value);
    // Set the amout of slope bias to be used with shadow mapping
    void SetShadowSlopeBias(float value);
    // Set the amout of bias to be used with shadow mapping
    void SetShadowBias(float value);
    // Set the amout of normal offset to be used with shadow mapping
    void SetShadowNormalOffset(float value);

    // * Post processing control
    // Set exposure tone mapping parameter
    void SetPostprocExposure(float value);
    // Set contrast parameter
    void SetPostprocContrast(float value);
    // Set color vibrance
    void SetPostprocVibrance(float value);
    // Set the radial falloff of the vignette post processing effect
    void SetPostprocVignetteFalloff(float value);
    // Set the overall intensity of the vignette post processing effect
    void SetPostprocVignetteBalance(float value);
    // Set the amount of distance shift caused by chromatic aberration
    void SetPostprocAberrationShift(float value);
    // Set the intensity of chromatic aberration
    void SetPostprocAberrationStrength(float value);
    // Control how much each color channel is affected by the vibrance effect
    void SetPostprocVibranceBalance(const math::vec3& value);

    // * Accessibility
    // Set the type of color blindness to correct / simulate
    void SetPostprocAccBlindnessType(int value);

    // * Debug / Editor helper functions
#ifdef __DEBUG__
    void dDrawSegment(const math::vec3& world_start,
                      const math::vec3& world_end,
                      int ttl = 60,
                      const math::vec3& color = math::vec3(0,1,0));
    void dDrawSphere(const math::vec3& world_pos,
                     float radius = 1.f,
                     int ttl = 60,
                     const math::vec3& color = math::vec3(0,1,0));
    void dDrawCross3(const math::vec3& world_pos,
                     float radius = 1.f,
                     int ttl = 60,
                     const math::vec3& color = math::vec3(0,1,0));
#endif

private:
    std::shared_ptr<EngineImpl> eimpl_; // opaque pointer
};

}

#endif // WCORE_H
