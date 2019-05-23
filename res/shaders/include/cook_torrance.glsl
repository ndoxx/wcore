const float PI = 3.14159265359;
const float invPI = 0.318309886;

float TrowbridgeReitzGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH*NdotH;

    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * (denom * denom);

    return max(num / denom, 0.001);
}

float SchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0f;

    float k = (r*r) * 0.125f;

    float num   = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return num / denom;

    /*float k = r*r;
    float NdotVinv = 1.0f/NdotV;

    return 8.0f / (k*NdotVinv + (8.0f-k));*/
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx2  = SchlickGGX(NdotV, roughness);
    float ggx1  = SchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Fresnel-Schlick approx.
vec3 FresnelSchlick(float VdotH, vec3 F0)
{
    float flipVdotH = 1.0f - VdotH;
    return ((1.0f - F0) * (flipVdotH*flipVdotH*flipVdotH*flipVdotH*flipVdotH)) + F0;
}

// Gaussian Spherical approx.
vec3 FresnelGS(float VdotH, vec3 F0)
{
    return ((1.0f - F0) * pow(2.0f, (-5.55473f*VdotH - 6.98316f)*VdotH)) + F0;
}

// BRDF
vec3 CookTorrance(vec3 lightColor,
                  vec3 lightDir,
                  vec3 normal,
                  vec3 viewDir,
                  vec3 albedo,
                  float fragMetallic,
                  float fragRoughness)
{
    // Calculate reflectance at normal incidence; if dielectric (like plastic) use F0
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
    // Actual formula for F0 is: pow(abs((1.0-ior) / (1.0+ior)), 2.0) with ior= index of refraction
    vec3 F0 = vec3(0.04f);
    F0 = mix(F0, albedo, fragMetallic);

    // calculate light radiance
    vec3 halfwayDir = normalize(viewDir + lightDir);
    vec3 radiance = lightColor;

    // Cook-Torrance BRDF
    float D = TrowbridgeReitzGGX(normal, halfwayDir, fragRoughness);
    vec3  F = FresnelGS(max(dot(halfwayDir, viewDir), 0.0f), F0);
    float G = GeometrySmith(normal, viewDir, lightDir, fragRoughness);

    vec3 num = D * F * G;
    float denom = 4.0f * (max(dot(normal, viewDir), 0.0f) * max(dot(normal, lightDir), 0.0f)) + 0.001f; // 0.001 to prevent divide by zero.
    vec3 specular = num / denom;

    // kS is equal to Fresnel
    vec3 kS = F;
    // Energy conservation -> diffuse = 1 - specular
    vec3 kD = vec3(1.0f) - kS;
    // Metals have no diffuse component. Linear blend quasi-metals.
    kD = -kD*fragMetallic + kD;
    kD *= invPI;
    // scale light by NdotL
    float NdotL = max(dot(normal, lightDir), 0.0f);

    // Outgoing radiance Lo = kD*f_Lambert + kS*f_Cook-Torrance
    // Specular term already multiplied by kS==F
    vec3 Lo = (kD * albedo) + specular;

    return Lo * (NdotL * radiance);
}
