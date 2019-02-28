#version 400 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_texCoord;

struct Transform
{
    mat4 m4_ModelView;
    mat4 m4_ModelViewProjection;
    mat3 m3_Normal; // Normal matrix to transform normals
};

struct render_data
{
    float f_wireframe_mix;
    vec3 v3_viewPos;
};

out vec3 vertex_pos;
out vec3 vertex_normal;
out vec3 tangent_viewDir;
out vec2 vertex_texCoord;
out mat3 vertex_TBN;

#ifdef VARIANT_SPLAT
out vec2 landscape_coord;
#endif

// UNIFORMS
uniform Transform tr;
uniform render_data rd;

#ifdef VARIANT_SPLAT
uniform float f_inv_chunk_size;
#endif

void main()
{
    vec4 viewPos   = tr.m4_ModelView * vec4(in_position, 1.0);
    vertex_pos  = viewPos.xyz/viewPos.w;
    vertex_texCoord = in_texCoord;

    vec3 N = normalize(tr.m3_Normal * in_normal);
    vec3 T = normalize(tr.m3_Normal * in_tangent);
    // re-orthogonalize T with respect to N (Gram-Schmidt process)
    T = normalize(T - dot(T, N)*N);
    vec3 B = -cross(N, T); // Apparently minus is needed for normal/parallax mapping to work correctly
    vertex_TBN = mat3(T,B,N);
    mat3 TBN_inv = transpose(vertex_TBN);

    vertex_normal = N;
    tangent_viewDir = normalize(TBN_inv * vertex_pos);

    #ifdef VARIANT_SPLAT
    landscape_coord = in_position.xz * f_inv_chunk_size;
    #endif

    gl_Position = tr.m4_ModelViewProjection * vec4(in_position,1.0);
}
