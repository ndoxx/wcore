#version 400 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 vertex_pos[];
in vec3 vertex_normal[];
in vec3 tangent_viewDir[];
in vec2 vertex_texCoord[];
in mat3 vertex_TBN[];

out vec3 frag_pos;
out vec3 frag_normal;
out vec3 frag_barycentric;
out vec3 frag_tangent_viewDir;
out vec2 frag_texCoord;
out mat3 frag_TBN;

void main() {
    int ii;
    for(ii=0; ii<gl_in.length(); ii++)
    {
        frag_tangent_viewDir = tangent_viewDir[ii];
        frag_normal   = vertex_normal[ii];
        frag_texCoord = vertex_texCoord[ii];
        frag_pos      = vertex_pos[ii];
        frag_TBN      = vertex_TBN[ii];
        //Output barycentric coordinates for each point of the triangle
        if(ii==0)      frag_barycentric = vec3(1.0,0.0,0.0);
        else if(ii==1) frag_barycentric = vec3(0.0,1.0,0.0);
        else           frag_barycentric = vec3(0.0,0.0,1.0);
            gl_Position   = gl_in[ii].gl_Position;
        EmitVertex();
    }
    EndPrimitive();
}
