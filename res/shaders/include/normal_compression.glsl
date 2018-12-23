// Function to compress a 3D normal to 2 components
vec2 compress_normal(vec3 norm)
{
    return normalize(norm+vec3(0.0,0.0,1.0)).xy;
}

// Decompress a 2D normal back to 3 components
vec3 decompress_normal(vec2 norm)
{
    float z2 = -dot(norm, norm) + 1.0f;
    float z  = sqrt(z2);
    return vec3(2*z*norm, 2*z2-1.0);
}
