#version 400 core

in mediump vec2 texc;

uniform sampler2D texture;
uniform int i_filter;
uniform float f_invert_r;
uniform float f_invert_g;
uniform float f_invert_h;
uniform float f_dz;

uniform float f_step_x;
uniform float f_step_y;

out vec4 out_color;

void main(void)
{
    vec2 tlv = vec2(texc.x + f_step_x, texc.y - f_step_y);
    vec2 lv  = vec2(texc.x + f_step_x, texc.y           );
    vec2 blv = vec2(texc.x + f_step_x, texc.y + f_step_y);
    vec2 tv  = vec2(texc.x           , texc.y - f_step_y);
    vec2 bv  = vec2(texc.x           , texc.y + f_step_y);
    vec2 trv = vec2(texc.x - f_step_x, texc.y - f_step_y);
    vec2 rv  = vec2(texc.x - f_step_x, texc.y           );
    vec2 brv = vec2(texc.x - f_step_x, texc.y + f_step_y);

    float tl = abs(texture2D(texture, tlv).r);
    float l  = abs(texture2D(texture, lv ).r);
    float bl = abs(texture2D(texture, blv).r);
    float t  = abs(texture2D(texture, tv ).r);
    float b  = abs(texture2D(texture, bv ).r);
    float tr = abs(texture2D(texture, trv).r);
    float r  = abs(texture2D(texture, rv ).r);
    float br = abs(texture2D(texture, brv).r);

    float dx = 0.f, dy = 0.f;
    if(i_filter == 0)
    {
        // normalized Sobel-Feldman kernel
        dx = (tl + l*2.f + bl - tr - r*2.f - br) / 4.f;
        dy = (tl + t*2.f + tr - bl - b*2.f - br) / 4.f;
    }
    else if(i_filter == 1)
    {
        // normalized Scharr kernel
        //dx = (tl*3.f + l*10.f + bl*3.f - tr*3.f - r*10.f - br*3.f) / 16.f;
        //dy = (tl*3.f + t*10.f + tr*3.f - bl*3.f - b*10.f - br*3.f) / 16.f;

        // normalized optimal Scharr kernel
        dx = (tl*47.f + l*162.f + bl*47.f - tr*47.f - r*162.f - br*47.f) / 256.f;
        dy = (tl*47.f + t*162.f + tr*47.f - bl*47.f - b*162.f - br*47.f) / 256.f;
    }

    vec3 normal =vec3(dx * f_invert_r * f_invert_h,
                      dy * f_invert_g * f_invert_h,
                      f_dz);
    normal = normalize(normal);

    vec3 n = vec3(normal.x*0.5f + 0.5f, normal.y*0.5f + 0.5f, normal.z);

    out_color = vec4(n, 1.f);
}
