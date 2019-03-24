// OBSOLETE: Texture map generation moved to GPU

// Normal map generation algorithm inspired from the work of Christian Petry
// Licence:
/*
 * Author: Christian Petry
 * Homepage: www.petry-christian.de
 *
 * License: MIT
 * Copyright (c) 2014 Christian Petry
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <cmath>
#include <iostream>

#include "texmap_generator.h"
#include "math3d.h"
#include "algorithms.h"

using namespace wcore::math;

namespace waterial
{
namespace generator
{

const float SHARPEN_AMT = 0.2f;

inline float wrap_uv(float uv)
{
    uv   = (uv >= 0.f) ? (uv-(long)uv) : 1.f+(uv-(long)uv);
    return (uv <  1.f) ? uv : uv  - 1.f;
}

float sample_red_nearest(const QImage& img, const vec2& uv)
{
    int x = std::floor(wrap_uv(uv.x())*img.width());
    int y = std::floor(wrap_uv(uv.y())*img.height());

    return qRed(img.pixel(x,y)) / 255.f;
}

vec3 sample_nearest(const QImage& img, const vec2& uv)
{
    int x = std::floor(wrap_uv(uv.x())*img.width());
    int y = std::floor(wrap_uv(uv.y())*img.height());

    return vec3(qRed(img.pixel(x,y))   / 255.f,
                qGreen(img.pixel(x,y)) / 255.f,
                qBlue(img.pixel(x,y))  / 255.f);
}

float dz_from_level_strength(float level, float strength)
{
    return clamp<float>(1.f/strength * (1.f + pow(2.f, level)) * (1.f/255.f), 0.f, 1.f);
}

void normal_from_depth(const QImage& depth_map, QImage& normal_map, const NormalGenOptions& options)
{
    int w = normal_map.width();
    int h = normal_map.height();

    vec2 step(1.f/w, 1.f/h);
    float dz = dz_from_level_strength(options.level, options.strength);

    for(int xx=0; xx<w; ++xx)
    {
        for(int yy=0; yy<h; ++yy)
        {
            vec2 vUv(xx/float(w), yy/float(h));

            vec2 tlv = vec2(vUv.x() + step.x(), vUv.y() - step.y());
            vec2 lv  = vec2(vUv.x() + step.x(), vUv.y()           );
            vec2 blv = vec2(vUv.x() + step.x(), vUv.y() + step.y());
            vec2 tv  = vec2(vUv.x()           , vUv.y() - step.y());
            vec2 bv  = vec2(vUv.x()           , vUv.y() + step.y());
            vec2 trv = vec2(vUv.x() - step.x(), vUv.y() - step.y());
            vec2 rv  = vec2(vUv.x() - step.x(), vUv.y()           );
            vec2 brv = vec2(vUv.x() - step.x(), vUv.y() + step.y());

            float tl = std::fabs(sample_red_nearest(depth_map, tlv));
            float l  = std::fabs(sample_red_nearest(depth_map, lv ));
            float bl = std::fabs(sample_red_nearest(depth_map, blv));
            float t  = std::fabs(sample_red_nearest(depth_map, tv ));
            float b  = std::fabs(sample_red_nearest(depth_map, bv ));
            float tr = std::fabs(sample_red_nearest(depth_map, trv));
            float r  = std::fabs(sample_red_nearest(depth_map, rv ));
            float br = std::fabs(sample_red_nearest(depth_map, brv));

            float dx = 0.f, dy = 0.f;
            if(options.filter == FilterType::SOBEL)
            {
                // normalized Sobel-Feldman kernel
                dx = (tl + l*2.f + bl - tr - r*2.f - br) / 4.f;
                dy = (tl + t*2.f + tr - bl - b*2.f - br) / 4.f;
            }
            else if(options.filter == FilterType::SCHARR)
            {
                // normalized Scharr kernel
                //dx = (tl*3.f + l*10.f + bl*3.f - tr*3.f - r*10.f - br*3.f) / 16.f;
                //dy = (tl*3.f + t*10.f + tr*3.f - bl*3.f - b*10.f - br*3.f) / 16.f;

                // normalized optimal Scharr kernel
                dx = (tl*47.f + l*162.f + bl*47.f - tr*47.f - r*162.f - br*47.f) / 256.f;
                dy = (tl*47.f + t*162.f + tr*47.f - bl*47.f - b*162.f - br*47.f) / 256.f;
            }

            vec3 normal(dx * options.invert_r * options.invert_h,
                        dy * options.invert_g * options.invert_h,
                        dz);
            normal.normalize();

            vec3 n(normal.x()*0.5f + 0.5f, normal.y()*0.5f + 0.5f, normal.z());

            QRgb out_color = qRgb(int(std::floor(n.x()*255)),
                                  int(std::floor(n.y()*255)),
                                  int(std::floor(n.z()*255)));
            normal_map.setPixel(xx, yy, out_color);
        }
    }
}

void blur_sharp(QImage& img, float sigma)
{
    if(sigma == 0.f) return;

    int w = img.width();
    int h = img.height();

    bool sharpen = (sigma>0.f);
    sigma = std::fabs(sigma);

    float sigma_x = (1.f/5.f) * (sigma/w);
    float sigma_y = (1.f/5.f) * (sigma/h);

    QImage img_tmp(w, h, QImage::Format_RGBA8888);

    // Horizontal pass: img -> img_tmp
    for(int xx=0; xx<w; ++xx)
    {
        for(int yy=0; yy<h; ++yy)
        {
            vec2 vUv(xx/float(w), yy/float(h));

            float lef4 = vUv.x() - 4.0 * sigma_x;
            float lef3 = vUv.x() - 3.0 * sigma_x;
            float lef2 = vUv.x() - 2.0 * sigma_x;
            float lef1 = vUv.x() - 1.0 * sigma_x;
            float rig1 = vUv.x() + 1.0 * sigma_x;
            float rig2 = vUv.x() + 2.0 * sigma_x;
            float rig3 = vUv.x() + 3.0 * sigma_x;
            float rig4 = vUv.x() + 4.0 * sigma_x;

            vec3 sum(0);
            sum += sample_nearest(img, vec2(lef4, vUv.y())) * 0.051;
            sum += sample_nearest(img, vec2(lef3, vUv.y())) * 0.0918;
            sum += sample_nearest(img, vec2(lef2, vUv.y())) * 0.12245;
            sum += sample_nearest(img, vec2(lef1, vUv.y())) * 0.1531;
            sum += sample_nearest(img, vUv                ) * 0.1633;
            sum += sample_nearest(img, vec2(rig1, vUv.y())) * 0.1531;
            sum += sample_nearest(img, vec2(rig2, vUv.y())) * 0.12245;
            sum += sample_nearest(img, vec2(rig3, vUv.y())) * 0.0918;
            sum += sample_nearest(img, vec2(rig4, vUv.y())) * 0.051;

            if(sharpen)
            {
               vec3 src(sample_nearest(img, vUv));
               sum = src + SHARPEN_AMT*(src - sum); // Unsharp mask
            }

            QRgb out_color = qRgb(int(std::floor(sum.x()*255)),
                                  int(std::floor(sum.y()*255)),
                                  int(std::floor(sum.z()*255)));
            img_tmp.setPixel(xx, yy, out_color);
        }
    }

    // Vertical pass: img_tmp -> img
    for(int xx=0; xx<w; ++xx)
    {
        for(int yy=0; yy<h; ++yy)
        {
            vec2 vUv(xx/float(w), yy/float(h));

            float top4 = vUv.y() - 4.0 * sigma_y;
            float top3 = vUv.y() - 3.0 * sigma_y;
            float top2 = vUv.y() - 2.0 * sigma_y;
            float top1 = vUv.y() - 1.0 * sigma_y;
            float bot1 = vUv.y() + 1.0 * sigma_y;
            float bot2 = vUv.y() + 2.0 * sigma_y;
            float bot3 = vUv.y() + 3.0 * sigma_y;
            float bot4 = vUv.y() + 4.0 * sigma_y;

            vec3 sum(0);
            sum += sample_nearest(img_tmp, vec2(vUv.x(), top4)) * 0.051;
            sum += sample_nearest(img_tmp, vec2(vUv.x(), top3)) * 0.0918;
            sum += sample_nearest(img_tmp, vec2(vUv.x(), top2)) * 0.12245;
            sum += sample_nearest(img_tmp, vec2(vUv.x(), top1)) * 0.1531;
            sum += sample_nearest(img_tmp, vUv                ) * 0.1633;
            sum += sample_nearest(img_tmp, vec2(vUv.x(), bot1)) * 0.1531;
            sum += sample_nearest(img_tmp, vec2(vUv.x(), bot2)) * 0.12245;
            sum += sample_nearest(img_tmp, vec2(vUv.x(), bot3)) * 0.0918;
            sum += sample_nearest(img_tmp, vec2(vUv.x(), bot4)) * 0.051;

            if(sharpen)
            {
               vec3 src(sample_nearest(img_tmp, vUv));
               sum = src + SHARPEN_AMT*(src - sum); // Unsharp mask
            }

            QRgb out_color = qRgb(int(std::floor(sum.x()*255)),
                                  int(std::floor(sum.y()*255)),
                                  int(std::floor(sum.z()*255)));
            img.setPixel(xx, yy, out_color);
        }
    }
}

void ao_from_depth(const QImage& depth_map, QImage& ao_map, const AOGenOptions& options)
{
    int w = ao_map.width();
    int h = ao_map.height();

    for(int xx=0; xx<w; ++xx)
    {
        for(int yy=0; yy<h; ++yy)
        {
            vec2 vUv(xx/float(w), yy/float(h));

            // Lower value
            float depth = sample_red_nearest(depth_map,  vUv);
            // inside range around mean value?!
            float perc_dist_to_mean = (options.range - std::fabs(depth - options.mean)) / options.range;
            float val = (perc_dist_to_mean > 0.f) ? sqrt(perc_dist_to_mean) : 0.f;
            // multiply by strength
            val += (1.f-val) * (1.f-options.strength);
            // invert if necessary
            val = (options.invert) ? (1.f-val) : val;
            // convert to rgb8
            val = int(std::floor(val*255));
            QRgb out_color = qRgb(val,val,val);
            ao_map.setPixel(xx, yy, out_color);
        }
    }
}

} // namespace generator
} // namespace waterial

