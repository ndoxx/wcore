#version 400 core

struct render_data
{
    float f_wireframe_mix;  // Wireframe blend factor in[0,1]
};

in f_data
{
    vec3 Normal;
    vec4 Color;
    vec4 ViewSpace;
    vec4 FragPos;
    vec3 Barycentric;
}frag;

layout(location = 0) out vec4 out_color;

uniform render_data rd;

float edge_factor(); // Wireframe shading
vec3  calc_dirlight(Renderer, vec3, vec3, float); // Phong model
void main()
{
    //vec4 in_color = texture(tex, gl_FragCoord.xy / tex_size);
    vec4 color = frag.Color;
    color.rgb += mix(vec3(0.8,0.5,0.0),
                     vec3(0.0,0.0,0.0),
                     edge_factor())*rd.f_wireframe_mix;
    out_color = color;
}

// Function to compute fragment distance to edges
float edge_factor()
{
    vec3 d = fwidth(frag.Barycentric);
    vec3 a3 = smoothstep(vec3(0.0), d*1.0, frag.Barycentric);
    return min(min(a3.x, a3.y), a3.z);
}

// Function to compute Phong model on fragment given a directional light
vec3 calc_dirlight(Renderer renderer, vec3 normal, vec3 viewDir, float visibility)
{
    // Light direction is just its position vector
    vec3 lightDir   = normalize(renderer.lightPosition.xyz);
    // Reflection direction of light with respect to local normal
    vec3 reflectDir = normalize(reflect(-lightDir, normal));

    // Diffuse shading factor
    float diffFactor = dot(normal, lightDir);
    diffFactor = clamp(diffFactor,0.0,1.0);

    // Specular shading factor
    float specFactor = 0.0;
    if(diffFactor>=0.0)
    {
        specFactor = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
        specFactor = clamp(specFactor,0.0,1.0);
    }

    // Combine results
    vec3 ambient  = renderer.lightAmbient  * vec3(texture(material.diffuse,  frag.TexCoord));
    vec3 diffuse  = renderer.lightDiffuse  * diffFactor * vec3(texture(material.diffuse,  frag.TexCoord)) * visibility;
    vec3 specular = renderer.lightSpecular * specFactor * vec3(texture(material.specular, frag.TexCoord)) * visibility;
    return (ambient + diffuse + specular);
}
