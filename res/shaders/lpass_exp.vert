#version 400 core
layout(location = 0) in vec3 in_position;

struct render_data
{
    float f_wireframe_mix;    // Wireframe blend factor in[0,1]
    float f_bright_threshold; // For bloom bright pass
    float f_bright_knee;      // For bloom bright pass
    vec3 v3_viewPos;
    vec2 v2_screenSize;
    // Shadow
    float f_shadowBias;
    vec2 v2_shadowTexelSize;
    bool b_shadow_enabled;
    // Ambient occlusion
    bool b_enableSSAO;
    // Reflections
    bool b_enableSSR;
    // Lighting
    bool b_lighting_enabled;
    // Position reconstruction
    vec4 v4_proj_params;
};

uniform render_data rd;
uniform mat4 m4_ModelViewProjection;
#ifdef VARIANT_DIRECTIONAL
    out vec2 frag_ray;
#endif

void main()
{
    vec4 clip_pos = m4_ModelViewProjection * vec4(in_position, 1.0);
    #ifdef VARIANT_DIRECTIONAL
        frag_ray = clip_pos.xy / clip_pos.w;
        frag_ray *= rd.v4_proj_params.xy;
    #endif
    gl_Position = clip_pos;
}
