// PCF soft shadows
const float NUM_SAMPLES = 3.0f;
const float SAMPLES_START = (NUM_SAMPLES-1.0f)/2.0f;
const float NUM_SAMPLES_SQUARED = NUM_SAMPLES*NUM_SAMPLES;

float sample_shadow_map(sampler2D shadowM, vec2 coords, float compare)
{
    return step(compare, texture(shadowM, coords).r);
}

float sample_shadow_map_linear(sampler2D shadowM, vec2 coords, float compare, vec2 texelSize)
{
    vec2 pixelPos = coords/texelSize + vec2(0.5f);
    vec2 fracPart = fract(pixelPos);
    vec2 startTexel = (pixelPos - fracPart) * texelSize;

    float blTexel = sample_shadow_map(shadowM, startTexel, compare);
    float brTexel = sample_shadow_map(shadowM, startTexel + vec2(texelSize.x, 0.0f), compare);
    float tlTexel = sample_shadow_map(shadowM, startTexel + vec2(0.0f, texelSize.y), compare);
    float trTexel = sample_shadow_map(shadowM, startTexel + texelSize, compare);

    float mixA = mix(blTexel, tlTexel, fracPart.y);
    float mixB = mix(brTexel, trTexel, fracPart.y);

    return mix(mixA, mixB, fracPart.x);
}

float sample_shadow_map_PCF(sampler2D shadowM, vec2 coords, float compare, vec2 texelSize)
{
    float result = 0.0f;
    for(float y = -SAMPLES_START; y <= SAMPLES_START; y += 1.0f)
    {
        for(float x = -SAMPLES_START; x <= SAMPLES_START; x += 1.0f)
        {
            vec2 coordsOffset = vec2(x,y)*texelSize;
            result += sample_shadow_map_linear(shadowM, coords + coordsOffset, compare, texelSize);
        }
    }
    return result/NUM_SAMPLES_SQUARED;
}

vec2 poissonDisk[16] = vec2[](
   vec2( -0.94201624, -0.39906216 ),
   vec2( 0.94558609, -0.76890725 ),
   vec2( -0.094184101, -0.92938870 ),
   vec2( 0.34495938, 0.29387760 ),
   vec2( -0.91588581, 0.45771432 ),
   vec2( -0.81544232, -0.87912464 ),
   vec2( -0.38277543, 0.27676845 ),
   vec2( 0.97484398, 0.75648379 ),
   vec2( 0.44323325, -0.97511554 ),
   vec2( 0.53742981, -0.47373420 ),
   vec2( -0.26496911, -0.41893023 ),
   vec2( 0.79197514, 0.19090188 ),
   vec2( -0.24188840, 0.99706507 ),
   vec2( -0.81409955, 0.91437590 ),
   vec2( 0.19984126, 0.78641367 ),
   vec2( 0.14383161, -0.14100790 )
);

vec2 random_sample(vec3 seed, int i)
{
    vec4 seed4 = vec4(seed,i);
    float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
    float rnd = fract(sin(dot_product) * 43758.5453);
    int index = int(16.0f*rnd)%16;
    return poissonDisk[index];
}

float sample_shadow_map_PCF_Poisson(sampler2D shadowM, vec2 coords, vec3 world_coords, float compare, vec2 texelSize)
{
    float result = 0.0f;

    for(int ii=0; ii<NUM_SAMPLES; ++ii)
    {
        vec2 coordsOffset = random_sample(world_coords.xyy, ii)*texelSize;
        result += sample_shadow_map(shadowM, coords + coordsOffset, compare);
    }

    return result/NUM_SAMPLES;
}

bool InRange(float val)
{
    return val > 0.0f && val < 1.0f;
}

float shadow_amount(sampler2D shadowM, vec3 shadowMapCoords, float bias, vec2 shadowTexelSize)
{
    if(InRange(shadowMapCoords.z) && InRange(shadowMapCoords.x) && InRange(shadowMapCoords.y))
    {
        return clamp(sample_shadow_map_PCF(shadowM, shadowMapCoords.xy, shadowMapCoords.z-bias, shadowTexelSize),0.0f,1.0f);
    }
    else
    {
        return 1.0f;
    }
}

float shadow_amount_Poisson(sampler2D shadowM, vec3 shadowMapCoords, vec3 fragWorld, float bias, vec2 shadowTexelSize)
{
    if(InRange(shadowMapCoords.z) && InRange(shadowMapCoords.x) && InRange(shadowMapCoords.y))
    {
        return clamp(sample_shadow_map_PCF_Poisson(shadowM, shadowMapCoords.xy, fragWorld, shadowMapCoords.z-bias, shadowTexelSize),0.0f,1.0f);
    }
    else
    {
        return 1.0f;
    }
}

float Chebyshev_upper_bound(sampler2D shadowM, vec2 coords, float distance)
{
    vec2 moments = texture(shadowM, coords).xy;

    // The fragment is either in shadow or penumbra. We now use chebyshev's upperBound to check
    // How likely this pixel is to be lit (p_max)
    float variance = moments.x*moments.x - moments.y;
    variance = clamp(variance, 0.002f, 1.0f);

    float d = distance - moments.x;
    float p_max = variance / (d*d + variance);

    return max(1.0f-p_max, float(distance <= moments.x));
}

float shadow_variance(sampler2D shadowM, vec3 shadowMapCoords)
{
    if(InRange(shadowMapCoords.z) && InRange(shadowMapCoords.x) && InRange(shadowMapCoords.y))
    {
        float visibility = Chebyshev_upper_bound(shadowM, shadowMapCoords.xy, shadowMapCoords.z);
        //return 1.0f/(1.0f+exp(-50.0f*(visibility-0.9f)));
        return visibility;
        //return pow(visibility,5);
    }
    else
    {
        return 1.0f;
    }
}
