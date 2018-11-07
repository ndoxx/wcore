#version 400 core

struct render_data
{
    float f_wireframe_mix;    // Wireframe blend factor in[0,1]
    float f_bright_threshold; // For bloom bright pass
    float f_fogDensity;
    vec3 v3_viewPos;
    vec3 v3_fogColor;
    bool b_enableFog;
    vec2 v2_screenSize;
};

struct dir_light
{
    vec3 v3_lightPosition;
    vec3 v3_lightColor;
};

struct point_light
{
    vec3 v3_lightPosition;
    vec3 v3_lightColor;
    float f_light_radius;
};
#define N_PT_LIGHTS 5

layout(location = 0) out vec3 out_color;
layout(location = 1) out vec3 out_bright_color;

// G-Buffer samplers
uniform sampler2D positionTex;
uniform sampler2D normalTex;
uniform sampler2D albedoSpecTex;
uniform sampler2D propsTex;

uniform render_data rd;
uniform dir_light   dl;
uniform point_light pl[N_PT_LIGHTS];

// CONSTANTS
// Relative luminance coefficients for sRGB primaries, values from Wikipedia
const vec3 W = vec3(0.2126, 0.7152, 0.0722);


vec3 blinn_phong(vec3 lightColor,
                 vec3 lightDir,
                 vec3 normal,
                 vec3 viewDir,
                 vec3 albedo,
                 float shininess,
                 float specInt,
                 float lumaCorrect);
vec3 decompress_normal(vec2 norm);
float attenuate(float distance, float radius);
void main()
{
    vec2 texCoord = gl_FragCoord.xy / rd.v2_screenSize;

    vec3 fragPos = texture(positionTex, texCoord).xyz;
    vec3 fragNormal = decompress_normal(texture(normalTex, texCoord).xy);
    vec3 fragAlbedo = texture(albedoSpecTex, texCoord).rgb;
    float fragSpecIntensity = texture(albedoSpecTex, texCoord).a;
    float fragShininess = 8.0+128.0*texture(propsTex, texCoord).r;
    float fragOverlay = texture(propsTex, texCoord).g;

    // View direction as a vector from fragment position to camera position
    vec3 viewDir = normalize(rd.v3_viewPos - fragPos);


    // Directional light constribution
    vec3 directional_contrib = blinn_phong(dl.v3_lightColor, dl.v3_lightPosition,
            fragNormal, viewDir, fragAlbedo, fragShininess, fragSpecIntensity, 1.0);
    // Point lights contribution
    vec3 points_contrib = vec3(0.0,0.0,0.0);
    for(int ii=0; ii<N_PT_LIGHTS; ++ii)
    {
        float distance = length(pl[ii].v3_lightPosition - fragPos);
        if(distance < pl[ii].f_light_radius)
        {
            // HACK attenuation function makes specular too dim, boost em up a bit
            float luma_correct = 0.7*dot(W,pl[ii].v3_lightColor);
            vec3 point_contrib = blinn_phong(pl[ii].v3_lightColor, pl[ii].v3_lightPosition - fragPos,
                fragNormal, viewDir, fragAlbedo, fragShininess, fragSpecIntensity, luma_correct);

            // attenuation
            float distance = length(pl[ii].v3_lightPosition - fragPos);
            points_contrib += point_contrib * attenuate(distance, pl[ii].f_light_radius);
        }
    }

    vec3 total_light = directional_contrib + points_contrib;

    // Fog
    if(rd.b_enableFog)
    {
        float dist      = length(fragPos - rd.v3_viewPos);
        float fogFactor = 1.0/exp(pow((dist * rd.f_fogDensity),3));
        fogFactor       = clamp(fogFactor, 0.0, 1.0);
        total_light = mix(rd.v3_fogColor, total_light, fogFactor);
    }

    // Debug overlay & wireframe
    total_light += fragOverlay * vec3(1.0, 0.4, 0.0);

    out_color = total_light;

    // "Bright pass"
    float luminance = dot(out_color, W);
    //float brightnessMask = float(luminance > rd.f_bright_threshold); // Step function
    //float brightnessMask = 1/(1+exp(-20*(luminance-rd.f_bright_threshold))); // Sigmoid logistic function
    float brightnessMask = (1+tanh(20*(luminance-rd.f_bright_threshold)))/2; // Sigmoid hyperbolic tangent

    out_bright_color = brightnessMask*out_color;
}

vec3 blinn_phong(vec3 lightColor,
                 vec3 lightDir,
                 vec3 normal,
                 vec3 viewDir,
                 vec3 albedo,
                 float shininess,
                 float specInt,
                 float lumaCorrect)
{
    lightDir = normalize(lightDir);
    // Halfway vector
    vec3 halfwayDir = normalize(lightDir + viewDir);
    // Diffuse shading factor
    float diff = max(dot(normal, lightDir), 0.0);
    // Modified energy conservative Blinn-Phong specular factor
    float spec = (1.0-diff)*pow(max(dot(normal, halfwayDir), 0.0), shininess);
    spec = clamp(spec,0.0,1.0);

    // combine results
    vec3 ambient  = 0.05*lightColor * albedo;
    vec3 diffuse  = lightColor * diff * albedo * lumaCorrect;
    vec3 specular = lightColor * spec * specInt * lumaCorrect;

    return ambient + diffuse + specular;
}

// Compute light attenuation for point lights
float attenuate(float distance, float radius)
{
    // originally pow(,20) but I found it fucks up specular reflection tails
    return (1.0 - pow(distance/radius, 5))/(1.0 + distance*distance);
}

// Decompress a 2D normal back to 3 components
vec3 decompress_normal(vec2 norm)
{
    float z2 = 1.0 - pow(length(norm), 2);
    float z  = sqrt(z2);
    return vec3(2*z*norm.x, 2*z*norm.y, 2*z2-1.0);
}
