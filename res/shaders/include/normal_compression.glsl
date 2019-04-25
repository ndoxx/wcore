//#define NORMAL_COMPRESSION_Z_RECONSTRUCT
#define NORMAL_COMPRESSION_SPHEREMAP_TRANSFORM
//#define NORMAL_COMPRESSION_LAMBERT_AZIMUTHAL

#ifdef NORMAL_COMPRESSION_Z_RECONSTRUCT
// Function to compress a 3D normal to 2 components
vec2 compress_normal(vec3 norm)
{
    return normalize(norm + vec3(0.f,0.f,1.f)).xy;
}
// Decompress a 2D normal back to 3 components
vec3 decompress_normal(vec2 norm)
{
    float z2 = 1.f - dot(norm, norm);
    float z  = sqrt(z2);
    return vec3(2.f*z*norm, 2.f*z2-1.f);
}
#endif

#ifdef NORMAL_COMPRESSION_SPHEREMAP_TRANSFORM
vec2 compress_normal(vec3 norm)
{
    vec2 enc = normalize(norm.xy) * (sqrt(-norm.z * 0.5f + 0.5f));
    return enc * 0.5f + 0.5f;
}
vec3 decompress_normal(vec2 norm)
{
    vec4 nn = vec4(2.f * norm.xy - 1.f, 1.f, -1.f);
    float l = dot(nn.xyz,-nn.xyw);
    nn.z = l;
    nn.xy *= sqrt(l);
    return nn.xyz * 2.f + vec3(0.f,0.f,-1.f);
}
#endif

#ifdef NORMAL_COMPRESSION_LAMBERT_AZIMUTHAL
vec2 compress_normal(vec3 norm)
{
    float f = 1.f / sqrt(8.f * norm.z + 8.f);
    return norm.xy * f + 0.5f;
}
vec3 decompress_normal(vec2 norm)
{
    vec2 fenc = norm * 4.f - 2.f;
    float f = dot(fenc,fenc);
    float g = sqrt(f * -0.25f + 1.f);
    vec3 n;
    n.xy = fenc * g;
    n.z = f * -0.5f + 1.f;
    return n;
}
#endif
