#version 400 core

out vec4 FragColor;

in vec3 v3_texCoord;

uniform samplerCube skyboxTex;

void main()
{
    FragColor = texture(skyboxTex, v3_texCoord);
}
