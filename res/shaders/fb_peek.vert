#version 400 core
layout(location = 0) in vec3 in_position;

out vec2 texCoord;

void main()
{
    gl_Position = vec4(in_position, 1.0);
    texCoord = (in_position.xy + 1.0)/2.0;
}
