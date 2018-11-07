#version 400 core
in vec2 texCoord;
out vec4 out_color;

uniform sampler2D textTex;
uniform vec3 v3_textColor;

void main()
{
    vec4 samp = vec4(1.0, 1.0, 1.0, texture(textTex, texCoord).r);
    out_color = vec4(v3_textColor, 1.0) * samp;
}
