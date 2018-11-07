#version 400 core

struct render_data
{
    float f_wireframe_mix;  // Wireframe blend factor in[0,1]
    vec3 v3_viewPos;
};

struct material
{
    sampler2D diffuseTex;
    sampler2D specularTex;
    sampler2D overlayTex;   // Overlay texture test
    float f_shininess;
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

    float f_light_att_K0;
    float f_light_att_K1;
    float f_light_att_K2;
};
#define N_PT_LIGHTS 3

in f_data
{
    vec3 Normal;
    vec2 texCoord;
    vec4 ViewSpace;
    vec4 FragPos;
    vec3 Barycentric;
}frag;

layout(location = 0) out vec3 out_color;

// UNIFORMS
uniform render_data rd;
uniform material    mt;
uniform dir_light   dl;
uniform point_light pl[N_PT_LIGHTS];


float edge_factor(); // Wireframe shading
vec3  calc_dirlight(dir_light, vec3, vec3, float);    // Directional light contribution
vec3 calc_point_light(point_light, vec3, vec3, vec3); // Point light contribution
void main()
{
    // Normal vector interpolated at fragment position
    vec3 norm    = normalize(frag.Normal);
    // Fragment position
    vec3 fragPos = frag.FragPos.xyz/frag.FragPos.w;
    // View direction as a vector from fragment position to camera position
    vec3 viewDir = normalize(rd.v3_viewPos - fragPos);

    // Directional light constribution
    vec3 directional_contrib = calc_dirlight(dl, norm, viewDir, 1.0f);

    // Point lights contribution
    vec3 point_contrib = vec3(0.0,0.0,0.0);
    for(int ii=0; ii<N_PT_LIGHTS; ++ii)
    {
        point_contrib += calc_point_light(pl[ii], norm, fragPos, viewDir);
    }


    vec3 total_light = directional_contrib + point_contrib;

    // Wireframe color contribution
    vec3 wireframe_contrib = mix(vec3(0.8,0.5,0.1),
                                 vec3(0.0,0.0,0.0),
                                 edge_factor())*rd.f_wireframe_mix;

    // Debug overlay mask
    vec4 overlay = texture(mt.overlayTex, frag.texCoord);
    bool overlay_mask = (overlay.g>0.5);

    // Blending
    vec3 color = mix(total_light, wireframe_contrib, 0.5);
    if(overlay_mask)
    {
        color = mix(color, vec3(1,0.5,0), 0.5);
    }

    out_color = color;
}

// Function to compute fragment distance to edges
float edge_factor()
{
    vec3 d = fwidth(frag.Barycentric);
    vec3 a3 = smoothstep(vec3(0.0), d*1.0, frag.Barycentric);
    return min(min(a3.x, a3.y), a3.z);
}

vec3 calc_dirlight(dir_light light, vec3 normal, vec3 viewDir, float visibility)
{
    // Light direction is just its position vector
    vec3 lightDir   = normalize(light.v3_lightPosition);
    // Reflection direction of light with respect to local normal
    vec3 reflectDir = normalize(reflect(-lightDir, normal));

    // Diffuse shading factor
    float diffFactor = dot(normal, lightDir);
    diffFactor = clamp(diffFactor,0.0,1.0);

    // Specular shading factor
    float specFactor = 0.0;
    if(diffFactor>=0.0)
    {
        specFactor = pow(max(dot(viewDir, reflectDir), 0.0), mt.f_shininess);
        specFactor = clamp(specFactor,0.0,1.0);
    }

    // Combine results
    vec3 ambient  = 0.1*light.v3_lightColor * vec3(texture(mt.diffuseTex,  frag.texCoord));
    vec3 diffuse  = light.v3_lightColor * diffFactor * vec3(texture(mt.diffuseTex,  frag.texCoord)) * visibility;
    vec3 specular = light.v3_lightColor * specFactor * vec3(texture(mt.specularTex, frag.texCoord)) * visibility;
    return (ambient + diffuse + specular);
}

vec3 calc_point_light(point_light light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.v3_lightPosition - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), mt.f_shininess);
    // attenuation
    float distance    = length(light.v3_lightPosition - fragPos);
    float attenuation = 1.0 / (light.f_light_att_K0 + light.f_light_att_K1 * distance +
                 light.f_light_att_K2 * (distance * distance));
    // combine results
    vec3 ambient  = 0.1*light.v3_lightColor * vec3(texture(mt.diffuseTex, frag.texCoord));
    vec3 diffuse  = light.v3_lightColor * diff * vec3(texture(mt.diffuseTex, frag.texCoord));
    vec3 specular = light.v3_lightColor * spec * vec3(texture(mt.specularTex, frag.texCoord));
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}
