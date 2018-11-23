#include <cassert>
#include <vector>
#include <random>

#include "heightmap_generator.h"
#include "height_map.h"
#include "math3d.h"
#include "logger.h"
#include "xml_utils.hpp"

namespace wcore
{

using namespace math;

void SimplexNoiseProps::parse_xml(rapidxml::xml_node<char>* node)
{
    xml::parse_node(node, "Octaves", octaves);
    xml::parse_node(node, "Frequency", frequency);
    xml::parse_node(node, "Persistence", persistence);
    xml::parse_node(node, "LoBound", loBound);
    xml::parse_node(node, "HiBound", hiBound);
    xml::parse_node(node, "Scale", scale);
}

void DropletErosionProps::parse_xml(rapidxml::xml_node<char>* node)
{
    xml::parse_node(node, "Iterations", iterations);
    xml::parse_node(node, "Kq", Kq);
    xml::parse_node(node, "Kw", Kw);
    xml::parse_node(node, "Kr", Kr);
    xml::parse_node(node, "Kd", Kd);
    xml::parse_node(node, "Ki", Ki);
    xml::parse_node(node, "Kg", Kg);
    xml::parse_node(node, "MinSlope", minSlope);
    xml::parse_node(node, "Epsilon", epsilon);
    xml::parse_node(node, "Seed", seed);
}

void PlateauErosionProps::parse_xml(rapidxml::xml_node<char>* node)
{
    xml::parse_node(node, "Iterations", iterations);
    xml::parse_node(node, "Talus", talus);
    xml::parse_node(node, "Fraction", fraction);
}


// TODO
// Replace this single generator by a collection held in the static class.
// This class will associate each generator to an index, and will be
// able to generate heightmaps given a generator index and a SimplexNoiseProps.
// ---------------------------------------------------------------------
NoiseGenerator2D<SimplexNoise<>> HeightmapGenerator::RNG_simplex_;


void HeightmapGenerator::init_simplex_generator(std::mt19937& rng)
{
    RNG_simplex_.init(rng);
}

void HeightmapGenerator::heightmap_from_simplex_noise(HeightMap& hm,
                                                      const SimplexNoiseProps& info)
{
    assert(hm.get_width()%2==0 && "HeightmapGenerator: Width must be even.");
    assert(hm.get_length()%2==0 && "HeightmapGenerator: Length must be even.");
    uint32_t width  = hm.get_width();
    uint32_t length = hm.get_length();

    for(uint32_t ii=0; ii<width; ++ii)
    {
        for(uint32_t jj=0; jj<length; ++jj)
        {
            float samp = RNG_simplex_.octave_noise(info.scale*(info.startX+ii),
                                                   info.scale*(info.startZ+jj),
                                                   info.octaves,
                                                   info.frequency,
                                                   info.persistence);
            samp *= (info.hiBound - info.loBound)/2;
            hm.set_height(ii, jj, samp + (info.hiBound + info.loBound)/2);
        }
    }
}
// ---------------------------------------------------------------------

void HeightmapGenerator::erode(HeightMap& hm, const PlateauErosionProps& props)
{
    for(size_t kk=0; kk<props.iterations; ++kk)
    {
        for(size_t x=0; x<hm.get_width(); ++x)
        {
            for(size_t y=0; y<hm.get_length(); ++y)
            {
                float altitude_difference_max = 0.0;
                math::vec2 pos_max(x,y);

                const float altitude_here = hm.get_height(x,y);

                hm.traverse_8_neighbors(x, y, [altitude_here, &altitude_difference_max, &pos_max](const math::vec2& pos, float altitude_there)
                {
                    float altitude_difference = altitude_here - altitude_there;
                    if (altitude_difference > altitude_difference_max)
                    {
                        altitude_difference_max = altitude_difference;
                        pos_max = pos;
                    }
                });

                if (0 < altitude_difference_max && altitude_difference_max <= props.talus)
                {
                    hm.set_height(x, y, altitude_here - props.fraction * altitude_difference_max);
                    hm.set_height(pos_max.x(), pos_max.y(), hm.get_height(pos_max.x(), pos_max.y()) + props.fraction * altitude_difference_max);
                }
            }
        }
    }
}



void HeightmapGenerator::erode_droplets(HeightMap& hmap,
                    const DropletErosionProps& params)
{
    uint32_t HMAP_WIDTH   = hmap.get_width(),  // max X
             HMAP_LENGTH  = hmap.get_length(), // max Z
             MAX_PATH_LEN = 4*fmax(HMAP_WIDTH,HMAP_LENGTH); // max droplet path length
    float Kq = params.Kq, Kw = params.Kw, Kr = params.Kr, Kd = params.Kd,
          Ki = params.Ki, minSlope = params.minSlope, Kg = params.Kg,
          epsilon = params.epsilon;

    // RNG stuff
    std::mt19937 rng;
    rng.seed(params.seed);
    std::uniform_int_distribution<uint32_t> rnd_x(0, HMAP_WIDTH-1);
    std::uniform_int_distribution<uint32_t> rnd_z(0, HMAP_LENGTH-1);
    std::uniform_real_distribution<float>   rnd_a(0, 2*M_PI);

    std::vector<vec2> erosion(HMAP_WIDTH*HMAP_LENGTH, vec2(0));

    #define HMAP_INDEX(X, Z) fmin(fmax( (X) ,0),HMAP_WIDTH-1) * HMAP_LENGTH + fmin(fmax( (Z) ,0),HMAP_LENGTH-1)
    #define DEPOSIT_AT(X, Z, W) \
    { \
        float delta=ds*(W); \
        erosion[HMAP_INDEX((X), (Z))][1] += delta; \
        hmap   [HMAP_INDEX((X), (Z))]    += delta; \
    }
    #define DEPOSIT(H) \
    { \
        DEPOSIT_AT(xi  , zi  , (1-xf)*(1-zf)) \
        DEPOSIT_AT(xi+1, zi  ,    xf *(1-zf)) \
        DEPOSIT_AT(xi  , zi+1, (1-xf)*   zf ) \
        DEPOSIT_AT(xi+1, zi+1,    xf *   zf ) \
        (H)+=ds; \
    }

    // Statistics
    uint64_t longPaths = 0, randomDirs = 0, sumLen = 0;

    // Main loop
    for(uint32_t iter=0; iter<params.iterations; ++iter)
    {
        // Generate random 2D position
        uint32_t xi = rnd_x(rng);
        uint32_t zi = rnd_z(rng);

        float xp = xi, zp = zi;
        float xf = 0,  zf = 0;
        float s = 0, v = 0, w = 1;

        float h00 = hmap.get_height_clamped(xi,   zi  );
        float h10 = hmap.get_height_clamped(xi+1, zi  );
        float h01 = hmap.get_height_clamped(xi  , zi+1);
        float h11 = hmap.get_height_clamped(xi+1, zi+1);

        float dx = 0, dz = 0, h = h00;

        uint32_t nmoves = 0;
        for(; nmoves<MAX_PATH_LEN; ++nmoves)
        {
            // Compute local gradient
            float gx = h00+h01-h10-h11;
            float gz = h00+h10-h01-h11;

            // Compute next position
            dx = (dx-gx)*Ki + gx;
            dz = (dz-gz)*Ki + gz;

            // Displacement length element
            float dl = sqrtf(dx*dx+dz*dz);
            // If didn't move much, means horizontal ground
            if(dl<epsilon)
            {
                // Pick random direction to flow
                float a = rnd_a(rng);
                dx = cosf(a);
                dz = sinf(a);
                ++randomDirs;
            }
            else
            {
                // Normalize direction
                dx /= dl;
                dz /= dl;
            }

            float nxp = xp+dx;
            float nzp = zp+dz;

            // Sample next height
            int nxi = int(floorf(nxp));
            int nzi = int(floorf(nzp));
            if(nxi<0 || nxi>HMAP_WIDTH-1 || nzi<0 || nzi>HMAP_LENGTH-1)
                break;

            float nxf = nxp-nxi;
            float nzf = nzp-nzi;

            float nh00 = hmap.get_height_clamped(nxi  , nzi  );
            float nh10 = hmap.get_height_clamped(nxi+1, nzi  );
            float nh01 = hmap.get_height_clamped(nxi  , nzi+1);
            float nh11 = hmap.get_height_clamped(nxi+1, nzi+1);

            float nh=(nh00*(1-nxf)+nh10*nxf)*(1-nzf)+(nh01*(1-nxf)+nh11*nxf)*nzf;

            // If next height nh higher than current height
            // try to deposit sediment up to neighbor height
            if(nh>=h)
            {
                float ds = (nh-h)+0.001f;

                if(ds>=s)
                {
                    // Deposit all sediment and stop
                    ds = s;
                    DEPOSIT(h)
                    s = 0;
                    break;
                }

                DEPOSIT(h)
                s -= ds;
                v = 0;
            }

            // Compute transport capacity
            float dh = h-nh;
            float slope = dh;
            //float slope=dh/sqrtf(dh*dh+1);

            float q = fmax(slope, minSlope)*v*w*Kq;

            // Deposit/erode (erode no more than dh)
            float ds = s-q;
            if(ds>=0)
            {
                // Deposit
                ds *= Kd;
                DEPOSIT(dh)
                s -= ds;
            }
            else
            {
                // Erode
                ds *= -Kr;
                ds = fmin(ds, dh*0.99f);

                #define ERODE(X, Z, W) \
                { \
                    float delta=ds*(W); \
                    hmap             [HMAP_INDEX((X), (Z))]-=delta; \
                    vec2& e = erosion[HMAP_INDEX((X), (Z))]; \
                    float r = e.x(), d = e.y(); \
                    if (delta<=d) d-=delta; \
                    else { r+=delta-d; d=0; } \
                    e[0]=r; e[1]=d; \
                }

                uint32_t z = fmax(zi-1,0);
                for (; z<=zi+2; ++z)
                {
                    float zo  = z-zp;
                    float zo2 = zo*zo;

                    uint32_t x = fmax(xi-1,0);
                    for (; x<=xi+2; ++x)
                    {
                        float xo = x-xp;
                        float w  = 1-(xo*xo+zo2)*0.25f;
                        if (w<=0) continue;
                        w *= 0.1591549430918953f;

                        ERODE(x, z, w)
                    }
                }
                #undef ERODE
                dh -= ds;
                s += ds;
            }

            // Move to neighbor
            v = sqrtf(v*v+Kg*dh);
            w *= 1-Kw;

            xp = nxp; zp = nzp;
            xi = nxi; zi = nzi;
            xf = nxf; zf = nzf;

            h   = nh;
            h00 = nh00;
            h10 = nh10;
            h01 = nh01;
            h11 = nh11;
        }

        if (nmoves>=MAX_PATH_LEN)
            ++longPaths;
        sumLen += nmoves;
    }
    #undef DEPOSIT
    #undef DEPOSIT_AT
    #undef HMAP_INDEX
}

}
