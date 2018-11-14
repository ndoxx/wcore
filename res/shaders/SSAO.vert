#version 400 core

struct render_data
{
    vec2 v2_screenSize;
    vec2 v2_noiseScale;
    vec3 v3_lightDir;

    float f_radius;
    float f_intensity;
    float f_scale;
    float f_bias;

    bool b_invert_normals;
#ifdef __EXPERIMENTAL_POS_RECONSTRUCTION__
    // Position reconstruction
    vec4 v4_proj_params;
#endif
};
uniform render_data rd;

layout(location = 0) in vec3 in_position;

#ifdef __EXPERIMENTAL_POS_RECONSTRUCTION__
    out vec2 frag_ray;
#endif

void main()
{
    vec4 clip_pos = vec4(in_position, 1.0);
    #ifdef __EXPERIMENTAL_POS_RECONSTRUCTION__
        frag_ray = clip_pos.xy*rd.v4_proj_params.xy;
    #endif
    gl_Position = clip_pos;
}
