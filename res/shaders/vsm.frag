#version 400 core

in vec4 vertex_pos;

layout(location = 0) out vec4 out_shadowMap;

void main()
{
    float depth = vertex_pos.z/vertex_pos.w ;
    depth = depth * 0.5f + 0.5f; //Don't forget to move away from unit cube ([-1,1]) to [0,1] coordinate system

    float moment1 = depth;
    float moment2 = depth*depth;

    // Adjusting moments (this is sort of bias per pixel) using derivative
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    moment2 += 0.25f*(dx*dx+dy*dy);

    //! Valide pour un FBO 32bits
    //! Pour un FBO 16bits packer dans les 4 composantes pour ne pas
    //! perdre en précision.
    out_shadowMap = vec4(moment1, moment2, 0.0f, 0.0f);
}
