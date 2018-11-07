#version 400 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in v_data
{
    vec4 Color;
    vec2 texCoord;
}vertices[];

out f_data
{
    vec4 Color;
    vec2 texCoord;
    vec3 Barycentric;
}frag;

void main() {
    int ii;
    for(ii=0; ii<gl_in.length(); ii++)
    {
        frag.Color    = vertices[ii].Color;
        frag.texCoord = vertices[ii].texCoord;
        //Output barycentric coordinates for each point of the triangle
        if(ii==0)      frag.Barycentric = vec3(1.0,0.0,0.0);
        else if(ii==1) frag.Barycentric = vec3(0.0,1.0,0.0);
        else           frag.Barycentric = vec3(0.0,0.0,1.0);
            gl_Position   = gl_in[ii].gl_Position;
        EmitVertex();
    }
    EndPrimitive();
}
