vec3 reconstruct_position(in float depth, in vec2 ray, in vec4 projParams)
{
    float depth_ndc_offset = depth * 2.0f + projParams.z;
    float depth_view = projParams.w / depth_ndc_offset;
    return depth_view * vec3(ray, -1.0f);
}

vec3 reconstruct_position(in sampler2D depthMap, in vec2 texCoords, in vec2 ray, in vec4 projParams)
{
    float depth = texture(depthMap, texCoords).r;
    float depth_ndc_offset = depth * 2.0f + projParams.z;
    float depth_view = projParams.w / depth_ndc_offset;
    return depth_view * vec3(ray, -1.0f);
}

// projParams = vec2(P(2,2)-1.0f, P(2,3))
float depth_view_from_tex(in sampler2D depthTex, in vec2 texCoords, in vec2 projParams)
{
    // Get screen space depth at coords
    float depth_raw = texture2D(depthTex, texCoords).r;
    // Convert to NDC depth (*2-1) and add P(2,2)
    float depth_ndc_offset = depth_raw * 2.0f + projParams.x;
    // Return positive linear depth
    return projParams.y / depth_ndc_offset;
}
