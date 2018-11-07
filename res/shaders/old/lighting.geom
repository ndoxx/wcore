#version 400 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in v_data
{
    vec3 Normal;
    vec2 texCoord;
    vec4 ViewSpace;
    vec4 FragPos;
}vertices[];

out f_data
{
    vec3 Normal;
    vec2 texCoord;
    vec4 ViewSpace;
    vec4 FragPos;
    vec3 Barycentric;
}frag;

void main() {
    int ii;
    for(ii=0; ii<gl_in.length(); ii++)
    {
        frag.Normal    = vertices[ii].Normal;
        frag.texCoord  = vertices[ii].texCoord;
        frag.ViewSpace = vertices[ii].ViewSpace;
        frag.FragPos   = vertices[ii].FragPos;
        //Output barycentric coordinates for each point of the triangle
        if(ii==0)      frag.Barycentric = vec3(1.0,0.0,0.0);
        else if(ii==1) frag.Barycentric = vec3(0.0,1.0,0.0);
        else           frag.Barycentric = vec3(0.0,0.0,1.0);
            gl_Position   = gl_in[ii].gl_Position;
        EmitVertex();
    }
    EndPrimitive();
}
