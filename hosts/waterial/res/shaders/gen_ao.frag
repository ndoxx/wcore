#version 400 core

in mediump vec2 texc;

uniform sampler2D texture;
uniform bool b_invert;
uniform float f_strength;
uniform float f_mean;
uniform float f_range;

out vec4 out_color;

void main(void)
{
    // Lower value
    float depth = texture2D(texture, texc.st).r;

    // inside range around mean value?!
    float perc_dist_to_mean = (f_range - abs(depth - f_mean)) / f_range;
    float val = (perc_dist_to_mean > 0.f) ? sqrt(perc_dist_to_mean) : 0.f;

    // multiply by strength
    val += (1.f-val) * (1.f-f_strength);

    // invert if necessary
    val = (b_invert) ? (1.f-val) : val;
    out_color = vec4(val, val, val, 1.f);
}
