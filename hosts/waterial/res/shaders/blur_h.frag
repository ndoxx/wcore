#version 400 core

in mediump vec2 texc;

uniform sampler2D texture;
uniform float f_sigma;

out vec4 out_color;

const float SHARPEN_AMT = 0.2f;

void main(void)
{
    bool sharpen = (f_sigma>0.f);
    float sigma = abs(f_sigma);

    float lef4 = texc.x - 4.0 * sigma;
    float lef3 = texc.x - 3.0 * sigma;
    float lef2 = texc.x - 2.0 * sigma;
    float lef1 = texc.x - 1.0 * sigma;
    float rig1 = texc.x + 1.0 * sigma;
    float rig2 = texc.x + 2.0 * sigma;
    float rig3 = texc.x + 3.0 * sigma;
    float rig4 = texc.x + 4.0 * sigma;

    vec3 sum = vec3(0.f,0.f,0.f);

    sum += texture2D(texture, vec2(lef4, texc.y)).rgb * 0.051;
    sum += texture2D(texture, vec2(lef3, texc.y)).rgb * 0.0918;
    sum += texture2D(texture, vec2(lef2, texc.y)).rgb * 0.12245;
    sum += texture2D(texture, vec2(lef1, texc.y)).rgb * 0.1531;
    sum += texture2D(texture, texc              ).rgb * 0.1633;
    sum += texture2D(texture, vec2(rig1, texc.y)).rgb * 0.1531;
    sum += texture2D(texture, vec2(rig2, texc.y)).rgb * 0.12245;
    sum += texture2D(texture, vec2(rig3, texc.y)).rgb * 0.0918;
    sum += texture2D(texture, vec2(rig4, texc.y)).rgb * 0.051;

    if(sharpen)
    {
       vec3 src = texture2D(texture, texc).rgb;
       sum = src + SHARPEN_AMT*(src - sum); // Unsharp mask
    }

    out_color = vec4(sum, 1.f);
}
