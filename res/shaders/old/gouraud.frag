#version 400 core

struct render_data
{
    float f_wireframe_mix;  // Wireframe blend factor in[0,1]
};

struct material
{
    sampler2D diffuseTex;
    sampler2D overlayTex;   // Overlay texture test
};

in f_data
{
    vec4 Color;
    vec2 texCoord;
    vec3 Barycentric;
}frag;

layout(location = 0) out vec4 out_color;

uniform render_data rd;
uniform material    mt;

float edge_factor(); // Wireframe shading
void main()
{
    //vec4 in_color = texture(tex, gl_FragCoord.xy / tex_size);
    vec4 color = frag.Color;
    color.rgb += mix(vec3(0.0,0.8,0.1),
                     vec3(0.0,0.0,0.0),
                     edge_factor())*rd.f_wireframe_mix;

    vec4 diffuse = texture(mt.diffuseTex, frag.texCoord);
    vec4 overlay = texture(mt.overlayTex, frag.texCoord);

    if(overlay.g>0.5)
        diffuse = mix(diffuse, vec4(0.9,0.7,0.1,1.0), 0.7);

    out_color = mix(diffuse, color, 0.5);
}

// Function to compute fragment distance to edges
float edge_factor()
{
    vec3 d = fwidth(frag.Barycentric);
    vec3 a3 = smoothstep(vec3(0.0), d*1.0, frag.Barycentric);
    return min(min(a3.x, a3.y), a3.z);
}
