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

    float top4 = texc.y - 4.0 * sigma;
    float top3 = texc.y - 3.0 * sigma;
    float top2 = texc.y - 2.0 * sigma;
    float top1 = texc.y - 1.0 * sigma;
    float bot1 = texc.y + 1.0 * sigma;
    float bot2 = texc.y + 2.0 * sigma;
    float bot3 = texc.y + 3.0 * sigma;
    float bot4 = texc.y + 4.0 * sigma;

    vec3 sum = vec3(0.f,0.f,0.f);

    sum += texture2D(texture, vec2(texc.x, top4)).rgb * 0.051;
    sum += texture2D(texture, vec2(texc.x, top3)).rgb * 0.0918;
    sum += texture2D(texture, vec2(texc.x, top2)).rgb * 0.12245;
    sum += texture2D(texture, vec2(texc.x, top1)).rgb * 0.1531;
    sum += texture2D(texture, texc              ).rgb * 0.1633;
    sum += texture2D(texture, vec2(texc.x, bot1)).rgb * 0.1531;
    sum += texture2D(texture, vec2(texc.x, bot2)).rgb * 0.12245;
    sum += texture2D(texture, vec2(texc.x, bot3)).rgb * 0.0918;
    sum += texture2D(texture, vec2(texc.x, bot4)).rgb * 0.051;

    if(sharpen)
    {
       vec3 src = texture2D(texture, texc).rgb;
       sum = src + SHARPEN_AMT*(src - sum); // Unsharp mask
    }

    out_color = vec4(sum, 1.f);
}
