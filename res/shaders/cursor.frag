#version 400 core
in vec2 texCoord;
out vec4 out_color;

uniform sampler2D inputTex;
uniform vec3 v3_color;

void main()
{
    float value = texture(inputTex, texCoord).r;
    float solid = step(0.5f, value);
    out_color = vec4(v3_color, value) * solid;
}
