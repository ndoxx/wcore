#include <cmath>
#include <iostream>

#include "normal_generator.h"
#include "math3d.h"

using namespace wcore::math;

namespace medit
{
namespace normal
{

static bool invertR = false;
static bool invertG = false;
static bool invertH = false;
static float dz = 1.f;

static float sample_nearest(const QImage& img, const vec2& uv)
{
    int x = std::round(uv.x()*img.width());
    int y = std::round(uv.y()*img.height());

    return qRed(img.pixel(x,y)) / 255.f;
}

void generate_from_depth(const QImage& depth_map, QImage& normal_map, FilterType filter)
{
    int w = normal_map.width();
    int h = normal_map.height();

    vec2 step(-1.f/w, -1.f/h);

    for(int xx=0; xx<w; ++xx)
    {
        for(int yy=0; yy<h; ++yy)
        {
            vec2 vUv(xx/float(w), yy/float(h));

            vec2 tlv = vec2(vUv.x() - step.x(), vUv.y() + step.y() );
            vec2 lv  = vec2(vUv.x() - step.x(), vUv.y()          );
            vec2 blv = vec2(vUv.x() - step.x(), vUv.y() - step.y());
            vec2 tv  = vec2(vUv.x()           , vUv.y() + step.y() );
            vec2 bv  = vec2(vUv.x()           , vUv.y() - step.y());
            vec2 trv = vec2(vUv.x() + step.x(), vUv.y() + step.y() );
            vec2 rv  = vec2(vUv.x() + step.x(), vUv.y()          );
            vec2 brv = vec2(vUv.x() + step.x(), vUv.y() - step.y());

            tlv = vec2(tlv.x() >= 0.f ? tlv.x() : (1.f + tlv.x()),   tlv.y() >= 0.f ? tlv.y() : (1.f  + tlv.y()));
            tlv = vec2(tlv.x() < 1.f  ? tlv.x() : (tlv.x() - 1.f ),  tlv.y() < 1.f  ? tlv.y() : (tlv.y() - 1.f ));
            lv  = vec2( lv.x() >= 0.f ?  lv.x() : (1.f + lv.x()),    lv.y()  >= 0.f ?  lv.y() : (1.f  +  lv.y()));
            lv  = vec2( lv.x() < 1.f  ?  lv.x() : ( lv.x() - 1.f ),  lv.y()  < 1.f  ?  lv.y() : ( lv.y() - 1.f ));
            blv = vec2(blv.x() >= 0.f ? blv.x() : (1.f + blv.x()),   blv.y() >= 0.f ? blv.y() : (1.f  + blv.y()));
            blv = vec2(blv.x() < 1.f  ? blv.x() : (blv.x() - 1.f ),  blv.y() < 1.f  ? blv.y() : (blv.y() - 1.f ));
            tv  = vec2( tv.x() >= 0.f ?  tv.x() : (1.f + tv.x()),    tv.y()  >= 0.f ?  tv.y() : (1.f  +  tv.y()));
            tv  = vec2( tv.x() < 1.f  ?  tv.x() : ( tv.x() - 1.f ),  tv.y()  < 1.f  ?  tv.y() : ( tv.y() - 1.f ));
            bv  = vec2( bv.x() >= 0.f ?  bv.x() : (1.f + bv.x()),    bv.y()  >= 0.f ?  bv.y() : (1.f  +  bv.y()));
            bv  = vec2( bv.x() < 1.f  ?  bv.x() : ( bv.x() - 1.f ),  bv.y()  < 1.f  ?  bv.y() : ( bv.y() - 1.f ));
            trv = vec2(trv.x() >= 0.f ? trv.x() : (1.f + trv.x()),   trv.y() >= 0.f ? trv.y() : (1.f  + trv.y()));
            trv = vec2(trv.x() < 1.f  ? trv.x() : (trv.x() - 1.f ),  trv.y() < 1.f  ? trv.y() : (trv.y() - 1.f ));
            rv  = vec2( rv.x() >= 0.f ?  rv.x() : (1.f + rv.x()),    rv.y()  >= 0.f ?  rv.y() : (1.f  +  rv.y()));
            rv  = vec2( rv.x() < 1.f  ?  rv.x() : ( rv.x() - 1.f ),  rv.y()  < 1.f  ?  rv.y() : ( rv.y() - 1.f ));
            brv = vec2(brv.x() >= 0.f ? brv.x() : (1.f + brv.x()),   brv.y() >= 0.f ? brv.y() : (1.f  + brv.y()));
            brv = vec2(brv.x() < 1.f  ? brv.x() : (brv.x() - 1.f ),  brv.y() < 1.f  ? brv.y() : (brv.y() - 1.f ));

            float tl = std::fabs(sample_nearest(depth_map, tlv));
            float l  = std::fabs(sample_nearest(depth_map, lv ));
            float bl = std::fabs(sample_nearest(depth_map, blv));
            float t  = std::fabs(sample_nearest(depth_map, tv ));
            float b  = std::fabs(sample_nearest(depth_map, bv ));
            float tr = std::fabs(sample_nearest(depth_map, trv));
            float r  = std::fabs(sample_nearest(depth_map, rv ));
            float br = std::fabs(sample_nearest(depth_map, brv));

            float dx = 0.f, dy = 0.f;
            if(filter == FilterType::SOBEL)
            {
                dx = tl + l*2.f + bl - tr - r*2.f - br;
                dy = tl + t*2.f + tr - bl - b*2.f - br;
            }
            else // Scharr
            {
                dx = tl*3.f + l*10.f + bl*3.f - tr*3.f - r*10.f - br*3.f;
                dy = tl*3.f + t*10.f + tr*3.f - bl*3.f - b*10.f - br*3.f;
            }

            //vec3 normal(dx * invertR * invertH * 255.0, dy * invertG * invertH * 255.0, dz);
            vec3 normal(dx * 255.0, dy * 255.0, dz * 255.0);
            normal.normalize();

            vec3 n(normal.x()*0.5f + 0.5f, normal.y()*0.5f + 0.5f, normal.z());

            QRgb out_color = qRgb(int(std::floor(n.x()*255)),
                                  int(std::floor(n.y()*255)),
                                  int(std::floor(n.z()*255)));
            normal_map.setPixel(xx, yy, out_color);
        }
    }
}

} // namespace normal
} // namespace medit

