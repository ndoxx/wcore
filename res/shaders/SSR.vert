#version 400 core

struct render_data
{
    vec2 v2_viewportSize;
    vec2 v2_texelSize;

    mat4 m4_projection;
    mat4 m4_invView;
    float f_near;
    float f_minGlossiness;
    //float f_pixelThickness;
    float f_maxRayDistance;
    float f_pixelStride;         // number of pixels per ray step close to camera
    float f_pixelStrideZCuttoff; // ray origin Z at this distance will have a pixel stride of 1.0
    float f_iterations;
    float f_binSearchIterations;
    float f_screenEdgeFadeStart; // distance to screen edge that ray hits will start to fade (0.0 -> 1.0)
    float f_eyeFadeStart;        // ray direction's Z that ray hits will start to fade (0.0 -> 1.0)
    float f_eyeFadeEnd;          // ray direction's Z that ray hits will be cut (0.0 -> 1.0)
    float f_ditherAmount;

    float f_probe; // dbg

    // Position reconstruction
    vec4 v4_proj_params;
};
uniform render_data rd;

layout(location = 0) in vec3 in_position;

out vec2 frag_ray;

void main()
{
    vec4 clip_pos = vec4(in_position, 1.0);
    frag_ray = clip_pos.xy*rd.v4_proj_params.xy;
    gl_Position = clip_pos;
}
