float square_falloff(vec2 coords, float curvature_exponent)
{
    return clamp(pow(16.0*coords.x*coords.y*(1.0f-coords.x)*(1.0f-coords.y), curvature_exponent), 0.0f, 1.0f);
}

float cube_falloff(vec3 coords, float curvature_exponent)
{
    return clamp(pow(64.0*coords.x*coords.y*coords.z*(1.0f-coords.x)*(1.0f-coords.y)*(1.0f-coords.z), curvature_exponent), 0.0f, 1.0f);
}

float square_falloff_2(vec2 coords, float curvature_exponent)
{
    return clamp(1.0f-(pow(2*coords.x-1,curvature_exponent)+pow(2*coords.y-1,curvature_exponent)),0.0f,1.0f);
}

float rand(float n)
{
    return fract(sin(n) * 43758.5453123);
}

float rand(vec2 c)
{
    return fract(sin(dot(c.xy ,vec2(12.9898,78.233))) * 43758.5453);
}
