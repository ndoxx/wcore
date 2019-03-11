// CONSTANTS
const float PARALLAX_MIN_LAYERS = 8.0f;
const float PARALLAX_MAX_LAYERS = 32.0f;
const float PARALLAX_LOD_THRESHOLD = 2.0f;

vec2 parallax_map(vec2 texCoords, vec3 viewDir, float f_parallax_height_scale, sampler2D depthTex)
{
    // Adaptive layer density (moar angle, moar layers)
    // In tangent space, normal is (0,0,1) so scalar product of viewDir and normal
    // is just z component of viewDir.
    float n_layers = mix(PARALLAX_MIN_LAYERS, PARALLAX_MAX_LAYERS, abs(viewDir.z));
    float layer_depth = 1.0f/n_layers;

    // The layer depth we're at
    float cur_layer_depth = 0.0f;
    vec2 delta_texCoords = viewDir.xy * (f_parallax_height_scale * layer_depth);

    vec2 cur_texCoords = texCoords;
    //float cur_depthValue = texture(depthTex, cur_texCoords).r;
    float cur_depthValue = texture(depthTex, cur_texCoords).a;
    float prev_depthValue = 0.0f;

    while(cur_layer_depth < cur_depthValue)
    {
        prev_depthValue = cur_depthValue;
        // Shift texture coordinates along direction of P
        cur_texCoords -= delta_texCoords;
        // Get depthmap value at current texture coordinates
        //cur_depthValue = texture(depthTex, cur_texCoords).r;
        cur_depthValue = texture(depthTex, cur_texCoords).a;
        // Get depth of next layer
        cur_layer_depth += layer_depth;
    }

    // Get texture coordinates before ray intersection (reverse operations)
    vec2 prev_texCoords = cur_texCoords + delta_texCoords;

    // Get depth after and before intersection
    float afterDepth  = cur_depthValue - cur_layer_depth;
    float beforeDepth = prev_depthValue - cur_layer_depth + layer_depth;

    // Interpolate texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 final_texCoords = mix(prev_texCoords, cur_texCoords, weight);

    return final_texCoords;
}
