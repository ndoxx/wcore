#version 400 core

const vec2 bias = vec2(0.5,0.5);

in highp vec4 position;
out mediump vec2 texc;

void main(void)
{
    gl_Position = position;
    texc.st = position.xy*bias+bias;
}
