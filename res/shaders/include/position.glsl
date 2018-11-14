vec3 reconstruct_position(in float depth, in vec2 ray, in vec4 projParams)
{
    float depth_ndc_offset = depth * 2.0f + projParams.z;
    float depth_view = projParams.w / depth_ndc_offset;
    return depth_view * vec3(ray, -1.0f);
}
