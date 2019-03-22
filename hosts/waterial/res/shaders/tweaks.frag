#version 400 core

in mediump vec2 texc;

uniform sampler2D texture;
uniform float f_hue = 0.f;
uniform float f_saturation = 1.f;
uniform float f_value = 1.f;

out vec4 out_color;

vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = c.g < c.b ? vec4(c.bg, K.wz) : vec4(c.gb, K.xy);
    vec4 q = c.r < p.x ? vec4(p.xyw, c.r) : vec4(c.r, p.yzx);

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 hsv_adjust(vec3 color, float hue, float saturation, float value)
{
    vec3 hsv_color = rgb2hsv(color);
    hsv_color.x += hue;
    hsv_color.y = clamp(hsv_color.y + saturation, 0.0, 1.0);
    hsv_color.z = clamp(hsv_color.z + value, 0.0, 1.0);
    return hsv2rgb(hsv_color);
}

void main(void)
{
    vec3 in_color = texture2D(texture, texc.st).rgb;
    out_color = vec4(hsv_adjust(in_color, f_hue, f_saturation, f_value), 1.f);
}
