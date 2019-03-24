#version 400 core

in mediump vec2 texc;

uniform sampler2D texture;

out vec4 out_color;

void main(void)
{
    out_color = texture2D(texture, texc.st);
}
